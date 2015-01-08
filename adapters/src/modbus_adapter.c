#define PRINT_ENABLED 1
#include "adapter.h"
#include "modbus_adapter.h"
#include <mio_handlers.h>
#include <mio.h>
#include <strophe.h>
#include "uthash.h"
#include "xml_tools.h"
#include "modbus.h"
#include "modbus-rtu.h"
#include "modbus-tcp.h"


#include <uuid/uuid.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#define TRUE 1
#define DEBUG
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

// SERIAL DEFINITIONS
#define BAUD_RATE   115200
#define PARITY      'N'
#define DATA_BITS   8
#define STOP_BITS   1

#define INPUT_REGISTER  0
#define REGISTER        1

#define MODBUS_REGISTER_OFFSET                  4000

typedef enum modbus_device_type
{
    ENUM,
    FE2,
    SFE2,
    SPLIT_BYTE,
    PE2,
    GE2,
    INT,
    BM
} modbus_device_type_t;

/** Modbus register stores register values */
typedef struct modbus_register
{
    int multi_byte; /* true when  multiple registers for single value*/

    char name[40];          /* Name used when publishing mio XML */
    char name2[40];    /* Name used when publishing second */

    uint16_t mb_start_register; /* Modbus resgister in which value is stored */
    uint16_t n_registers;   /* Number of registers after the starting */
    uint16_t register_type;
    modbus_device_type_t response_type;
    modbus_device_type_t response_type2;
    int offset;
    int register_length;
    uint16_t value;         /* Value obtained back */
    char* transducer;
    char* transducer_2;
    char ref_name[40];
    UT_hash_handle hh;
} modbus_register_t;

typedef struct modbus_device
{
    char* event_node_id;
    uint16_t slave_id;
    modbus_register_t* registers;
    UT_hash_handle hh;
    //UT_hash_handle modreg_map;
} modbus_device_t;


typedef struct modbus_adapter
{
    modbus_t* context;
    char* serial_path;
    modbus_device_t* devices;
    char *jid_username;
    char *jid_password;
    char *xmpp_server;
    char *pubsub_server;
    mio_conn_t* pubsub_conn;
    uint32_t xmpp_server_port;
    uint32_t polling_rate;
    pthread_mutex_t* lock;
} modbus_adapter_t;

static char* get_mio_string(modbus_register_t* reg, int split);
static uint16_t get_value(int32_t data_type,char* value);
//static float ftoc(float f);
static modbus_adapter_t* new_modbus_adapter();
static modbus_device_t* new_modbus() ;
static modbus_register_t* new_modbus_register();
static void free_modbus_registers(modbus_register_t* registers);
static void free_modbus_adapter(modbus_adapter_t* adapter);
static void free_modbus_device(modbus_device_t* adapter);

modbus_adapter_t* parse_modbus_context(adapter_t * adapter);

static void disconnect_modbusrtu(modbus_adapter_t* adapter);
static int32_t connect_modbusrtu(modbus_adapter_t* adapter);
static int32_t get_all_registers(modbus_adapter_t* adapter);
static int32_t get_register(modbus_adapter_t* adapter, modbus_device_t* mod);
static int32_t publish_all_registers(adapter_t* adapter);
static int32_t write_register(modbus_adapter_t* adapter, modbus_device_t* mod, modbus_register_t* registers);
static void free_modbus_device(modbus_device_t* mod);
static void print_modbus(modbus_device_t* mod);

static void start_modbus_element(void *data, const char *element, const char **attribute);
static void end_modbus_element(void *data, const char *el);

static void start_device_element(void *data, const char *element, const char **attribute);
static void end_device_element(void *data, const char *el);


static modbus_device_t* clone_modbus_device(modbus_device_t* wi);
static modbus_register_t* clone_modbus_register(modbus_register_t* reg);

static modbus_adapter_t *_adapter;
static modbus_device_t *_modbus;
static modbus_device_t *_device;
static modbus_register_t *_modbus_register;
static int32_t depth = 0;
static char* _tid;
static char* _name;
/*********************** Public Modbus Functions *********************/

