#include "mio.h"
#include "mio_handlers.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>

#define USAGE "./adapter_dispatcher username password event_node_id"


typedef struct ap{
  char*id;
  int pid;
  struct ap *next;
} adapter_process_t;

int begin_adapter( mio_conn_t *conn, char* adapter_id)
{
  mio_response_t *adapter_response;
  mio_response_t *reference_response;
  mio_meta_t *adapter_meta = NULL;
  char *adapter_name;
  //mio_reference_t *adapter_ref;
  mio_packet_t *packet;
  int pid = 0;
  int err;
  char command[1024];
  

  adapter_response = mio_response_new();
  err = mio_meta_query(conn, adapter_id, adapter_response);
  if (err != MIO_OK)
  {
    printf("COuld not query node meta data\n");
    return 1;
  }
  packet = (mio_packet_t*) adapter_response -> response;

  switch(adapter_response -> response_type)
  {
    case MIO_RESPONSE_PACKET:
      if (packet -> type == MIO_PACKET_META)
      {
        adapter_meta = (mio_meta_t*) packet -> payload;
      }
      break;

    default:
      if (packet -> type == MIO_PACKET_META)
      {
        adapter_meta = (mio_meta_t*) packet -> payload;
      }
      break;
  }

  if (adapter_meta == NULL)
  {
    mio_response_free(adapter_response);
    return 0;
  }

  adapter_name = adapter_meta -> name;
  
  reference_response = mio_response_new();
  err = mio_references_query(conn, adapter_id,reference_response);

  if (err != MIO_OK)
  {
     printf("could not query references\n");
     return 1;
  }

  if (strcmp(adapter_name, "hue") == 0)
  {
    // check to see if references are set
    // if not perform autodiscovery
    packet = (mio_packet_t*) reference_response -> response;
    if ( packet -> payload == NULL)
    {

      sprintf(command, "hue_discover %s %s %s",
          conn -> xmpp_conn -> jid, conn -> xmpp_conn -> pass, adapter_id );
      pid = system(command);
      if (pid != WIFEXITED(pid))
      {
        printf("Could not open hue_discover process. Return status %d\n",pid); 
      }
      
    }
    // start hue adapter
    sprintf(command, "screen -dms ping bash -c \"hue -j %s -p %s -t hue -c %s -d 1 -a 1 \"",
          conn -> xmpp_conn -> jid, conn -> xmpp_conn -> pass, adapter_id );
    pid = system(command);
    if (pid != WIFEXITED(pid))
    {
      printf("Could not open hue_discover process. Return status %d\n",pid); 
    }
  }
  mio_response_free(reference_response);
  mio_response_free(adapter_response);
  return pid;
}
int main( int argc, char** argv)
{
  char *user;
  char *password;
  char *adapter_node;
  mio_conn_t *conn;
  mio_response_t *response;
  mio_packet_t *packet;
  int err;
  mio_reference_t *adapter_ref;
  mio_reference_t *tmp_ref;
  mio_reference_t *iter_ref;
  mio_reference_t *iter_ref2;
  adapter_process_t *adapter_proc = NULL;
  adapter_process_t *tmp_proc = NULL;
  

  if (argc != 4) 
  {
    printf(USAGE);
    return 1;
  }

  user = argv[1];
  password = argv[2];
  adapter_node = argv[3];

  conn = mio_conn_new(MIO_LEVEL_DEBUG);
  err = mio_connect(user, password, NULL, NULL, conn);
  if (err != MIO_OK)
  {
    printf("Could not connect to MIO.\n");
    return 1;
  }

  // determine what adapters exist
  

  // listen for new adapters
  while (1)
  {
    response = mio_response_new();
    mio_pubsub_data_receive(conn,response);
    switch(response -> response_type)
    {
      case MIO_RESPONSE_PACKET:
        packet = (mio_packet_t*) response -> response;
        if (packet -> type == MIO_PACKET_REFERENCES)
          break;
        if (strcmp(adapter_node, packet -> id) != 0)
          break;
        tmp_ref = (mio_reference_t*) packet -> payload;

        // find discrepancies
        // first look for new adapters
        for (iter_ref = tmp_ref; iter_ref != NULL; iter_ref = iter_ref -> next)
        {
          if (iter_ref -> type != MIO_REFERENCE_CHILD)
            continue;

          for(iter_ref2 = adapter_ref; iter_ref2 != NULL; iter_ref2 = iter_ref2 -> next) 
          {
            if((iter_ref2 -> type == MIO_REFERENCE_CHILD) && (strcmp(iter_ref -> node, tmp_ref -> node) == 0))
            {
              break;
            }
          }
          if (iter_ref2 == NULL)
          {
            if (adapter_proc == NULL)
            {
              adapter_proc = malloc(sizeof(adapter_process_t));
              memset(adapter_proc,0x0,sizeof(adapter_process_t));

            } else
            {
              tmp_proc = malloc(sizeof(adapter_process_t));
              memset(tmp_proc,0x0,sizeof(adapter_process_t));
              tmp_proc -> next = adapter_proc;
              adapter_proc = tmp_proc;
            }
            adapter_proc -> id = iter_ref -> node; 

            adapter_proc -> pid = begin_adapter(conn,iter_ref -> node);
          }
          break;
          default:
              printf("issue with meta packet response\n");
              return 1;
        }
        // look for adapters that were removed
        /*
        for (iter_ref = iter_ref; iter_ref != NULL; iter_ref = iter_ref -> next)
        {
          if (iter_ref -> type != MIO_REFERENCE_CHILD)
            continue;

          for(iter_ref2 = adapter_ref; iter_ref2 != NULL; iter_ref2 = iter_ref2 -> next) 
          {
            if((iter_ref2 -> type == MIO_REFERENCE_CHILD) && (strcmp(iter_ref -> id, iter_ref)))
            {
              break;
            }
          }
          if (iter_ref2 == NULL)
          {
            begin_adapter(iter_ref);
          }
        }*/
        mio_reference_free(adapter_ref);
        adapter_ref = tmp_ref;
        break;
    }
    mio_response_free(response);

  }

  return 0;
}
