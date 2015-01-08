#define PRINT_ENABLED 1
#include "uthash.h"
#include "adapter.h"
#include "pup_adapter.h"
#include "libpup.h"
#include <mio_handlers.h>
#include <mio.h>
#include <strophe.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include "xml_tools.h"

#ifdef DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif
#define UPDATE_PERIOD 10

#ifndef TRUE
#define TRUE        1
#endif
#ifndef FALSE
#define FALSE       0
#endif
#define DATA_SIZE   20

typedef struct mio_info
{
    mio_conn_t* pubsub_conn;
    char *jid_username;
    char *jid_password;
    char *xmpp_server;
    uint32_t xmpp_port;
    char *pubsub_server;
    uint32_t polling_rate;
    char* location;
} mio_info_t;

typedef struct pup_adapter
{
    mio_info_t* info;
    pup_context_t *context;
} pup_adapter_t;



pup_adapter_t* new_pup_adapter(uint16_t addr, char* serial_file, pup_device_t *devices, char* username, char* password);
mio_info_t* new_mio_info(char* username, char* password);
pup_device_t* parse_pup_devices(mio_conn_t *connection, char* event_id);

void free_pup_adapter(pup_adapter_t*adapter);
void free_mio_info(mio_info_t* info);

void connect_mio_info(mio_info_t *info);

void device_update(pup_adapter_t* adapter);
void actuate_listener(pup_adapter_t* adapter);

void token_passer(pup_adapter_t* adapter);

pup_adapter_t* parse_pup_node(adapter_t *adapter);
pup_device_t* parse_pup_devices(mio_conn_t *connection, char* event_id);

static int32_t update_device(pup_adapter_t *adapter, pup_device_t *device);
static int32_t read_device(pup_context_t *context, pup_device_t *device);

pup_context_t* parse_pup_context(mio_conn_t *connection, char* event_id) ;

// meant to be a threaded function. this functon listens to sensor andrew
//        actuation requests to actuate the devices.
void* pup_actuate(void* vadapter)
{
    pup_adapter_t *adapter = (pup_adapter_t*) vadapter;
    // declare temporary variable pointers
    pup_device_t *device;
    pup_attribute_t * attribute;
    pup_channel_t * channel;
    mio_conn_t* pubsub_conn = adapter -> info -> pubsub_conn;
    mio_response_t *mio_response;
    mio_packet_t *mio_packet;
    mio_data_t *mio_data;
    mio_transducer_value_t *mio_tran;

    // continue listening for actuation requests forever
    while (TRUE) {
        if (pubsub_conn -> xmpp_conn -> state == XMPP_STATE_CONNECTED) {
            mio_response = mio_response_new();
            // receive mio data_packet
            mio_pubsub_data_receive(pubsub_conn, mio_response);
            mio_packet = (mio_packet_t*) mio_response->response;
            mio_data = (mio_data_t*) mio_packet->payload;
            // iterate over transducer data in mio_data packet
            for (mio_tran = mio_data->transducers; mio_tran != NULL; mio_tran = mio_tran->next)
            {
                // ignor non transducer commands
                if (mio_tran -> type != MIO_TRANSDUCER_SET_VALUE)
                    continue;
                // check if event_node is present
                for (device = adapter -> context -> devices; device; device = device -> hh.next)
                {
                    if (strcmp(device -> node_id, mio_data->event) !=0)
                        continue;
                    for (channel = device -> channels; channel; channel = channel-> hh.next)
                        for (attribute = channel -> attributes; attribute; attribute = attribute-> hh.next)
                        {
                            // if present propogate change
                            if (attribute -> update == mio_tran -> id)
                            {
                                pthread_mutex_lock(adapter -> context -> serial_mutex);
                                attribute -> value = (uint8_t*) malloc(sizeof(int32_t));
                                *((int32_t*)attribute -> value) = atoi(mio_tran->typed_value);
                                write_attribute(adapter -> context, device -> addr, channel -> channel, attribute);
                                pthread_mutex_unlock(adapter -> context ->serial_mutex);
                            }
                        }
                }

            }
        } else usleep(1000);
    }

}