void* modbus_parser(void* adapterv)
{
    adapter_t *adapter = (adapter_t *) adapterv;
    char* modbus_dir = (char*) adapter -> config_dir;
    char modbus_xml[512],  device_xml[512];
    DIR *d;
    d = opendir(modbus_dir);
    modbus_adapter_t *mod_adapter;
    if (d != NULL)
    {
        closedir(d);
        snprintf(modbus_xml,512,"%s/modbus.xml",modbus_dir);
        snprintf(device_xml,512,"%s/devices.xml",modbus_dir);
        _adapter = new_modbus_adapter();
        _adapter -> devices = NULL;
        _modbus = new_modbus();
        _modbus_register = new_modbus_register();
        if(parse_xml(modbus_xml, start_modbus_element, end_modbus_element))
            return NULL;
        if (parse_xml(device_xml, start_device_element, end_device_element))
            return NULL;
    } else {
        _adapter = parse_modbus_context(adapter);
    }

    mod_adapter = _adapter;
    if( connect_modbusrtu(mod_adapter) == -1)
    {
         free_modbus_adapter(mod_adapter);
         mod_adapter = NULL;
   }

    return mod_adapter;
}

void* modbus_actuate(void* vadapter)
{
    modbus_adapter_t *adapter = (modbus_adapter_t*) vadapter;
    mio_conn_t* pubsub_conn = adapter -> pubsub_conn;
    modbus_device_t* modbus_device_tmp;
    mio_packet_t* packet;
    mio_data_t* data;
    mio_response_t* response;
    mio_transducer_value_t* tran;
    modbus_register_t* register_tmp;
    int32_t err,*retval;

    while (TRUE) {
        if (pubsub_conn -> xmpp_conn -> state == XMPP_STATE_CONNECTED) {
            response = mio_response_new();
            err = mio_pubsub_data_receive(adapter -> pubsub_conn, response);
            if (err != MIO_OK)
            {
                mio_response_free(response);
                continue;
            }
            packet = (mio_packet_t*) response->response;
            data = (mio_data_t*) packet->payload;
            for (tran = data->transducers; tran != NULL; tran = tran->next) {
                if (tran -> type != MIO_TRANSDUCER_SET_VALUE)
                    continue;
                    for (modbus_device_tmp = adapter -> devices; 
                        modbus_device_tmp; modbus_device_tmp = 
                        modbus_device_tmp ->hh.next)
                    {
                        if (strcmp(modbus_device_tmp->event_node_id, data->event))
                            continue;
                        pthread_mutex_lock(adapter -> lock);
                        for (register_tmp = modbus_device_tmp -> registers; 
                            register_tmp; register_tmp = register_tmp-> hh.next)
                        {
                            if (strcmp((register_tmp -> name),tran -> name)==0)
                            {
                                register_tmp -> value = get_value(
                                    register_tmp->response_type,
                                    tran->typed_value);
                                break;
                            }
                        }
                        if (register_tmp == NULL)
                        {
                            pthread_mutex_unlock(adapter ->lock);
                            continue;
                        }
                        err = write_register(adapter, modbus_device_tmp, 
                            register_tmp);
                        pthread_mutex_unlock(adapter -> lock);
                    }

            }
            mio_response_free(response);

        } else
            usleep(1000);
    }
    *retval = 0;
    pthread_exit((void *)retval);
}

void* modbus_data(void* vadapter)
{
    adapter_t *adapter = (adapter_t*) vadapter;
    modbus_adapter_t* mod_adapter = (modbus_adapter_t*) adapter -> context;
    int32_t err;
    while (1)
    {
        //pthread_mutex_lock(adapter -> lock);
        err = get_all_registers(mod_adapter);
        if (err != 0)
        {
            if (DEBUG)
                printf("Error publish_all_registers %d\n",err);
            continue;
        }
        err = publish_all_registers(adapter);
        if (err != 0)
        {
            if (DEBUG)
                printf("Error publish_all_registers %d\n",err);
            continue;
        }
        //pthread_mutex_unlock(adapter -> lock);
        sleep(UPDATE_PERIOD);
    }
}

