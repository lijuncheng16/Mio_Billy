#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strophe.h>
#include <common.h>
#include <mio.h>
#include <mio_handlers.h>
#include <stdlib.h>
#include <signal.h>
#include <uuid/uuid.h>
#include <time.h>
#include <stdlib.h>

#define TRUE                    1

#define N_PUBLISHERS_FLAG       "-p\0"
#define N_SUBSCRIBERS_FLAG      "-s\0"
#define PUBLISH_FACTOR_FLAG     "-np\0"
#define SUBSCRIBE_FACTOR_FLAG   "-ns\0"
#define USER_FLAG               "-u\0"
#define PASS_FLAG               "-pass\0"
#define SERVER_FLAG             "-ser\0"
#define TIME_FLAG               "-t\0"
#define PUBLISH_SIZE_FLAG       "-d\0"
#define PORT_FLAG               "-port\0"


#define UUID_LENGTH             47

// benchmark_t holds benchmark description
typedef struct 
{
  int n_publishers; // number of publishers 
  int n_subscribers; // number of subscribers
  int subscribe_factor; // number of event nodes to subscribe to
  int publish_factor; // number of event nodes to publish to
  char *user; // administrator username
  char *pass; // administrator password
  char *server; // xmpp server address
  int port; // xmpp server port
  int publish_size; // bytes to publish
  int time; // in seconds to publish/subscribe to
} benchmark_t;

typedef struct
{
    int user_index;
    benchmark_t *setup;
} thread_args_t;

// The connections for publishers and subscribers
static mio_conn_t **publishers = NULL;
static mio_conn_t **subscribers = NULL;

// The uuids of the nodes that each publisher publishes to
//      and each subscriber subscribes to.
static char ***publish_nodes = NULL;
static char ***subscribe_nodes = NULL;

// Increment count when publisher publishes
//      and increment when subscriber receives.
static int **publish_count = NULL;
static int **subscribe_count = NULL;

// References to each of the publish and subscribe
//      routine threads
static pthread_t *publish_threads = NULL;
static pthread_t *subscribe_threads = NULL;

static benchmark_t* parse_args(int argc, char **argv)
{
  int arg_index;
  char *arg_flag, *arg_val;
  benchmark_t* benchmark = malloc(sizeof(benchmark_t));
  memset(benchmark,0x0,sizeof(benchmark_t));

  for (arg_index = 1; arg_index < argc-1; arg_index++)
  {
    arg_flag = argv[arg_index];
    arg_val = argv[arg_index + 1];
    printf("flag %s val %s\n",arg_flag,arg_val);
    if (strcmp(arg_flag, N_PUBLISHERS_FLAG) == 0)
    {
      benchmark -> n_publishers = atoi(arg_val);
      arg_index ++;
    } else if (strcmp(arg_flag, N_SUBSCRIBERS_FLAG) == 0)
    {
      printf("n subscriber %d\n", atoi(arg_val));
      benchmark -> n_subscribers = atoi(arg_val);
      arg_index ++;
    }  else if (strcmp(arg_flag, SUBSCRIBE_FACTOR_FLAG) == 0)
    {
      printf("subfactor %d\n", atoi(arg_val));
      benchmark -> subscribe_factor = atoi(arg_val) ;
      arg_index ++;
    } else if (strcmp(arg_flag, PUBLISH_FACTOR_FLAG) == 0)
    {
      benchmark -> publish_factor = atoi(arg_val);
      arg_index ++;
    } else if (strcmp(arg_flag, USER_FLAG) == 0)
    {
      benchmark -> user = arg_val;
      arg_index ++;
    } else if (strcmp(arg_flag, PASS_FLAG) == 0)
    {
      benchmark -> pass = arg_val;
      arg_index ++;
    } else if (strcmp(arg_flag, SERVER_FLAG) == 0)
    {
      benchmark -> server = arg_val;
      arg_index ++;
    } else if (strcmp(arg_flag, PORT_FLAG) == 0)
    { 
      benchmark -> port = atoi(arg_val);
      arg_index ++;
    } else if (strcmp(arg_flag, TIME_FLAG) == 0)
    { 
      benchmark -> time = atoi(arg_val);
      arg_index ++;
    } else if (strcmp(arg_flag, PUBLISH_SIZE_FLAG) == 0)
    { 
      benchmark -> publish_size = atoi(arg_val);
      arg_index ++;
    }
  }
  return benchmark;
}

