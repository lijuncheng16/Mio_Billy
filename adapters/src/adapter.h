#ifndef ADAPTER_H
#define ADAPTER_H

#include "mio.h"

/** Defines for supported adapter types */
#define ENFUSENAME "enfuse"
#define HUENAME "hue"
#define BACNETNAME "bacnet"
#define MODBUSNAME "modbus"
#define PUPNAME "pup"

/** A struct used to hold general adapter information.
 */
typedef struct adapter{
    char* jid; /** Jabber unique ID */
    char* password; /** Password for jid */
    char* xmpp_server;  /** Domain of MIO XMPP server */
    int xmpp_port; /** Port the MIO XMPP listens on */
    int polling_rate; /**  */
    mio_conn_t* connection;

    int adapterfd;
    int register_port;
    struct sockaddr_in *ipc_addr;
    
    char* type;
    void* context;
    pthread_mutex_t* context_lock;
    pthread_t *data_thread;
    pthread_t *actuate_thread;
    char* config_dir;
    void* (*actuate) (void *);
    void* (*data) (void *);
    void* (*parser) (void *);
}adapter_t;

#endif
