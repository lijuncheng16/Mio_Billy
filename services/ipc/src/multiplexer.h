#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include <uthash.h>
#include <mio.h>

/****************************
  struct defintions
  *************************/

/** HASH Map element struct.
 * maps event_node, item to socket_id
 * used for writes.
 */
typedef struct multiplexer_map{
    char *event_node; /** Event node id (hash key) */
    char *item;       /** Item published to (hash key) */
    int *sockfd;     /** List of destination socket_ids*/
    pthread_mutex_t* mutex;
    UT_hash_handle event_map;
    UT_hash_handle item_map;
    struct multiplexer_map * next; /** Linked list for non_map */
} multiplexer_map_t;

/** Holds credentials of the mio_connection.
 * along with the mio_conn_t struct*/
typedef struct {
    char *jid;                /** Unique jabber identifier of user*/
    char *password;           /** Password associated with jid.*/
    mio_conn_t *mio;          /** MIO xmpp conneciton to server */
    pthread_t *mio_thread_r;
    pthread_t *mio_thread_w;
    int miofd;
    int mioport;
    pthread_mutex_t *mutex;
} mio_info_t;

/** Holds multiplexer route information */
typedef struct {
    multiplexer_map_t *all_list;
    multiplexer_map_t *event_map; /** map from event_node -> multiplexer_map_t*/
    multiplexer_map_t *item_map;  /** map from item -> multiplexer_map_t*/
    pthread_mutex_t *mutex;
} multiplexer_t;

/** A list of registered multiplexer sockets and the threads that manage them
 of threads and. Each index i corresponds  */
typedef struct {
    int n_registered_sockets;
    int *registered_sockets; /** list of sockets registered to */
    struct sockaddr_in **sock_addresses;
    pthread_t ** route_threads;
    pthread_t *register_thread;
    int register_port;
    pthread_mutex_t **socket_mutex; /** List of mutex for each socket for writing to each socket */
    pthread_mutex_t *register_mutex;
} multiplex_register_t;

/**  Contains the threads and configuration the multiplexer manages*/
typedef struct {
    mio_info_t* mio_info;
    multiplexer_t* multiplexer;
    multiplex_register_t *register_info;
} multiplexer_context_t;

/* END struct definitions */


multiplexer_context_t* multiplexer_setup(char* jid, char* password, int port);
void multiplexer_stop(multiplexer_context_t *context);

char* receive_message(int fd);
#endif