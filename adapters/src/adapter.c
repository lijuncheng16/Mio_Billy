#if defined(ALL) || defined(ENFUSEADAPTER)
#include "libfuse_adapter.h"
#include <curl/curl.h>
#endif


#if defined(HUE)  || defined(ALL)
#include "libhue_adapter.h"
#include <curl/curl.h>
#endif

#if defined(ALL) || defined(MODBUSADAPTER)
#include "modbus_adapter.h"
#endif

#if defined(ALL) || defined(PUPADAPTER)
#include "pup_adapter.h"
#endif
#if defined(BACNETADAPTER)
#include "bacnet_adapter.h"
#endif
#include "adapter.h"
#include "xml_tools.h"

#include <string.h>
#include <unistd.h>
#include <mio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <common.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>

#define LOCAL_HOST "127.0.0.1"
#define ADAPTER_USAGE "./adapter -t [type] -c [configuration directory] -j [jid] -p [password (ooptional)] -l [log file] -a [actuate include to run actuate adapter] -d  [data include to run data adapter]\n\ttype- [enfuse,hue,bacnet,modbus,pup] -ipc [ipc_port]\n\t"

#define TRUE      1
#define FALSE     0


/** struct to hold command line arguments */
typedef struct
{
    char* jid; /** */
    char* password; /** */
    char* type; /** */
    char* config_dir; /** */
    char* log_file; /** */
    int actuate; /** */
    int data; /** */
    int ipc_port; /** */
} _adapter_args_t; /** */

static _adapter_args_t* _parse_adapter_args(int argc, char** args);
static void _initialize_context(adapter_t* adapter);


/** Takes command line arguments and parses them.
 * @param argc Command line argument count.
 * @param args Command line arguments.
 * @returns Initialized adapter_args_t on success, NULL on failure.
 */
static _adapter_args_t* _parse_adapter_args(int argc, char** args)
{
    int i;
    _adapter_args_t *adapter_args = (_adapter_args_t*) malloc(sizeof(_adapter_args_t));
    memset(adapter_args,0x0,sizeof(_adapter_args_t));
    if (argc == 1)
    {
        free(adapter_args);
        return NULL;
    }
    for (i = 1; i < argc; i++)
    {
        if (strcmp(args[i], "-p") == 0)
        {
            adapter_args -> password = args[i+1];
            i++;
        }  else if (strcmp(args[i], "-t") == 0)
        {
            adapter_args -> type = string_copy(args[i+1]);
            i++;
        } else if (strcmp(args[i], "-j") == 0)
        {
            adapter_args -> jid = args[i+1];
            i++;
        } else if (strcmp(args[i], "-c") == 0)
        {
            adapter_args -> config_dir = (args[i+1]);
            i++;
        } else if (strcmp(args[i], "-l") == 0)
        {
            adapter_args -> log_file =(args[i+1]);
            i++;
        } else if (strcmp(args[i],"-a")==0)
        {

            adapter_args -> actuate = 1;
        } else if (strcmp(args[i],"-d")==0)
        {
            adapter_args -> data = 1;
        } else if (strcmp(args[i], "-ipc") == 0)
        {
            adapter_args -> ipc_port = atoi(args[i+1]);
            i++;
        }
    }
    if ( (!adapter_args -> data && ! adapter_args -> actuate) ||
            (adapter_args -> jid == NULL) || (adapter_args -> password == NULL) ||
            (adapter_args -> config_dir == NULL))
    {
        free(adapter_args);
        return NULL;
    }
    return adapter_args;
}

/** Allocates memory and initializes that memory for an adapter
* @returns Allocated and initialized adapter_t struct.
*/
adapter_t* new_adapter()
{
    adapter_t* a = (adapter_t*) malloc(sizeof(adapter_t));
    memset(a,0x0,sizeof(adapter_t));
    return a;
}

/** Frees memor associated with adapter_t struct
* @param adapter Adapter struct to free.
*/
void release_adapter(adapter_t* adapter)
{
    if (adapter -> data)
    {
        pthread_cancel(*(adapter -> data_thread));
    }
    if (adapter -> actuate)
    {
        pthread_cancel(*(adapter -> actuate_thread));
    }
    if (adapter -> jid)
        free(adapter -> jid);
    if (adapter -> password)
        free(adapter -> password);
    if (adapter -> xmpp_server)
        free(adapter -> xmpp_server);
    if (adapter -> connection)
        mio_conn_free(adapter -> connection);
    if (adapter -> context_lock)
        pthread_mutex_destroy(adapter -> context_lock);
    free(adapter);
}