/************************ Utility Functions **************************/
/*static float ftoc(float f)
{
    float c = (7.0/9.0)*f - 32.0;
    return c;
}*/

static uint16_t get_value(int32_t data_type,char* value)
{
    float val_hold;
    uint16_t ret;
    if (!sscanf(value,"%f",&val_hold))
    {
        printf("could not read %s in get_value\n",value);
        return 0;
    }
    switch(data_type)
    {
    case INT:
    case ENUM:
        ret = val_hold;
        break;
    case FE2:
        ret = val_hold * 10;
        break;
    case SFE2:
        if(val_hold < 0)
        {
            ret =((int) -val_hold*10) | 0x8000;
        } else
            ret = val_hold * 10;
    default:
        ret = val_hold;
        break;
    }
    return ret;
}
static char* get_mio_string(modbus_register_t* reg, int split)
{
    char* value = malloc(10);
    uint16_t val = reg -> value;
    uint16_t reg_type = reg-> response_type;
    if (split)
        val = 0xFF & (val >> (split -1));
    if (split == 2)
        reg_type = reg -> response_type2;
    switch(reg_type)
    {
    case INT:
        sprintf(value,"%d", val);
        break;
    case ENUM:
        sprintf(value,"%d",val);
        break;
    case FE2:
        sprintf(value,"%3.1f",((float) val)/10.0);
        break;
    case SFE2:
    default:
        sprintf(value,"%3.1f", ((float) val) /10.0);
        break;
    }
    return value;
}



/* End modbus utility functions */

/************************ modbus structs *******************************/
static modbus_adapter_t* new_modbus_adapter()
{
    modbus_adapter_t* adapter = (modbus_adapter_t*) malloc(sizeof(modbus_adapter_t));

    adapter -> devices = NULL;
    adapter -> jid_username = NULL;
    adapter -> jid_password = NULL;
    adapter -> xmpp_server = NULL;
    adapter -> pubsub_server = NULL;
    adapter -> pubsub_conn = NULL;
    adapter -> xmpp_server_port = 0;
    adapter -> polling_rate = 10;
    adapter -> lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    memset(adapter -> lock,0x0, sizeof(pthread_mutex_t));
    int32_t err = pthread_mutex_init(adapter -> lock, NULL);
    if (err != 0)
    {
        if (DEBUG)
            printf("modbus adpater: mutex init failed\n");
        return NULL;
    }
    pthread_mutex_unlock(adapter->lock);
    return adapter;
}

static modbus_device_t* new_modbus()
{
    modbus_device_t* modbus = (modbus_device_t*) malloc(sizeof(modbus_device_t));
    modbus -> event_node_id = NULL;
    modbus -> slave_id = 0;
    modbus -> registers = NULL;
    return modbus;
}

static modbus_register_t* new_modbus_register()
{
    modbus_register_t* reg = (modbus_register_t*) malloc(sizeof(modbus_register_t));
    memset(reg -> name,0x0,40);             /* Name used when publishing mio XML */
    memset(reg -> name2,0x0,40);            /* Name used when publishing mio XML */
    memset(reg -> ref_name,0x0,40);             /* Name used when publishing mio XML */
    reg -> mb_start_register = 0;   /* Modbus resgister in which value is stored */
    reg -> n_registers = 0; /* Number of registers after the starting register */
    reg -> response_type = 0;       /* Type of response expected: MB_FLOAT, etc. */
    reg -> response_type2 = 0;      /* Type of response expected: MB_FLOAT, etc. */
    reg -> value = 0;           /* Value obtained back */
    return reg;
}