int register_user(mio_conn_t *conn, char *user, char *pass)
{
	int err;
	mio_response_t *response = mio_response_new();
    xmpp_stanza_t *iq = NULL, *query = NULL, *username = NULL, *username_tag = NULL, *password = NULL, *password_tag = NULL;
    mio_stanza_t *stanza = mio_stanza_new(conn);
    // Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		return MIO_ERROR_DISCONNECTED;
	}

    // Create a new user stanza
	iq = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(iq, "iq");
	xmpp_stanza_set_attribute(iq, "id", "reg2");
	xmpp_stanza_set_type(iq, "set");

    // Creqte query stanza
    query = xmpp_stanza_new(conn -> xmpp_conn -> ctx);
    xmpp_stanza_set_name(query,"query");
    xmpp_stanza_set_ns(query,"jabber:iq:register");
    
    // create username stanza 
    username = xmpp_stanza_new(conn -> xmpp_conn -> ctx);
    username_tag = xmpp_stanza_new(conn -> xmpp_conn -> ctx);
    xmpp_stanza_set_name(username,"username");
    xmpp_stanza_set_text(username_tag, user);



    // create password stanza
    password = xmpp_stanza_new(conn -> xmpp_conn -> ctx);
    password_tag = xmpp_stanza_new(conn -> xmpp_conn -> ctx);
    xmpp_stanza_set_name(password, "password");
    xmpp_stanza_set_text(password_tag,pass);

    // create password stanza
    password = xmpp_stanza_new(conn -> xmpp_conn -> ctx);
    password_tag = xmpp_stanza_new(conn -> xmpp_conn -> ctx);
    xmpp_stanza_set_name(password, "password");
    xmpp_stanza_set_text(password_tag,pass);
    // Build xmpp message
	xmpp_stanza_add_child(password, password_tag);
	xmpp_stanza_add_child(username, username_tag);
	xmpp_stanza_add_child(query, username);
	xmpp_stanza_add_child(query, password);
	xmpp_stanza_add_child(iq, query);
    stanza -> xmpp_stanza = iq;
    
    // Send out the stanza
	err = _mio_send_nonblocking(conn, stanza);
    
    // Release the stanzas
	mio_stanza_free(stanza);
    
	mio_response_free(response);
	return 0;
}

void* publisher_thread(thread_args_t *thread_args)
{
    int pub_index = thread_args -> user_index;
    benchmark_t *setup = thread_args -> setup;
    
    char *time_str = NULL, *random_message = NULL, *event_node;
	mio_stanza_t *item;
    mio_conn_t *conn = publishers[pub_index];
    if (conn == NULL)
      printf("conn is null %d\n", pub_index);
    // generate random message
    random_message = malloc(sizeof(char)* 1000);
    memset(random_message,0x41,1000);
    random_message[999]='\0';
    int node_index;
    mio_response_t *response;
  while(TRUE)
  {
      for ( node_index = 0; node_index < setup -> publish_factor; node_index++)
      {
          event_node = publish_nodes[pub_index][node_index];
          time_str = mio_timestamp_create();
          item = mio_pubsub_item_data_new(conn);
          mio_item_transducer_value_add(item, "name", "100",
              random_message, random_message,time_str);
         
          response = mio_response_new();
          mio_item_publish(conn, item, event_node,response);
          free(time_str);
          free(response);
          mio_stanza_free(item);
          publish_count[pub_index][node_index]++;
      }
  }
  return (void*) NULL;
}