// polls device for attribute values, meant to be run in a thread
// updates every UPDATE_PERIOD
void* pup_data(void* vadapter)
{
    pup_adapter_t* adapter = (pup_adapter_t*) vadapter;
    int32_t err,t;
    pup_context_t* context = adapter -> context;
    pup_device_t* device;
    while (TRUE)
    {
        pthread_mutex_lock(context -> serial_mutex);
        // iterate through devices and read all attributes from device
        //      push these values to sensor andrew
        for (device = context -> devices; device != NULL; device = device -> hh.next)
        {
            err = read_device(context,device);
            if (err)
            {
                if (DEBUG)
                    printf("error reading from device\n");
                continue;
            }
            err = update_device(adapter, device);
            if (err)
            {
                printf("error updating channel\n");
                continue;
            }
        }
        pthread_mutex_unlock(context -> serial_mutex);
        t = UPDATE_PERIOD;
        while (t)
        {
            t = sleep(t);
            if (t < 0)
                break;
        }
    }
    pthread_exit(NULL);
}

// reads all attributes in pup device device in context
// returns 0 on success, -1 on failure
static int32_t read_device(pup_context_t *context, pup_device_t *device)
{
    pup_channel_t *channel;
    pup_attribute_t *attribute;
    int32_t err;
    // iterate through channels and attributes of device
    for (channel = device -> channels; channel != NULL; channel = channel -> hh.next)
        for (attribute = channel -> attributes; attribute != NULL; attribute = attribute -> hh.next)
        {
            err = read_attribute(context, device -> addr,channel -> channel,attribute);
            if (err != 0)
            {
                if (DEBUG)
                    printf("read_attribute failed %d. Channel %d addr %d attribute %s\n",err, device -> addr, channel -> channel, attribute -> attribute_name);
                return -1;
            }
        }
    return 0;
}

// pushes device attribute values to sensor andrew
// returns 0 on success -1 on failure
static int32_t update_device(pup_adapter_t *adapter, pup_device_t *device)
{
    pup_channel_t *channel;
    pup_attribute_t *attribute;
    mio_stanza_t *item = mio_pubsub_item_data_new(adapter -> info -> pubsub_conn);
    char* time_str = mio_timestamp_create();
    char* value_str = NULL;
    char tid_str[30];
    for (channel = device -> channels; channel != NULL ; channel = channel -> hh.next)
    {
        for (attribute = channel -> attributes; attribute != NULL ; attribute = attribute -> hh.next)
        {
            if ((attribute -> update) && (attribute -> value != NULL))
            {
                value_str = data_string(attribute->display,*((uint32_t*)attribute -> value));
                if (value_str == NULL)
                    continue;
                sprintf(tid_str,"%d",attribute->update);
                mio_item_transducer_value_add(item, NULL, attribute->type, value_str, value_str,time_str);
                free(value_str);
            } else
                printf("not updating %s \n", attribute -> type);
        }
    }

    while (adapter -> info -> pubsub_conn -> xmpp_conn -> state != XMPP_STATE_CONNECTED)
    {
        printf("xmpp not connected\n");
        usleep(1000);
    }
    mio_response_t* response = mio_response_new();
    mio_item_publish_data(adapter ->info-> pubsub_conn, item, device -> node_id, response);
    free(time_str);
    mio_response_free(response);
    mio_stanza_free(item);
    return 0;
}

// handles token passing between pup context  and next master of context.
// Meant to run in thread. NOTE assumes serial mutex locked when called
void token_passer(pup_adapter_t* adapter)
{
    pup_frame_t* frame;
    if (adapter -> context == NULL)
    {
        printf("context is NULL\n");
        return;
    }
    while (TRUE)
    {
        // wait for token pass message
        while(TRUE)
        {
            frame = read_frame(adapter -> context -> fd);
            if ((frame -> command == pup_pass_token) &&
                    (frame -> addr == adapter -> context -> addr))
            {
                adapter -> context -> next_master -> addr = (*((uint16_t*)frame -> data));
                break;
            }
            free_pup_frame(frame);
        }
        // unlock serial mutex and then lock when other threads have completed
        if (DEBUG)
            printf("\nreceived token\n");
        free_pup_frame(frame);
        pthread_mutex_unlock(adapter -> context -> serial_mutex);
        pthread_mutex_lock(adapter -> context -> serial_mutex);
        pass_token(adapter -> context,1);
        if (DEBUG)
            printf("passed token\n");
    }
    pthread_exit(NULL);
}