static void free_modbus_adapter(modbus_adapter_t* adapter)
{
    if (adapter -> context != NULL)
        modbus_free(adapter -> context);

    if (adapter -> serial_path != NULL)
        free(adapter -> serial_path);

    if (adapter -> devices != NULL)
        free_modbus_device(adapter -> devices);

    if (adapter -> jid_username != NULL)
        free(adapter -> jid_username);

    if (adapter -> jid_password != NULL)
        free(adapter -> jid_password);

    if (adapter -> pubsub_conn != NULL)
        mio_conn_free(adapter -> pubsub_conn);

    if (adapter -> lock != NULL)
        pthread_mutex_destroy(adapter -> lock);
    free(adapter);
}

static void free_modbus_device(modbus_device_t* modbus)
{
    modbus_device_t* modbus_device_tmp;
    modbus_device_t* modbus_next;
    for (modbus_device_tmp = modbus; modbus_device_tmp != NULL; modbus_device_tmp=modbus_next)
    {
        modbus_next = modbus_device_tmp->hh.next;
        if (modbus_device_tmp -> event_node_id != NULL)
            free(modbus_device_tmp ->event_node_id);
        if (modbus_device_tmp -> registers != NULL)
            free_modbus_registers(modbus_device_tmp -> registers);
        free(modbus_device_tmp);
    }
}

static void free_modbus_registers(modbus_register_t* registers)
{
    modbus_register_t* register_tmp;
    modbus_register_t* register_next;
    for (register_tmp = registers; register_tmp != NULL; register_tmp=register_next)
    {
        register_next = register_tmp->hh.next;
        free(register_tmp);
    }
}


/*End modbus struct*/

/*********************** Modbus functions *****************************/


static int32_t connect_modbusrtu(modbus_adapter_t* adapter)
{
    int32_t err = 0;
    adapter -> context = modbus_new_rtu(adapter -> serial_path, BAUD_RATE, PARITY, DATA_BITS, STOP_BITS);
    modbus_set_debug(adapter -> context, 1);
    if (adapter -> context == NULL)
    {
        printf("context could not be created\n");
        return -1;
    }
    if (modbus_connect(adapter -> context) == -1) {
        err = errno;
        if (DEBUG)
            printf("Connection failed: %s\n", modbus_strerror(err));
        modbus_free(adapter -> context);
        adapter -> context = NULL;
        return -1;
    }
    return 0;
}

static void disconnect_modbusrtu(modbus_adapter_t* adapter)
{
    modbus_close(adapter -> context);
    modbus_free(adapter -> context);
    adapter -> context = NULL;
}

static int32_t get_all_registers(modbus_adapter_t* adapter)
{
    modbus_device_t* modbus = NULL;
    modbus_register_t* registers = NULL;
    int32_t err;
    
    for (modbus  = adapter -> devices; modbus != NULL; modbus = modbus->hh.next)
    {
        printf("%dslave\n", modbus -> slave_id);
        err = modbus_set_slave(adapter -> context, modbus -> slave_id);
        if (err == -1)
        {
            fprintf(stderr, "Set slave failed: %s\n", modbus_strerror(errno));
            return err;
        }

        for (registers  = modbus -> registers; registers != NULL; 
            registers=registers->hh.next)
        {
            err = modbus_read_registers(adapter -> context, 
                registers -> mb_start_register, 1,&registers -> value);
            if (err == -1)
            {
                if (DEBUG)
                    fprintf(stderr, "Read input regsiters failed: %s %s\n", 
                        registers -> name, modbus_strerror(errno));
                continue;
            }
        }
    }
    return 0;
}

static int32_t get_register(modbus_adapter_t* adapter, modbus_device_t* device)
{
    modbus_register_t* registers = device -> registers;
    //HASH_FIND_PTR(modbus -> registers,&reg,registers);
    int32_t err;
    err = modbus_set_slave(adapter -> context, device -> slave_id);
    if (err == -1)
    {
        if (DEBUG)
            printf( "Set slave failed: %s\n", modbus_strerror(errno));
        return -1;
    }
    if (registers -> register_type == INPUT_REGISTER)
        err = modbus_read_input_registers(adapter -> context,registers -> mb_start_register,1, &registers -> value);
    else
        err = modbus_read_registers(adapter -> context, registers -> mb_start_register , 1,&registers -> value);

    printf("%d\n",registers -> value);
    if (err == -1)
    {
        if (DEBUG)
            printf( "Read register failed: %s\n", modbus_strerror(errno));
        return -1;
    }
    return 0;
}

