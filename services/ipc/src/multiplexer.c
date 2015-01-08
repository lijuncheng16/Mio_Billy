#include <uthash.h>
#include <mio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include "multiplexer.h"

#define REGISTER_QUEUE_LENGTH 100
#define LOCAL_HOST            "127.0.0.1"
#define BUFFER_SIZE           1024


typedef struct {
    multiplexer_context_t *context;
    int fd;
} context_port_t;


/****************************
  local function definitions
  *************************/
static void* _register_thread(void* vcontext);
// static void _multiplexer_setup(char* jid, char* password, char *config_path);
static void _distribute_message(multiplexer_context_t *context, char* message, int mio);
static int _fd_setup(char* address, int port);
static void* _mio_handler_read(void* vcontext);
static void* _mio_handler_write(void* vcontext);
static void* _route_thread(void* vcontext);
static void _add_multiplexer_route(multiplexer_context_t *context, int clientfd);
static void* _buffered_read(int sockfd);

/* end local function definitions */

/****************************
  struct creation & freeing functions
  *************************/

/** Allocates memory for a multiplexer map element. Initializes
 * mutex for the associated socket as well */
multiplexer_map_t* new_multiplexer_map()
{
    multiplexer_map_t* map = malloc(sizeof(multiplexer_map_t));
    memset(map,0x0,sizeof(multiplexer_map_t));
    map -> mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(map -> mutex, NULL);
    return map;
}

/** Allocates memory for mio info struct*/
mio_info_t* new_mio_info()
{
    mio_info_t* info = malloc(sizeof(mio_info_t));
    memset(info, 0x0, sizeof(mio_info_t));
    info -> mio = mio_conn_new(MIO_LEVEL_ERROR);

    info -> mio_thread_r = malloc(sizeof(pthread_t));
    memset(info -> mio_thread_r,0x0,sizeof(pthread_t));
    info -> mio_thread_w = malloc(sizeof(pthread_t));
    memset(info ->mio_thread_w,0x0,sizeof(pthread_t));
    info -> mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(info -> mutex, NULL);
    return info;
}

/** Allocates memory for multiplexer struct*/
multiplexer_t* new_multiplexer()
{
    multiplexer_t* multiplexer = malloc(sizeof(multiplexer_t));
    memset(multiplexer, 0x0, sizeof(multiplexer_t));
    multiplexer -> mutex = malloc(sizeof(pthread_mutex_t));
    memset(multiplexer -> mutex, 0x0, sizeof(pthread_mutex_t));
    pthread_mutex_init(multiplexer -> mutex, NULL);
    return multiplexer;
}

/** Allocates memory for multiplex register struct*/
multiplex_register_t* new_multiplex_register()
{
    multiplex_register_t*  reg = malloc(sizeof(multiplex_register_t));
    memset(reg,0x0,sizeof(multiplexer_t));
    reg -> register_thread = malloc(sizeof(pthread_t));
    memset(reg -> register_thread, 0x0, sizeof(pthread_t));
    reg -> register_mutex = malloc(sizeof(pthread_mutex_t));
    memset(reg -> register_mutex,0x0,sizeof(pthread_mutex_t));
    pthread_mutex_init(reg -> register_mutex, NULL);
    return reg;
}

/** Allocates memory for mio info struct*/
multiplexer_context_t* _new_multiplexer_context()
{
    multiplexer_context_t *context;
    context = malloc(sizeof(multiplexer_context_t));
    memset(context, 0x0, sizeof(multiplexer_context_t));
    context -> mio_info = new_mio_info();
    context -> multiplexer = new_multiplexer();
    context -> register_info = new_multiplex_register();
    return context;
}
/* end struct creation freeing functions */


/*************************
Multiplexer Setup and Takedown
*************************/