void print_benchmark(benchmark_t *setup)
{
    printf("User - %s", setup -> user);
    printf("Server - %s:%d", setup -> server,setup -> port);
    printf("N Publishers - %d\n", setup -> n_publishers);
    printf("N Subscribers - %d\n", setup -> n_subscribers);
    printf("Publish Factor - %d\n",setup -> publish_factor);
    printf("Subscribe Factor - %d\n",setup -> subscribe_factor);
    printf("Publish Size - %d B\n", setup -> publish_size);
    printf("Time - %d s\n", setup -> time);
    
}
void print_results(benchmark_t *setup)
{
    printf("benchmark completed\n");
    print_benchmark(setup);
    int user_index, node_index;
    double publish_average = 0;
    double subscribe_average = 0;
    int local_sum;
    for (user_index = 0; user_index < setup -> n_publishers; user_index++)
    {
      local_sum = 0;
      for (node_index = 0; node_index < setup -> publish_factor; node_index++)
      {
        local_sum += publish_count[user_index][node_index];
      }
      printf("Publisher %d published %d messaged\n", user_index, local_sum);
      publish_average += local_sum;
    }
    publish_average = publish_average / (((double) setup -> n_publishers)*(double) setup -> time);

    printf("\n\nPublish Average %f\n\n", publish_average);
    for (user_index = 0; user_index < setup -> n_subscribers; user_index++)
    {
      local_sum = 0;
      for (node_index = 0; node_index < setup -> subscribe_factor; node_index++)
      {
        local_sum += subscribe_count[user_index][node_index];
      }
      printf("Subscriberer %d received %d messages\n", user_index, local_sum);
      subscribe_average += local_sum;
    }
    subscribe_average = subscribe_average / (((double) setup -> n_subscribers) * (double) setup -> time);
    printf("\n\nSubscribe Average %f\n\n", subscribe_average);
}
int subscriber_thread(thread_args_t *thread_args)
{
  int sub_index = thread_args -> user_index;
  benchmark_t *setup = thread_args -> setup;
    char  *event_node;
    
    mio_conn_t *conn = subscribers[sub_index];
    if (conn == NULL)
      printf("sub conn is null\n");
    mio_data_t * mio_data = NULL;
    mio_response_t *response = NULL;
    mio_packet_t *mio_packet = NULL;
    int node_index;
    while (1) {

		response = mio_response_new();
		mio_pubsub_data_receive(conn, response);
		//This is a handy toowwwwwwl for printing response information
		// For the purpose of illustration, we will manually print the elements
		// mio_response_print(response);
		//Parse the response packet type
		switch (response->response_type) {
                //For commands that require an acknowledgment, it can be checked with MIO_RESPONSE_OK
            case MIO_RESPONSE_OK:
                break;
                
                //For errors, the code can be printed as described below
            case MIO_RESPONSE_ERROR:
              
                break;
                
                //If the response contains more complex data, it is encapsulated in a MIO_RESPONSE_PACKET
            case MIO_RESPONSE_PACKET:
                mio_packet = (mio_packet_t *) response->response;
                //Since there are multiple types of responses, you typically first switch on the type
                if (mio_packet->type == MIO_PACKET_DATA) {
                    //MIO_PACKET_DATA contains a list of transducer values that contain data
                    mio_data = (mio_data_t *) mio_packet-> payload;
                    event_node = mio_data->event;
                    //Traverse the linked list of sensor values
                    for (node_index = 0; node_index < setup -> subscribe_factor; node_index++ ) {
                        if (strcmp(event_node, subscribe_nodes[sub_index][node_index]) == 0)
                        {

                          subscribe_count[sub_index][node_index]++;
                          break;
                        }
                    }
                } else
                    printf("Unknown MIO response packet\n");
                break;
                
            default:
                printf("unknown response type\n");
                
		}
        
		mio_response_free(response);
	}
    return 0;
}