static int32_t publish_all_registers(adapter_t* adapter)
{
    modbus_adapter_t* context = adapter -> context;
    modbus_register_t* registers;
    modbus_device_t* device;
    char* value;
    char* time_str;
    mio_response_t* publish_response;
    mio_stanza_t* item;
    for (device  = context -> devices; device != NULL; device=device->hh.next)
    {
        time_str = mio_timestamp_create();
        item = mio_pubsub_item_data_new(adapter -> connection);
        for (registers  = device -> registers; registers != NULL; registers=registers->hh.next)
        {
            if (registers -> name2 != NULL)
                value = get_mio_string(registers, 1);
            else
                value = get_mio_string(registers, 0);
            mio_item_transducer_value_add(item, NULL,registers -> name, value, value, time_str);
            if (registers -> name2 != NULL)
            {
                value = get_mio_string( registers, 2);
                mio_item_transducer_value_add(item, NULL,registers -> name2, value, value, time_str);
            }
        }
        publish_response = mio_response_new();
        while (adapter -> connection -> xmpp_conn -> state != XMPP_STATE_CONNECTED)
            usleep(1000);
        mio_item_publish_data(adapter -> connection,item,device->event_node_id,publish_response);
        free(time_str);
        if (DEBUG)
            mio_response_print(publish_response);
        mio_response_free(publish_response);
        mio_stanza_free(item);
    }
    return 0;
}

static int32_t write_register(modbus_adapter_t* adapter, 
    modbus_device_t* modbus, modbus_register_t* registers)
{
    int32_t err;
    err = modbus_set_slave(adapter -> context, modbus -> slave_id);
    if (err == -1)
    {
        fprintf(stderr, "Set slave failed: %s\n", modbus_strerror(errno));
        return -1;
    }
    int32_t val = registers -> value & 0xFFFF;
    err = modbus_write_register(adapter -> context, registers -> mb_start_register
                                ,val);
    if (err == -1)
    {
        fprintf(stderr, "Write regsiters failed: %s\n", modbus_strerror(errno));
        return -1;
    }
    if (DEBUG)
        printf("Writing register: %s %d\n", registers -> name, registers -> value);

    return 0;
}

static void print_modbus(modbus_device_t* modbus)
{
    modbus_device_t* modbus_device_tmp;
    printf("modbus List:\n\n");
    for (modbus_device_tmp = modbus; modbus_device_tmp != NULL; 
        modbus_device_tmp=modbus_device_tmp->hh.next)
    {
        printf("\tEvent Node ID: %s\n\tSlave ID: %d\n\t",
           modbus_device_tmp -> event_node_id,  modbus_device_tmp -> slave_id);
    }
}


/********************** MODBUS XML Parser **************************/