/** Sets adapter attributes to those specified by command line arguments.
* @param adapter Allocated adapter struct to initialize.
* @param args Initialized adapter_args struct.
*/
static void _initialize_adapter(adapter_t* adapter, _adapter_args_t* args)
{
    int err;
    struct sockaddr_in *ipc_addr = malloc(sizeof(struct sockaddr_in));
    int adapterfd, router_port = 0;
    uint8_t *buffer = malloc(100);
    int buffer_len = 100;
    /* Begin MIO XMPP connection or ipc connections*/
    if (args -> ipc_port)
    {
        adapter -> register_port = args -> ipc_port;

        /* initialize ipc_addr */
        memset(ipc_addr,0x0,sizeof(struct sockaddr_in));
        adapterfd = socket(AF_INET,SOCK_STREAM,0);
        inet_pton(AF_INET,LOCAL_HOST,&ipc_addr->sin_addr);

        //inet_aton(LOCAL_HOST, &(ipc_addr -> sin_addr));
        ipc_addr->sin_port = htons(args -> ipc_port);
        int mode = 0;
        ioctl(adapterfd, FIONBIO, &mode);
        err = connect(adapterfd, (struct sockaddr *) ipc_addr, sizeof(struct sockaddr_in));
        if (err == -1)
            printf("couldn't conenct \n");
        read(adapterfd,buffer,buffer_len);
        close(adapterfd);

        sscanf((char*) buffer, "%d", &router_port);
        ipc_addr -> sin_port = htons(router_port);
        printf("%d\n",router_port);

        adapterfd = socket(AF_INET,SOCK_STREAM,0);
        mode = 0;
        ioctl(adapterfd, FIONBIO, &mode);

        err = connect(adapterfd, (struct sockaddr*)ipc_addr, sizeof(struct sockaddr_in));
        if (err == -1)
            printf("couldn't conenct \n");
        adapter -> adapterfd = adapterfd;
        adapter -> ipc_addr = ipc_addr;

    } else
    {
        adapter -> jid = string_copy(args -> jid);
        if (args -> password != NULL)
        {
            adapter -> password = string_copy(args -> password);
        }

        if (adapter -> password == NULL)
            adapter -> password = getpass("MIO Password: ");
        adapter -> connection = mio_conn_new(MIO_LEVEL_ERROR);

        err = mio_connect(adapter -> jid, adapter -> password, NULL, NULL, adapter -> connection);
        if (err != MIO_OK)
        {
            fprintf(stderr, "Could not connect to MIO\n");
            return;
        }
    }

    if (args -> type != NULL)
        adapter -> type = string_copy(args -> type);
    adapter -> config_dir = string_copy(args -> config_dir);
    /* Based on adapter type set other pieces.*/
#if defined(ALL) || defined(ENFUSEADAPTER)
#if defined(ENFUSEADAPTER)
    curl_global_init(CURL_GLOBAL_ALL);
    adapter -> data = libfuse_data;
    adapter -> parser = libfuse_parser;
#else
    if ((strcmp(adapter -> type,ENFUSENAME)==0))
    {
        curl_global_init(CURL_GLOBAL_ALL);
        /* no actuation capability on enfuse device */
        adapter -> data = libfuse_data;
        adapter -> parser = libfuse_parser;
    }
#endif
#endif

#if defined(ALL) || defined(MODBUSADAPTER)
#if defined(MODBUSADAPTER)

    if (args -> data)
        adapter -> data = modbus_data;
    if (args -> actuate)
        adapter -> actuate = modbus_actuate;
    adapter -> parser = modbus_parser;
#else
    if (strcmp(adapter -> type,MODBUSNAME) == 0)
    {
        if (args -> data)
            adapter -> data = modbus_data;
        if (args -> actuate)
            adapter -> actuate = modbus_actuate;
        adapter -> parser = modbus_parser;
    }
#endif
#endif

#if defined(ALL) || defined(PUPADAPTER)
#if defined(PUPADAPTER)
    if (args -> actuate)
        adapter -> actuate = pup_actuate;
    if (args -> data)
        adapter -> data = pup_data;
    adapter -> parser = pup_parser;
#else
    if (strcmp(adapter -> type,PUPNAME)==0)
    {
        if (args -> actuate)
            adapter -> actuate = pup_actuate;
        if (args -> data)
            adapter -> data = pup_data;
        adapter -> parser = pup_parser;
    }
#endif
#endif

#if defined(ALL) || defined(HUE)
#if defined(HUE)
    if (args -> actuate)
        adapter -> actuate = hue_actuate;
    if (args -> data)
        adapter -> data = hue_data;
    adapter -> parser = hue_parser;
#else
    if (strcmp(adapter -> type,HUENAME)==0)
    {
        if (args -> actuate)
            adapter -> actuate = hue_actuate;
        if (args -> data)
            adapter -> data = hue_data;
        adapter -> parser = hue_parser;
    }
#endif
#endif

#if defined(ALL) || defined(BACNETADAPTER)
#if defined(BACNETADAPTER)
    if (args -> actuate)
        adapter -> actuate = bacnet_actuate;
    if (args -> data)
        adapter -> data = bacnet_data;
    adapter -> parser = bacnet_parser;
#else

    if (strcmp(adapter -> type,BACNETNAME)==0)
    {
        if (args -> actuate)
            adapter -> actuate = bacnet_actuate;
        if (args -> data)
            adapter -> data = bacnet_data;
        adapter -> parser = bacnet_parser;
    }
#endif
#endif
}

/** Calls the parser function of adapter and then begins the actuate and data threads.
 * @param Initialized adapter struct.
 */
void _initialize_context(adapter_t* adapter)
{
    int err;
    adapter -> context = (void*) (adapter -> parser) (adapter);
    if (adapter -> context != NULL)
    {
        if (adapter -> actuate)
        {
            adapter -> actuate_thread = malloc(sizeof(pthread_t));
            err = pthread_create(adapter -> actuate_thread,NULL, adapter -> actuate,
                                 (void*) adapter);
            if (err)
            {
                printf("could not begin data thread\n");
            }
        }
        if (adapter -> data)
        {
            adapter -> data_thread = malloc(sizeof(pthread_t));
            err = pthread_create(adapter -> data_thread, NULL, adapter -> data,
                                 (void*) adapter);
            if (err)
            {
                printf("could not begin data thread\n");
            }
        }
    }
}

int main(int argc ,char** args)
{
    _adapter_args_t* adapter_args = _parse_adapter_args(argc,args);

    if (adapter_args == NULL)
    {
        printf(ADAPTER_USAGE);
        return 1;
    }
    adapter_t* adapter = new_adapter();

    _initialize_adapter(adapter,adapter_args);
    _initialize_context(adapter);

    /* wait for 'q' to exit program */
    while(TRUE)
    {
        sleep(60);
    }
    release_adapter(adapter);
    return 0;
}