// creates new pup_adapter
// arguments - addr is address of pup context, serial_file is path to tty serial file,
// returns - initialized pup adapter
pup_adapter_t* new_pup_adapter(uint16_t addr, char* serial_file, pup_device_t *devices, char* username, char* password)
{
    pup_adapter_t* adapter = (pup_adapter_t*) malloc(sizeof(pup_adapter_t));
    adapter -> info = new_mio_info(username, password);
    adapter -> context =  new_pup_context(addr, serial_file, 9600, devices, 0);
    if (serial_file != NULL)
        connect_pup_context(adapter -> context);
    return adapter;
}

// frees allocated memory of pup adapter
void free_pup_adapter(pup_adapter_t* adapter)
{
    free(adapter);
    return;
}

// initializes mio_info struct
// arguments - username jid-username for sensor andrew, password jid-password for sensor andrew user
// returns - initialized mio_info
mio_info_t* new_mio_info(char* username, char* password)
{
    mio_info_t* info = (mio_info_t*) malloc(sizeof(mio_info_t));
    memset(info,0x0,sizeof(mio_info_t));

    if (username != NULL)
    {
        if (DEBUG)
            info -> pubsub_conn = mio_conn_new(MIO_LEVEL_ERROR);
        else
            info -> pubsub_conn = mio_conn_new(MIO_LEVEL_DEBUG);
        info -> jid_username = username;
        if (password == NULL)
            info -> jid_password = getpass("password: ");
        else
            info -> jid_password = password;
        info -> xmpp_server =  _mio_get_server(username);
        info -> pubsub_server = (char*) malloc(60);
        strcpy(info -> pubsub_server, "pubsub.");
        strcat(info->pubsub_server,info -> xmpp_server);

        mio_connect(info -> jid_username,info -> jid_password,NULL,NULL,info -> pubsub_conn);
    }
    return info;
}

// connect mio_info struct to Sensor Andrew
// arguments - unconnected mio_info into
void connect_mio_info(mio_info_t *info)
{
    if (DEBUG)
        info -> pubsub_conn = mio_conn_new(MIO_LEVEL_DEBUG);
    else
        info -> pubsub_conn = mio_conn_new(MIO_LEVEL_ERROR);
    if(info -> jid_password == NULL)
        info->jid_password = getpass("password: ");
    info -> xmpp_server = _mio_get_server(info -> jid_username);
    info -> pubsub_server = (char*) malloc(60);
    strcpy(info -> pubsub_server,"pubsub.");
    strcat(info -> pubsub_server,info->xmpp_server);
    mio_connect(info -> jid_username, info -> jid_password,
                NULL,NULL,info -> pubsub_conn);
}

// frees memory allocated for mio_info_t
void free_mio_info(mio_info_t* info)
{
    free(info);
    return;
}


static void handle_data(void *data, const char *content, int length);

static void end_network_element(void *data, const char *el);
static void start_network_element(void *data, const char *element, const char **attribute);

static void end_device_element(void *data, const char *el);
static void start_device_element(void *data, const char *element, const char **attribute);

static void end_regmap_element(void *data, const char *el);
static void start_regmap_element(void *data, const char *element, const char **attribute);

static pup_channel_t* clone_channel(pup_channel_t* orig_channels,int32_t hash_table);
static char* clone_str(const char* str);
static void lock_adapter_mutex();

static void replicate_channels(pup_device_t * device, pup_channel_t* channel, int32_t r1, int32_t r2, int32_t r3,int32_t r4);

static int32_t depth = 0;
static pup_adapter_t* _adapter = NULL;
static pup_device_t* _device = NULL;
static pup_channel_t* _channel = NULL;
static pup_attribute_t* _attribute = NULL;
static char* _range = NULL;
static char* _ranges = NULL;
static int32_t _tid = 0;

pthread_mutex_t* adapter_mutex = NULL;

//  locks the adapter_mutex, can only read in one adapter context at once due to static variables
void lock_adapter_mutex()
{
    if (adapter_mutex == NULL)
    {
        adapter_mutex = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(adapter_mutex,0);
    }
    pthread_mutex_lock(adapter_mutex);
}