modbus_adapter_t* parse_modbus_context(adapter_t * adapter)
{
     mio_reference_t *modbus_reference;
     mio_reference_t *ref_tmp;
     mio_response_t *modbus_ref_response;
     mio_response_t *modbus_meta_response;
     mio_packet_t *packet;
     mio_meta_t *modbus_meta;
     mio_transducer_meta_t *transducer;
     mio_property_meta_t *modbus_properties;
     mio_property_meta_t *property_itr;
     modbus_adapter_t *mod_adapter = malloc(sizeof(modbus_adapter_t));
     modbus_register_t *mod_register;
     modbus_device_t *device;
     int err;
     memset(mod_adapter,0x0,sizeof(modbus_adapter_t));

     modbus_ref_response = mio_response_new();
     err = mio_references_query(adapter -> connection, adapter -> config_dir, modbus_ref_response);
     //iterate over references
     if (err != MIO_OK)
     {
          mio_response_free(modbus_ref_response);
          printf("could not query for modbus references.]n");
          return NULL;
     }
     packet = modbus_ref_response -> response;
     modbus_reference = (mio_reference_t*) packet -> payload; 

     // check for serial path
     modbus_meta_response = mio_response_new();
     err = mio_meta_query(adapter -> connection, adapter -> config_dir, modbus_meta_response);
     packet = modbus_meta_response -> response;
     modbus_meta = (mio_meta_t*) packet -> payload;
     if (err != MIO_OK)
     {
          mio_response_free(modbus_meta_response);
          printf("could not query for modbus references.]n");
          return NULL;
     }
     mio_response_free(modbus_meta_response);
     for (property_itr = modbus_meta -> properties; property_itr != NULL; 
         property_itr = property_itr -> next)
     {
         if (strcmp(property_itr -> name, "Serial Path") == 0)
         {
             mod_adapter -> serial_path = string_copy(property_itr -> value);
             break;
         }
     }

     for (ref_tmp = modbus_reference; ref_tmp != NULL; 
         ref_tmp = ref_tmp -> next)
     {
         modbus_meta_response = mio_response_new();
         err = mio_meta_query(adapter -> connection, adapter -> config_dir, 
              modbus_meta_response);
         if (err != MIO_OK)
         {
             mio_response_free(modbus_meta_response);
             continue;
         }
         modbus_properties = modbus_meta -> properties;
         device = new_modbus();
         device -> event_node_id = ref_tmp -> node;

         for (property_itr = modbus_properties; property_itr != NULL; 
             property_itr = property_itr -> next )
         {
             if (strcmp(property_itr -> name, "Slave ID") == 0)
             {
                 device -> slave_id = atoi(property_itr -> value);
                  
             }
         }

         HASH_ADD_STR(mod_adapter -> devices, event_node_id,_modbus);

         for (transducer = modbus_meta -> transducers; transducer != NULL; 
             transducer = transducer -> next)
         {
             mod_register = malloc(sizeof(modbus_register_t));
             for (property_itr = transducer -> properties; property_itr!=NULL;
                 property_itr = property_itr -> next)
             {
                 if (strcmp(property_itr -> name, "Name") == 0)
                 {
                     sprintf(mod_register -> name, "%s", property_itr -> value);    
                 } else if (strcmp(property_itr -> name, "Register Type") == 0)
                 {
                     mod_register -> response_type = atoi(property_itr -> value);
                 } else if (strcmp(property_itr -> name, "Offset") == 0)
                 {
                     mod_register -> offset = atoi(property_itr -> value);
                 } else if (strcmp(property_itr -> name, "Register Length") == 0)
                 {
                     mod_register -> register_length = atoi(property_itr -> value);
                 } else if (strcmp(property_itr -> name,"Address") == 0)
                 {
                     mod_register -> mb_start_register = atoi(property_itr -> value);
                 }
             }
             if (device -> registers == NULL)
                 device -> registers = mod_register;
             else 
             {
                 mod_register = device -> registers -> hh.next;
                 device -> registers = mod_register;
             }

         }
         mio_response_free(modbus_meta_response);
     }

     return mod_adapter;

}

void start_modbus_element(void *data, const char *element, const char **attribute)
{
    int32_t i;
    depth++;
    for (i = 0; attribute[i]; i += 2)
    {
        if (depth == 1)
        {
            if (strcmp("serial_path",attribute[i]) == 0)
                _adapter -> serial_path = string_copy(attribute[i+1]);
            else
                printf("%s not supported\n",attribute[i]);

        } else if (depth == 2)
        {
            if (strcmp("event_node_id",attribute[i])==0)
            {
                _modbus -> event_node_id = string_copy(attribute[i+1]);
            } else if (strcmp("slave-id",attribute[i]) == 0)
            {
                int32_t hold = atoi(attribute[i+1]);
                _modbus -> slave_id = hold;
            } else
                printf("%s not supported\n",attribute[i]);
        }
    }
}