int main(int argc, char **argv)
{
  //parse command line arguments
  benchmark_t *setup = parse_args(argc,argv);

  // setup user connection
  mio_conn_t *conn = mio_conn_new();
  mio_connect(setup -> user, setup -> pass, MIO_LEVEL_DEBUG, NULL, NULL, conn);

  // - register new publishers
  // - connect new publishers
  // - register new publishers event nodes
  int user_index, error, node_index;

  char tmp_username[UUID_LENGTH];
  char tmp_password[UUID_LENGTH];
  char *tmp_node_id,*tmp_jid;
    mio_response_t *response = NULL;
  uuid_t tmp_uuid;
  srand(time(NULL));
  thread_args_t *thread_args;
  
  publishers = malloc(setup -> n_publishers * sizeof(mio_conn_t*));
  memset(publishers, 0x0, setup -> n_publishers * sizeof(mio_conn_t*));
  publish_nodes = (char***) malloc(setup -> n_publishers * sizeof(char**));
  publish_count = (int**) malloc(setup -> n_publishers *sizeof(int*));

  for (user_index = 0; user_index < setup -> n_publishers; user_index++)
  {
    uuid_generate(tmp_uuid);
    uuid_unparse(tmp_uuid,tmp_username);

    uuid_generate(tmp_uuid);
    uuid_unparse(tmp_uuid,tmp_password);

    error = register_user(conn, tmp_username,tmp_password);
    publishers[user_index] = mio_conn_new();
    if (error)
    {
      printf("error registering user %s \n", tmp_username);
    } else
    {
      tmp_jid = malloc(100);
      sprintf(tmp_jid,"%s@%s", tmp_username, setup -> server);
        mio_connect(tmp_jid, tmp_password, MIO_LEVEL_ERROR,
            NULL,NULL,publishers[user_index]);
    }

    publish_nodes[user_index] = malloc(setup -> publish_factor * sizeof(char*));
    for (node_index = 0; node_index < setup -> publish_factor; node_index++)
    {
      tmp_node_id = malloc(UUID_LENGTH * sizeof(char));
      uuid_generate(tmp_uuid);
      uuid_unparse(tmp_uuid,tmp_node_id);
      publish_nodes[user_index][node_index] = tmp_node_id; 
      response = mio_response_new();
      mio_node_create(publishers[user_index], tmp_node_id, "", NULL, response); 
      mio_response_free(response);
      tmp_node_id = NULL;
    }
    publish_count[user_index] = (int*) malloc(sizeof(int)*setup -> publish_factor);
    memset(publish_count[user_index],0x0,sizeof(int)*setup -> publish_factor);
    memset(tmp_username,0x0,UUID_LENGTH);
    memset(tmp_password,0x0,UUID_LENGTH);
  }

  // register new subscribers
  // connect new subscribers
 // randomly assign the nodes to subscribe to
 // initialize count matrix
  subscribers = malloc(setup -> n_subscribers * sizeof(mio_conn_t*));
  memset(subscribers, 0x0, setup -> n_subscribers * sizeof(mio_conn_t*));
  
  subscribe_nodes = (char***) malloc(setup -> n_subscribers * sizeof(char**));
  
  subscribe_count = (int**) malloc(setup -> n_subscribers *sizeof(int*));
  printf("%d\n", setup -> n_subscribers);
  for (user_index = 0; user_index < setup -> n_subscribers; user_index++)
  {
    uuid_generate(tmp_uuid);
    uuid_unparse(tmp_uuid,tmp_username);

    uuid_generate(tmp_uuid);
    uuid_unparse(tmp_uuid,tmp_password);

    subscribers[user_index] = mio_conn_new();
    error = register_user(conn, tmp_username,tmp_password);
    if (error)
    {
      printf("error registering user %s \n", tmp_username);
    } else
    {
      tmp_jid = malloc(100);
      sprintf(tmp_jid,"%s@%s", tmp_username,setup -> server);
        mio_connect(tmp_jid, tmp_password, MIO_LEVEL_ERROR,
            NULL,NULL,subscribers[user_index]);
    }
    subscribe_nodes[user_index] = malloc(setup -> subscribe_factor * sizeof(char*));
    int pub_index = (user_index * setup -> subscribe_factor)/(setup -> publish_factor) % setup -> n_publishers;
    int event_index = (user_index * setup -> subscribe_factor) % (setup -> publish_factor);
    for (node_index = 0; node_index < setup -> subscribe_factor; node_index++)
    { 
      printf("%d node, %d pub, %d event\n", node_index, pub_index, event_index);
      tmp_node_id = publish_nodes[pub_index][ event_index];
      subscribe_nodes[user_index][node_index] = tmp_node_id;
      
      response = mio_response_new();
      error = mio_subscribe(subscribers[user_index], tmp_node_id, response);
      
      mio_response_free(response);
      event_index++;
      pub_index += event_index / setup -> publish_factor;
      pub_index = pub_index % setup -> n_publishers;
      event_index = event_index % setup -> publish_factor;
    }
    subscribe_count[user_index] = (int*) malloc(sizeof(int)*setup -> subscribe_factor);
    memset(subscribe_count[user_index],0x0,sizeof(int)*setup -> subscribe_factor);
    memset(tmp_username,0x0,UUID_LENGTH);
    memset(tmp_password,0x0,UUID_LENGTH);
  } 
  
  publish_threads = (pthread_t*) malloc(setup -> n_publishers*sizeof(pthread_t));
  subscribe_threads = (pthread_t*) malloc(setup -> n_publishers*sizeof(pthread_t));

  // spawn process for each subscriber
  // spawn process for each publisher
  for (user_index = 0; user_index < setup -> n_subscribers; user_index++)
  {
      thread_args = (thread_args_t*) malloc(sizeof(thread_args_t));
      thread_args -> user_index = user_index;
      thread_args -> setup = setup;
      pthread_create(&subscribe_threads[user_index], NULL, (void*) &subscriber_thread, thread_args);
  }
  for (user_index = 0; user_index < setup -> n_publishers; user_index++)
  {
      thread_args = (thread_args_t*) malloc(sizeof(thread_args_t));
      thread_args -> user_index = user_index;
      thread_args -> setup = setup;
      pthread_create(&publish_threads[user_index], NULL, (void*) &publisher_thread, thread_args);
  }

  // have subscribers and publishers increment
  // kill all threads after proveided time
    long elapsed = 0;
    long last_time = time(NULL), time_tmp;
            
  while(elapsed < setup -> time)
  {
      sleep(setup -> time /10);
      time_tmp = time(NULL);
      elapsed += time_tmp - last_time;
      last_time = time_tmp;
  }
    
    for (user_index = 0; user_index < setup -> n_subscribers; user_index++)
    {
        pthread_cancel(subscribe_threads[user_index]);
    }
    for (user_index = 0; user_index < setup -> n_publishers; user_index++)
    {
        pthread_cancel(publish_threads[user_index]);
    }
    print_results(setup);
    return 0;
}