// Parses XML start_handler and end_handler sets appropriate local static variables
// arguments - xml_file is path to xml file to parse, start_handler is the expat xml start parser function
//          end_handler is the expat xml end parser function
// returns - 0 on success 1 on failure
/*int32_t parse_xml(char* xml_file,void (*start_handler)(void *data, const char *element, const char **attribute),
                  void (*end_handler)(void *data, const char *el))
{
    int32_t buff_size = 200,read_size, err, level, done = 0;
    char buff[200];
    FILE *fp;
    depth = 0;
    fp = fopen(xml_file, "r");
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, &level);
    XML_SetElementHandler(parser, start_handler, end_handler);
    XML_SetCharacterDataHandler(parser, handle_data);
    while (!done)
    {
        read_size = fread(buff, sizeof(char), buff_size, fp);
        if (read_size != buff_size)
            done = 1;
        else
            done = 0;
        if (XML_STATUS_ERROR == (XML_Parse(parser, buff, read_size, done))) {
            err = errno;
            printf("XML_Parse Failed %d\n", err);
            return 1;
        }
    }
    XML_ParserFree(parser);
    fclose(fp);
    return 0;
}*/

// this clones
// arguments - original channels pointer, hash_table indicates if it is a single channel or
//          a UTHASH map
// returns - deep copy of pup_channel
pup_channel_t* clone_channel(pup_channel_t* orig_channels, int32_t hash_table)
{
    pup_channel_t *clone = NULL;
    pup_channel_t *clone_tmp, *channel;
    pup_attribute_t *attributes = NULL, *attribute_clone ,*attribute_tmp;
    if (hash_table)
    {
        for (channel = orig_channels; channel; channel = channel -> hh.next)
        {
            for(attribute_tmp = channel -> attributes; attribute_tmp; attribute_tmp = attribute_tmp -> hh.next)
            {
                attribute_clone = (pup_attribute_t*) malloc(sizeof(pup_attribute_t));
                memset(attribute_clone,0x0,sizeof(pup_attribute_t));
                strcpy(attribute_clone -> attribute_name, attribute_tmp -> attribute_name);
                attribute_clone -> display = attribute_tmp -> display;
                attribute_clone -> type = clone_str(attribute_tmp -> type);
                attribute_clone -> value = NULL;
                attribute_clone -> display = attribute_tmp -> display;
                HASH_ADD_STR(attributes,attribute_name,attribute_clone);
            }
            clone_tmp = (pup_channel_t*) malloc(sizeof(pup_channel_t));
            memset(clone_tmp,0x0,sizeof(pup_channel_t));
            clone_tmp -> channel = channel -> channel;
            clone_tmp -> name = clone_str(channel -> name);
            clone_tmp -> attributes = attributes;
            HASH_ADD_INT(clone,channel,clone_tmp);
            attributes = NULL;
            clone_tmp = NULL;
        }
    } else {
        for(attribute_tmp = orig_channels -> attributes; attribute_tmp; attribute_tmp = attribute_tmp -> hh.next)
        {
            attribute_clone = new_pup_attribute(attribute_tmp -> attribute_name, attribute_tmp -> type,NULL,0);
            attribute_clone -> display = attribute_tmp -> display;
            HASH_ADD_STR(attributes,attribute_name,attribute_clone);
        }
        clone = new_pup_channel(orig_channels -> channel, orig_channels -> name, attributes);
    }

    return clone;
}

// creates deep copy of string
// arguments - const string
// returns - deep copy of string
char* clone_str(const char* str) {
    if (str == NULL)
        return NULL;
    int32_t len = strlen(str);
    char* clone = (char*) malloc(len+1);
    strcpy(clone,str);
    clone[len] = '\0';
    return clone;
}