/** Initializes multiplexer and associated threads.
* Spawns mio_connection thread. A registration thread
*
* param jid The Jabberd Unique Identifier of user
* param password The password associated with jid.
* param port The port the ipc registration thread should listen on.
* returns Initialized
*/
multiplexer_context_t* multiplexer_setup(char* jid, char* password, int port)
{
    int error;
    multiplexer_context_t *context = _new_multiplexer_context();
    if (jid == NULL)
    {
        printf("IPC ERROR: Null jid");
        return NULL;
    }
    if (password == NULL)
    {
        password = getpass("MIO password: ");
    }

    error = mio_connect(jid, password, NULL, NULL,
                context -> mio_info -> mio);
    if (error != MIO_OK)
    {   
        printf("IPC ERROR: Could not connect to Mio server\n");
        return NULL;
    }
    sleep(1);
    /* copy mio info over */
    context -> mio_info -> jid = malloc(strlen(jid)+1);
    strcpy(context -> mio_info -> jid, jid);
    context -> mio_info -> password = malloc(strlen(password));
    strcpy(context -> mio_info -> password, password);
    
    context -> mio_info -> miofd = _fd_setup(LOCAL_HOST,0);
    printf("IPC status: MIO context setup\n");

    pthread_create(context -> mio_info -> mio_thread_r,NULL,_mio_handler_read,
                  context);
    pthread_create(context -> mio_info -> mio_thread_w,NULL,_mio_handler_write,
                   context);
    printf("IPC status: MIO threads started\n");
    /*begin registration thread*/
    context -> register_info -> register_port = port;
    pthread_create(context -> register_info -> register_thread, NULL, _register_thread,context);
    printf("IPC status: Register thread started\n");

    return context;
}

void multiplexer_stop(multiplexer_context_t *context)
{

}

/* END of multiplexer setup */

/*************************
Local Function Definitions
*************************/

/******* ADD to multiplexer route *********************/
void _add_multiplexer_route(multiplexer_context_t* context, int clientfd)
{
    multiplex_register_t *register_info = context -> register_info;
    multiplexer_t *multiplexer = context -> multiplexer;

    /* lock register struct so no changes to it can be made */
    int new_n,n;
    int error;
    int* registered_sockets_tmp;
    char port_buffer[20];
    struct sockaddr_in **addresses_tmp;
    struct sockaddr_in * handler_addr;
    pthread_t ** thread_tmp;
    pthread_mutex_t **socket_mutex_tmp;
    int handlerfd;
    multiplexer_map_t * map_element;

    socklen_t slen;
    pthread_mutex_lock(register_info -> register_mutex);
    /* generate another socket to listen for publishes */
    handlerfd = _fd_setup(LOCAL_HOST,0);
    handler_addr = malloc(sizeof(struct sockaddr_in));
    slen = sizeof(*handler_addr);
    getsockname(handlerfd, (struct sockaddr *) handler_addr, &slen);

    /* update registered sockets_info */
    new_n = register_info -> n_registered_sockets + 1;

    /* update socket addresses for each socket file descriptor */
    addresses_tmp = malloc(new_n * sizeof(struct sockaddr_in *));
    memcpy(addresses_tmp, register_info -> sock_addresses, register_info -> n_registered_sockets);
    free(register_info -> sock_addresses);
    addresses_tmp[new_n-1] = handler_addr;
    register_info -> sock_addresses = addresses_tmp;

    n = sprintf(port_buffer,"%d",ntohs(handler_addr->sin_port));
    printf("registered on %d\n", ntohs(handler_addr -> sin_port));
    write(clientfd, port_buffer, n);
    close(clientfd);

    /* get handle for client */
    if ((clientfd = accept(handlerfd, (struct sockaddr*)NULL, NULL)) == -1)
    {
        error = errno;
        fprintf(stderr,"IPC Multiplexer: register not accepted, %d\n",error);
        return;
    }
    /* update registered sockets_info */
    registered_sockets_tmp = malloc(new_n * sizeof(int));
    
    memcpy(registered_sockets_tmp, register_info -> registered_sockets, (new_n-1) * sizeof(int));
    registered_sockets_tmp[new_n-1] = handlerfd;
    if (register_info -> registered_sockets)
        free(register_info -> registered_sockets);
    register_info -> registered_sockets = registered_sockets_tmp;

    /* allocate memory for new route_threads */
    thread_tmp = malloc(new_n * sizeof(pthread_t *));
    memcpy(thread_tmp,register_info -> route_threads, register_info -> n_registered_sockets);
    thread_tmp[new_n - 1] = malloc(sizeof(pthread_t));
    context_port_t *cp = malloc(sizeof(context_port_t));
    cp -> context = context;
    cp -> fd = clientfd;
    pthread_create(thread_tmp[new_n - 1], NULL, _route_thread,cp);
    free(register_info -> route_threads);
    register_info -> route_threads = thread_tmp;

    /* allocate memory for new socket_mutex */
    socket_mutex_tmp = malloc(new_n * sizeof(pthread_mutex_t));
    memcpy(socket_mutex_tmp, register_info -> socket_mutex, (new_n-1)*sizeof(pthread_mutex_t));
    socket_mutex_tmp[new_n-1] = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(socket_mutex_tmp[new_n-1],NULL);
    if (register_info -> socket_mutex != NULL)
        free(register_info -> socket_mutex);
    register_info -> socket_mutex = socket_mutex_tmp;

    map_element = malloc(sizeof(multiplexer_map_t));
    memset(map_element,0x0, sizeof(multiplexer_map_t));

    map_element -> next = multiplexer -> all_list;
    map_element -> sockfd = malloc(sizeof(int));
    *(map_element -> sockfd) = handlerfd;

    multiplexer -> all_list = map_element;
    /* unlock register setup and send client port to bind to */
    pthread_mutex_unlock(context -> register_info -> register_mutex);
}