void end_modbus_element(void *data, const char *el)
{
    if (depth == 2)
    {
        HASH_ADD_STR(_adapter -> devices, event_node_id,_modbus);
        _modbus = new_modbus();
    }
    depth--;
}

void start_device_element(void *data, const char *element, const char **attribute)
{
    int32_t i;
    depth++;
    for (i = 0; attribute[i]; i += 2)
    {
        if (depth == 2)
        {
            if (strcmp(attribute[i],"name") == 0)
                strcpy(_modbus_register -> name,attribute[i+1]);
            else if(strcmp(attribute[i],"ref_name") == 0)
                strcpy(_modbus_register -> ref_name,attribute[i+1]);
            else if(strcmp(attribute[i],"addr") == 0)
            {
                _modbus_register -> mb_start_register = atoi(attribute[i+1]);;
            } else if(strcmp(attribute[i],"name2")==0)
            {
                strcpy(_modbus_register -> name2,attribute[i+1]);
            }
            else if(strcmp(attribute[i],"data_type")==0)
            {
                if (strcmp(attribute[i+1], "FE2")==0)
                    _modbus_register -> response_type = FE2;
                else if (strcmp(attribute[i+1],"ENUM") == 0)
                    _modbus_register -> response_type = ENUM;
                else if (strcmp(attribute[i+1],"SFE2") == 0)
                    _modbus_register -> response_type = SFE2;
                else if (strcmp(attribute[i+1],"INT") == 0)
                    _modbus_register -> response_type = INT;
            } else if(strcmp(attribute[i],"data_type2")==0)
            {
                if (strcmp(attribute[i+1], "FE2")==0)
                    _modbus_register -> response_type2 = FE2;
                else if (strcmp(attribute[i+1],"ENUM") == 0)
                    _modbus_register -> response_type2 = ENUM;
                else if (strcmp(attribute[i+1],"SFE2") == 0)
                    _modbus_register -> response_type2 = SFE2;
                else if (strcmp(attribute[i+1],"INT") == 0)
                    _modbus_register -> response_type2 = INT;
            } else
                printf("%s not supported\n", attribute[i]);
        }
    }
}

void end_device_element(void *data, const char *el)
{
    modbus_device_t *mod;
    modbus_device_t *clone;
    if (depth == 1)
    {
        for(mod= _adapter -> devices; mod != NULL; mod = mod -> hh.next)
        {
            clone = clone_modbus_device(_modbus);
            mod -> registers = clone -> registers;
            free(clone); 
        }
        if (DEBUG)
            print_modbus(_adapter -> devices);
    } else if (depth == 2)
    {
        HASH_ADD_STR(_modbus -> registers, ref_name, _modbus_register);
        _modbus_register = new_modbus_register();
    }
    depth--;
}

/* End XML parsing functions */

/**************** XML helper functions ***********************/
static modbus_register_t* clone_modbus_register(modbus_register_t* reg)
{
    modbus_register_t *clone = new_modbus_register();
    strcpy(clone -> name, reg -> name);
    strcpy(clone -> name2, reg -> name2);
    strcpy(clone -> ref_name, reg -> ref_name);
    clone -> mb_start_register = reg -> mb_start_register;
    clone -> n_registers = reg -> n_registers;
    clone -> response_type = reg -> response_type;
    clone -> response_type2 = reg -> response_type2;
    clone -> value = reg -> value;

    return clone;
}
static modbus_device_t* clone_modbus_device(modbus_device_t* wi)
{
    modbus_device_t* clone;
    modbus_register_t* reg, *reg_tmp;

    if (wi == NULL)
        return NULL;
    clone = new_modbus();
    for (reg = wi -> registers; reg; reg = reg -> hh.next)
    {
        reg_tmp = clone_modbus_register(reg);
        HASH_ADD_STR(clone -> registers,ref_name,reg_tmp);
    }

    return clone;
}
/*END XML Helper functions */