pup_device_t* end_of(pup_device_t* device)
{
    if (device == NULL)
    {
        return NULL;
    }
    for (; device -> hh.next != NULL; device = device -> hh.next)
    {}
    return device;
}
// parses given network device and tid_xml files and returns a pup_adapter initialized
//        with information from those files
// arguments - path to network_xml file, device_xml, tid_xml see c
//      configureations/configuration.txt for information on the contents of these files
// returns - initialized pup_adapter that, if successful is connected to sensor andrew
//            as well as the pup_context to the serial port
void* pup_parser(void* adapterv)
{
    adapter_t *adapter = (adapter_t*) adapterv;
    char* dir = adapter -> config_dir;
    char* network_xml = malloc(100), *device_xml = malloc(100);
    DIR *d = opendir(dir);
    sprintf(network_xml,"%s/network.xml",dir);
    sprintf(network_xml,"%s/device.xml",dir);
    pup_adapter_t *pup_context;
    if ( d == NULL)
    {
      lock_adapter_mutex();
      _adapter = (pup_adapter_t*) malloc(sizeof(pup_adapter_t));
      memset(_adapter,0x0,sizeof(pup_adapter_t));
      _adapter -> info = (mio_info_t*) malloc(sizeof(mio_info_t));
      memset(_adapter -> info, 0x0, sizeof(mio_info_t));
      _adapter -> context = new_pup_context(0,NULL,9600,NULL,0);

      _device = new_pup_device(0,NULL,NULL,NULL);
      _channel = new_pup_channel(0,NULL,NULL);
      _attribute = new_pup_attribute(NULL,NULL,NULL,0);

      parse_xml(network_xml, start_network_element, end_network_element);

      parse_xml(device_xml, start_device_element, end_device_element);

      pup_context = _adapter;
      _adapter = NULL;
    } else
    {
	
        pup_context = parse_pup_node(adapter);
    }
    connect_pup_context(adapter -> context);
    pthread_mutex_unlock(adapter_mutex);
    return pup_context;
}
 
pup_adapter_t* parse_pup_node(adapter_t *adapter)
{
     mio_reference_t *pup_reference;
     mio_reference_t *ref_tmp;
     mio_response_t *pup_ref_response;
     mio_packet_t *packet;

     pup_context_t *tmpcontext;
     pup_adapter_t *padapter = NULL;
     int err;

     pup_ref_response = mio_response_new();
     err = mio_references_query(adapter -> connection, adapter -> config_dir, pup_ref_response);
     if (err != MIO_OK)
     {
         mio_response_free(pup_ref_response);
         printf("could not get references\n");
         return NULL;
     }

     packet = (mio_packet_t*) pup_ref_response -> response;
     pup_reference = (mio_reference_t*) packet -> payload;
     for (ref_tmp = pup_reference; ref_tmp != NULL; ref_tmp = ref_tmp -> next)
     {
         tmpcontext = parse_pup_context(adapter -> connection, ref_tmp -> node);
         tmpcontext -> hh.next  = padapter -> context;
         padapter -> context = tmpcontext;
     }
     return  padapter;
}

pup_context_t* parse_pup_context(mio_conn_t *connection, char* event_id) 
{
    mio_response_t *ref_response, *meta_response;
    mio_meta_t *adapter_meta;
    mio_packet_t *packet;
    mio_reference_t *adapter_ref, *ref_tmp;
    mio_property_meta_t *prop;
    pup_device_t *tmp_devices, *end_device, *devices;
    pup_context_t *context = malloc(sizeof(pup_context_t));

    memset(context,0x0,sizeof(pup_context_t));
    meta_response = mio_response_new();
    mio_meta_query(connection, event_id, meta_response);
    packet = (mio_packet_t*) meta_response -> response;
    adapter_meta = (mio_meta_t *) packet -> payload;
    for (prop = adapter_meta -> properties; prop != NULL; prop = prop -> next)
    {
        if (strcmp(prop -> name, "Path\0") == 0)
        {
            context -> serial_path = string_copy(prop -> value);
        } else if (strcmp(prop -> name,"Address\0" ) == 0)
        {
            context -> addr = atoi(prop -> value);
        }
    }
    context -> serial_mutex = malloc(sizeof(pthread_mutex_t));
    memset(context -> serial_mutex,0x0,sizeof(pthread_mutex_t));
    if (connect_pup_context(context) != 0)
    {
        fprintf(stderr, "Could not connect pup context\n");
        return NULL;
    }

    pthread_mutex_init(context -> serial_mutex, NULL);
    mio_response_free(meta_response);
    ref_response = mio_response_new();
    if (mio_references_query(connection, event_id, ref_response))
    {
        fprintf(stderr, "Could not get meta packet\n");
        return NULL;
    }
    packet = (mio_packet_t *) meta_response -> response;
    adapter_ref = (mio_reference_t*) packet -> payload;
    for (ref_tmp = adapter_ref; ref_tmp != NULL; ref_tmp = ref_tmp -> next)
    {
        if (ref_tmp -> type == MIO_REFERENCE_PARENT)
            continue;
        tmp_devices = parse_pup_devices(connection, ref_tmp -> node);
        tmp_devices -> hh.next = devices;
        end_device = end_of(tmp_devices);
        end_device -> hh.next = devices;
        devices = tmp_devices;
    }
    return context;
}