/* END add to multiplexer */

/** Initializes a socket for the register thread to use.
 * @param address
 * @param port
 * @returns
 */
static int _fd_setup(char* address, int port)
{
    int registerfd = 0, error;
    struct sockaddr_in register_addr;
    int mode = 1;

    /* create socket file descriptor for register thread */
    registerfd = socket(AF_INET, SOCK_STREAM, 0);

    /* register socket address information */
    memset(&register_addr, 0x0, sizeof(register_addr));

    register_addr.sin_family = AF_INET;
    inet_pton(AF_INET,LOCAL_HOST,&register_addr.sin_addr);
    //error = inet_aton(LOCAL_HOST, &(register_addr.sin_addr));
    register_addr.sin_port = htons(port);

    if (bind(registerfd, (struct sockaddr *) &register_addr,
             sizeof(struct sockaddr_in)) < 0)
    {
        error = errno;
        fprintf(stderr,"IPC Multiplexer: error \n");
        return -1;
    }

    if (listen(registerfd, REGISTER_QUEUE_LENGTH) < 0)
    {
        error = errno;
        fprintf(stderr,"listening to port unsucessful %d\n", error);
    } 
    //ioctl(registerfd, FIONBIO, &mode); 

    return registerfd;
}


/** Thread that listens on port
 * @param context
 * @param port
 */
static void* _register_thread(void* vcontext)
{
    multiplexer_context_t *context = (multiplexer_context_t*) vcontext;
    int registerfd = 0;
    int clientfd = 0;
    int error = 0;
    char port_buffer[20];
    /* generate fd for register */
    registerfd = _fd_setup(LOCAL_HOST, context -> register_info -> register_port);
    printf("IPC status: register port setup\n");

    /* run register thread forever */
    while(1)
    {
        /* wait for another client to request access to multiplexer */
        if (registerfd <= 0)
        {
            printf("Could not setup register port \n");
            sleep(1);
            continue;
        }
        if ((clientfd = accept(registerfd, (struct sockaddr*)NULL, NULL)) == -1)
        {
            error = errno;
            fprintf(stderr,"IPC Multiplexer: Register not accepted, %d\n",error);
            continue;
        }
        printf("recieved accept\n");
        _add_multiplexer_route(context,clientfd);
        fprintf(stdout, "IPC Multiplexer : spawned thread %s\n",port_buffer);
        //registerfd = _fd_setup(LOCAL_HOST, context -> register_info -> register_port);

    }
    return NULL;
}

/** Thread that handles incoming messages from mio
 * @param context
 * @param mio_sock
 */