pup_device_t* parse_pup_devices(mio_conn_t *connection, char* event_id)
{
    mio_response_t *ref_response, *meta_response;
    mio_packet_t *packet;
    mio_transducer_meta_t *tran;
    pup_channel_t *channel;
    pup_attribute_t *tmp_attribute;
    mio_meta_t *meta;
    mio_property_meta_t *prop;
    mio_reference_t *device_ref, *ref_tmp;
    pup_device_t *tmp_device, *devices, *end_device;
 
    ref_response = mio_response_new();
    if (mio_references_query(connection,event_id,ref_response) != MIO_OK)
    {
        fprintf(stderr, "Could not retrieve device references\n");
        return NULL;
    }

    packet = (mio_packet_t*) ref_response -> response;
    device_ref = (mio_reference_t*) packet -> payload;
    for (ref_tmp = device_ref; ref_tmp != NULL; ref_tmp = ref_tmp -> next)
    {
        if (ref_tmp -> type == MIO_REFERENCE_PARENT)
            continue;
        meta_response = mio_response_new();
        if (mio_meta_query(connection, ref_tmp -> node,meta_response) != MIO_OK)
        {
            fprintf(stderr, "Could not\n");
            continue;
        }

        if (ref_tmp -> meta_type == MIO_META_TYPE_LOCATION)
        {
           tmp_device = parse_pup_devices(connection, ref_tmp -> node); 
           end_device = end_of(tmp_device);
           end_device -> hh.next = devices;
           devices = tmp_device;
        } else 
        {
            tmp_device = new_pup_device(0,NULL,NULL,NULL);
            meta_response = mio_response_new();
            if (mio_meta_query(connection, ref_tmp -> node, meta_response) != MIO_OK)
            {
                fprintf(stderr, "Could not get device meta data\n");
                continue;
            }
            packet = (mio_packet_t *) meta_response -> response;
            meta = (mio_meta_t *) packet -> payload;
            for (prop = meta -> properties; prop != NULL; prop = prop -> next)
            {
                if (strcmp(prop -> name, "Unit\0") == 0)
                {
                    tmp_device -> addr = atoi(prop -> value);
                }
            }
            for (tran = meta -> transducers; tran != NULL; tran = tran -> next)
            {
                channel = new_pup_channel(0,NULL,NULL);
                tmp_attribute = new_pup_attribute(NULL,NULL,NULL,0);
                for (prop = tran -> properties; prop != NULL; prop = prop -> next)
                {
                    if (strcmp(prop -> name,"Channel") == 0)
                    {
                        channel  -> channel =  atoi(prop -> value);
                    }

                    if (strcmp(prop-> name, "Attribute") == 0)
                    {
                        strcpy(tmp_attribute -> attribute_name, prop -> value);
                    }
                }
                channel -> attributes = tmp_attribute;
                channel -> hh.next = tmp_device -> channels;
                tmp_device -> channels = channel;
            }
            mio_response_free(meta_response);
            tmp_device -> hh.next = devices;
            devices = tmp_device;
        }
    }
    mio_response_free(ref_response);
    return devices;
}
// expat start_element function for network,xml file.
// Initializes adapter and devices from information in this file
void start_network_element(void *data, const char *element, const char **attribute)
{
    int32_t i;
    depth++;
    for (i = 0; attribute[i]; i += 2)
    {
        if (depth == 1)
        {
            if (strcmp("jid-username",attribute[i]) == 0)
            {
                _adapter -> info-> jid_username = clone_str(attribute[i+1]);
                printf("%s\n",_adapter -> info -> jid_username);
            } else if (strcmp("jid-password",attribute[i])==0)
                _adapter -> info -> jid_password = clone_str(attribute[i+1]);
            else if (strcmp("polling-rate",attribute[i]) == 0)
                _adapter -> info -> polling_rate = atoi(attribute[i+1]);
            else if (strcmp("pup_address",attribute[i]) == 0)
                _adapter -> context -> addr = atoi(attribute[i+1]);
            else if(strcmp("name",attribute[i]) == 0)
                _adapter -> context -> name = clone_str(attribute[i+1]);
            else if (strcmp("serial_path",attribute[i]) == 0)
                _adapter -> context -> serial_path = clone_str(attribute[i+1]);
            else if (strcmp("next_master",attribute[i])==0)
                _adapter -> context -> next_master = new_pup_device(atoi(attribute[i+1]),NULL,NULL,NULL);
            else
                printf("%s not supported\n",attribute[i]);

        } else if (depth == 2)
        {
            if (strcmp("type",attribute[i]) == 0)
                _device -> type = clone_str(attribute[i+1]);
            else if (strcmp("pup_address", attribute[i])==0)
                _device -> addr = atoi(attribute[i+1]);
            else if (strcmp("location", attribute[i])==0)
                _device -> location = clone_str(attribute[i+1]);
            else if (strcmp("name", attribute[i])==0)
                _device ->name = clone_str(attribute[i+1]);
            else if (strcmp("node_id", attribute[i])==0)
            {
                int32_t k;
                for (k = 0; k < strlen(attribute[i+1]); k++)
                    _device -> node_id[k] = (attribute[i+1])[k];
                _device -> node_id[k] = '\0';
            } else
                printf("%s not supported\n",attribute[i]);
        }
    }
}