static void* _mio_handler_write(void* vcontext)
{
    multiplexer_context_t *context = (multiplexer_context_t*) vcontext;
    char* message = NULL;
    size_t message_len = 0;

    mio_response_t *response;
    while(1)
    {
        response = mio_response_new();
        //printf("waiting for xmpp message\n");
        mio_pubsub_data_receive(context -> mio_info -> mio, response);
        if (response -> stanza == NULL)
        {
            mio_response_free(response);
            printf("IPC ERROR: data receive stanza null\n");

            continue;
        }
        //mio_stanza_to_text(response -> stanza, &message, &message_len);
        xmpp_stanza_to_text(response -> stanza -> xmpp_stanza, &message, &message_len);
       
        // receive message
        printf("Distributing\n%s\n",message);
        _distribute_message(context,message,0);
        mio_response_free(response);
        if (message_len != 0)
            free(message);
    }
    return NULL;
}

/** Thread that handles outgoing messages to mio and incoming messages from
 * @param context
 * @param mio_sock
 */
static void* _mio_handler_read(void* vcontext)
{
    multiplexer_context_t * context = vcontext;
    int fd = context -> mio_info -> miofd;
    char *stanza_response,*event_response ;
    mio_response_t *response;
    mio_stanza_t *stanza = NULL;
    mio_info_t *mio_info = context -> mio_info;

    while (mio_info -> mio -> xmpp_conn -> state != XMPP_STATE_CONNECTED)
    {
        usleep(1000);
    }

    mio_parser_t *parser = mio_parser_new(mio_info -> mio);
    char* publish_request;

    while(1)
    {
        /* read in buffer */
        publish_request = _buffered_read(fd);
        if (publish_request == NULL)
        {
            sleep(1);
            continue;
        }
        printf("got a publish request\n");
        stanza_response = malloc(strlen(publish_request));
        event_response = malloc(500);
        sscanf(publish_request,"%s,%s",event_response,publish_request);
        stanza = mio_parse(parser, (char*) publish_request);

        /* Publish stanza to mio */
        response = mio_response_new();
        while (mio_info -> mio -> xmpp_conn -> state != XMPP_STATE_CONNECTED)
            usleep(1000);
        mio_item_publish_data(mio_info -> mio,stanza,event_response,response);
        mio_response_print(response);
        mio_response_free(response);
        mio_stanza_free(stanza);
        mio_response_free(response);
        free(stanza_response);
        free(publish_request);
    }
    return NULL;
}

/** Thread that handles incoming XMPP messages
 * @param context
 * @param mio_sock  
 */
static void* _route_thread(void* vcontext)
{   context_port_t* cp = vcontext;
    multiplexer_context_t *context = cp -> context;
    int clientfd = cp -> fd;
    void *message = NULL;
    while(1)
    {
        /* read in stanza */
        printf("waiting for publish stanza\n");
        message = _buffered_read(clientfd);
        if (message == NULL)
        {
            sleep(1);
            continue;
        }
        printf("got publish stanza %s\n", message);
        _distribute_message(context, (char*) message, 1);
        close(clientfd);
    }
    return NULL;
}

/** performs logical routing for mio_handler and route_handlers
 * @param context
 * @param stanza
 */
static void _distribute_message(multiplexer_context_t *context, char* stanza, int mio)
{
    //printf("distributing message\n");
    //mio_response_t *response = NULL;
    int write_len;
    int error;

    // write stanza to the mio_listener
    if (mio)
    {
        pthread_mutex_lock(context -> mio_info -> mutex);
        if ((write_len = write(context -> mio_info -> miofd, stanza, sizeof(stanza))) == 0)
        {
            error = errno;
            printf("Error writing to mio listener, failed with %d errror.",error);
        }
        pthread_mutex_unlock(context -> mio_info -> mutex);
    }
}

static void* _buffered_read(int sockfd)
{
    uint8_t *buffer;
    uint8_t *read_buffer = NULL;
    int buffer_size = BUFFER_SIZE, read_size = 0,n;
    buffer = malloc(buffer_size);
    while (1)
    {
        n = read(sockfd,buffer, buffer_size);
        if (n == -1)
        {
            //printf("%d read returned -1\n", sockfd);
            return NULL;
        }
        printf("buffered read: %s\n",buffer);
        /* append buffer to read_buffer */
        if (read_buffer != NULL)
            read_buffer = realloc(read_buffer,read_size + n);
        else 
            read_buffer = malloc(n);
        memcpy(&read_buffer[read_size], buffer, n);
        
        read_size += n;
        if ( n < BUFFER_SIZE)
        {
            break;
        }

    }
    return read_buffer;
}