// start_element expat function. Initializes device types as defined in
//      configurations/configuration.txt
void start_device_element(void *data, const char *element, const char **attribute)
{
    int32_t i;
    depth++;
    for (i = 0; attribute[i]; i += 2)
    {
        if (depth == 1)
        {
            if (strcmp(attribute[i],"device_type") == 0)
                _device -> type = clone_str(attribute[i+1]);
            else
                printf("%s not supported oop\n",attribute[i]);
        } else if (depth == 2)
        {
            if(strcmp(attribute[i], "channel_id") == 0)
            {
                uint32_t hold;
                sscanf(attribute[i+1],"%x",&hold) ;
                _channel -> channel = hold;
            }
            else if (strcmp(attribute[i],"channel") == 0)
                _channel -> name = clone_str(attribute[i+1]);
            else if (strcmp(attribute[i],"channel_range") == 0)
                _range = clone_str(attribute[i+1]);
            else if (strcmp(attribute[i],"channel_ranges") == 0)
                _ranges = clone_str(attribute[i+1]);
            else
                printf("%s not supported oop\n",attribute[i]);
        } else if(depth == 3)
        {
            if (strcmp(attribute[i],"update") == 0)
                _attribute -> update = atoi(attribute[i+1]);
            else if (strcmp(attribute[i],"attribute_id")==0)
                strncpy(_attribute -> attribute_name,attribute[i+1],2);
            else if (strcmp(attribute[i],"attribute")==0)
                _attribute -> type = clone_str(attribute[i+1]);
            else if (strcmp(attribute[i],"type") == 0)
            {
                int32_t left_dec,right_dec;
                char s;
                sscanf(attribute[i+1],"%c_%d_%d",&s,&left_dec,&right_dec);
                if ((s == 's' )&&(left_dec == 10))
                    _attribute -> display = 0xFF;
                else
                    _attribute -> display = 0xFD;

            } else
                printf("%s not supported\n",attribute[i]);
        }
    }
}

// start_element expat function. see configurations/configuration.txt
// sets the regid with associated devices and attributes
void start_regmap_element(void *data, const char *element, const char **attribute)
{
    int32_t i;
    depth++;
    for (i = 0; attribute[i]; i += 2)
    {
        if (depth == 2)
        {
            if (strcmp("node",attribute[i])==0)
                HASH_FIND_STR((_adapter -> context -> devices), attribute[i+1], _device);
            else
                printf("%s is not supported\n",attribute[i]);
        } else if (depth == 3)
        {
            if (strcmp("name",attribute[i])== 0)
            {
                for (_channel = _device -> channels; _channel; _channel = _channel -> hh.next)
                {
                    for (_attribute = _channel -> attributes; _attribute; _attribute = _attribute -> hh.next)
                    {
                        if (strcmp(_attribute -> type,attribute[i+1]) == 0)
                            break;
                    }
                    if (_attribute != NULL)
                        break;
                }
            } else if (strcmp("id",attribute[i])==0)
                _tid = atoi(attribute[i+1]);
            else
                printf("%s is not supported\n",attribute[i]);
        }
    }
}

// there is no data to handle
void handle_data(void *data, const char *content, int length)
{
    return;
}

// decrement the current level of the tree
// Handles the end of each section in network.xml
void end_network_element(void *data, const char *el)
{
    if (depth == 2)
    {
        HASH_ADD_STR((_adapter -> context -> devices),node_id,_device);
        _device = new_pup_device(0,NULL,NULL,NULL);
    }
    depth--;
}

// end of device element
void end_device_element(void *data, const char *el)
{
    pup_device_t *device;
    if (depth == 1)
    {
        // check device
        if (_device -> type != NULL)
        {
            // clone channels for each device of this type
            for (device = (_adapter -> context -> devices); device != NULL; device = device -> hh.next)
            {
                if (strcmp(device -> type, _device->type) == 0) {
                    device -> channels = clone_channel(_device -> channels, 1);
                }
            }
        } else {
            printf("device type is null\n");
        }
        free_pup_device(_device);
        _device = new_pup_device(0,NULL,NULL,NULL);
    } else if (depth == 2)
    {
        int32_t range_min, range_max, range_min2,range_max2;
        if (_range != NULL)
        {
            sscanf(_range,"%d-%d",&range_min,&range_max);
            free(_range);
            _range = NULL;
            replicate_channels(_device,_channel,range_min,range_max,0,0);
            free_pup_channel(_channel);
        } else if (_ranges != NULL)
        {
            sscanf(_ranges,"%d-%d,%d-%d",&range_min,&range_max,&range_min2,&range_max2);
            free(_ranges);
            _ranges = NULL;
            replicate_channels(_device,_channel, range_min,range_max,range_min2,range_max2);
            free_pup_channel(_channel);
        } else
            HASH_ADD_INT(_device -> channels,channel,_channel);
        _channel = new_pup_channel(0,NULL,NULL);
    } else if (depth == 3)
    {
        if (_attribute -> attribute_name[0] != '\0')
            HASH_ADD_STR(_channel -> attributes, attribute_name, _attribute);
        _attribute = new_pup_attribute(NULL,NULL,NULL,0);
    }
    depth--;
}

// replicates channels ranging from using the clone channel functionction
// arguments - device to replicate channels on, channel is the channel to replicate
//              r1 is beginning of range 1, r2 is the end of range 1, r3 is the beginning
//              of range 2 and r4 is the end of range 2
// returns - void
void replicate_channels(pup_device_t* device, pup_channel_t *channel, int32_t r1, int32_t r2, int32_t r3,int32_t r4)
{
    int i,k = 0;
    pup_channel_t * channel_tmp;
    char* tmp;
    for (i = r1; i <= r2; i++)
    {
        k++;
        channel_tmp = clone_channel(channel,0);
        channel_tmp -> channel += i;
        tmp = (char*)malloc(strlen(channel_tmp -> name) + 3);
        sprintf(tmp,"%s %d",channel_tmp -> name,i);
        free(channel_tmp -> name);
        channel_tmp -> name = tmp;

        tmp = (char*) malloc(strlen(channel_tmp -> attributes -> type) + 4);
        sprintf(tmp, "%s %d", channel_tmp -> attributes -> type, i);
        free(channel_tmp -> attributes -> type);
        channel_tmp -> attributes -> type = tmp;
        HASH_ADD_INT(_device -> channels,channel,channel_tmp);
        channel_tmp = NULL;
    }
    if (!r3 && !r4)
        return;
    for (i = r3; i <= r4; i++)
    {
        k++;
        channel_tmp = clone_channel(channel,0);
        channel_tmp -> channel += i;
        tmp = (char*)malloc(strlen(channel_tmp -> name) + 5);
        sprintf(tmp,"%s %d",channel_tmp -> name,k);
        channel_tmp -> name = tmp;
        tmp = (char*) malloc(strlen(channel_tmp -> attributes -> type) + 4);

        sprintf(tmp, "%s %d", channel_tmp -> attributes -> type, i-r3+r2+1);
        free(channel_tmp -> attributes -> type);
        channel_tmp -> attributes -> type = tmp;

        HASH_ADD_INT(_device -> channels,channel,channel_tmp);
        channel_tmp = NULL;
    }
}

/* decrement the current level of the tree */
void end_regmap_element(void *data, const char *el)
{
    if (depth == 2)
    {
        _device = NULL;
    } else if (depth == 3)
    {
        if (_attribute != NULL)
            _attribute -> update = _tid;
        _attribute = NULL;
    }
    depth--;
}

