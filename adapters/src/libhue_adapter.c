#include "xml_tools.h"
#include "adapter.h"
#include "libfuse_adapter.h"
#include "expat.h"
#include "uthash.h"
#include <strophe.h>
#include <mio_handlers.h>
#include <mio.h>
#include <stdlib.h>
#include <common.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "libhue_adapter.h"
#include "libhue.h"

#define UPDATE_PERIOD 10

typedef struct _bulb_node {
    int bulb_id;
    char* bridge_id;
    char* node_id;
    struct _bulb_node *next;
} _bulb_node_t;

typedef struct _hue_user {
    char* bridge_id;
    char* user;
    struct _hue_user *next;
} _hue_user_t;

typedef struct _hue_context
{
    hue_bridge_t *bridges;
    _hue_user_t *users;
    _bulb_node_t *bulb_nodes;
} _hue_context_t;

/********************** Local Function definitions ********************************/

static void _publish_hue_bulb(adapter_t* adapter, hue_bulb_t *bulb, char* event_id);
_hue_user_t* _find_user(_hue_user_t *users, char* bridge_id);
char* _find_event(_bulb_node_t *bulb_nodes, char* bridge_id, int bulb_id);

/* END local function definitions */



/**************************** public hue functions ************************/

/**
*/
void* hue_data(void* adapterv)
{
    adapter_t *adapter = (adapter_t*) adapterv;
    hue_bridge_t *bridge;
    hue_bulb_t *bulb, *bulb_tmp;
    char *event_id;
    _hue_user_t *user;
    hue_user_t *tmp_user = new_hue_user(NULL,NULL,NULL);
    _hue_context_t *context = (_hue_context_t*) adapter -> context;

    while(1)
    {

        for ( bridge = context -> bridges; bridge != NULL; bridge = bridge -> hh.next)
        {
            user = _find_user(context -> users, bridge -> id);
            if (user == NULL)
            {
                printf("user null\n");
                continue;
            }

            tmp_user -> username = user -> user;
            printf("tmp username %s\n", tmp_user -> username);
            if (bridge -> bulbs == NULL)
            {
                printf("bulbs are null\n");
            }
            for (bulb = bridge -> bulbs; bulb != NULL; bulb = bulb -> next)
            {
                bulb_tmp = get_light_attributes(bridge, tmp_user, bulb -> id);
                event_id = _find_event(context -> bulb_nodes, bridge -> id, bulb -> id);
                printf("here %s\n", event_id);
                _publish_hue_bulb(adapter, bulb_tmp, event_id);
                free(bulb_tmp);
            }
        }


        sleep(UPDATE_PERIOD);
    }
    free(tmp_user);
}

void* hue_actuate(void* adapterv)
{
    adapter_t *adapter = (adapter_t*) adapterv;
    _hue_context_t *context = adapter -> context;
    mio_conn_t* pubsub_conn = adapter -> connection;
    mio_packet_t *packet;
    mio_data_t *data;
    mio_response_t *response;
    mio_transducer_value_t* tran;
    _bulb_node_t *device;
    _hue_user_t *hue_user;
    hue_bridge_t *bridge;
    char *name, *value;
    int err;
    char uuid[512];
    while (1)
    {
        if (pubsub_conn -> xmpp_conn -> state == XMPP_STATE_CONNECTED) {
            response = mio_response_new();
            err = mio_pubsub_data_receive(pubsub_conn, response);
            if (err != MIO_OK)
            {
                printf("mio_pubsub_data_receive failed with error: %d\n", err);
                continue;
            }
            packet = (mio_packet_t*) response->response;
            data = (mio_data_t*) packet->payload;
            for (tran = data->transducers; tran != NULL; tran = tran->next) {
                if (tran -> type != MIO_TRANSDUCER_SET_VALUE)
                    continue;
                else
                {
                    sprintf(uuid,"%s",data -> event);
                    for (device = context -> bulb_nodes;
                            device != NULL; device = device -> next)
                    {
                        //if (strcmp(device -> node_id,data -> event) == 0)
                        if (strcmp(device -> node_id,uuid) == 0)
                            break;

                    }
                    // get user,bridge, and name
                    if (device == NULL)
                        continue;

                    for (hue_user = context -> users;
                            hue_user != NULL; hue_user = hue_user -> next)
                    {
                        if (strcmp(hue_user -> bridge_id, device -> bridge_id) == 0)
                            break;

                    }
                    if (hue_user == NULL)
                        continue;
                    for (bridge = context -> bridges;
                            bridge != NULL; bridge = bridge -> hh.next)
                    {
                        if (strcmp(bridge -> id, device -> bridge_id) == 0)
                            break;

                    }
                    if (bridge == NULL)
                        continue;

                    if (strcmp(tran -> name, "On") == 0)
                    {
                        name = "on\0";
                        if (strcmp(tran -> typed_value,"1") == 0)
                            value = "true\0";
                        else
                            value = "false\0";
                        err =  set_light_state(bridge, hue_user -> user, device -> bulb_id,
                                               name, value);
                    } else if (strcmp(tran -> name, "Brightness") == 0)
                    {
                        name = "bri\0";
                        err =  set_light_state(bridge, hue_user -> user, device -> bulb_id,
                                               name, tran -> typed_value);
                    } else if (strcmp(tran -> name, "Hue") == 0)
                    {
                        name = "hue\0";
                        err =  set_light_state(bridge, hue_user -> user, device -> bulb_id,
                                               name, tran -> typed_value);
                    } else if (strcmp(tran -> name, "Saturation") == 0)
                    {
                        name = "sat\0";
                        err =  set_light_state(bridge, hue_user -> user, device -> bulb_id,
                                               name, tran -> typed_value);
                    } else if (strcmp(tran -> name, "Color Temperature") == 0)
                    {
                        name = "ct\0";
                        err =  set_light_state(bridge, hue_user -> user, device -> bulb_id,
                                               name, tran -> typed_value);
                    } else
                    {
                        continue;
                    }


                    if (err)
                        printf("error with writing to hue bulb\n");

                }

            }

        }
    }
    return NULL;
}

char* _mio_property_value_get(mio_property_meta_t *properties, char* property_name)
{
    mio_property_meta_t *property;
    for (property = properties; property != NULL; property = property -> next)
    {
        if (strcmp(property -> name, property_name) == 0)
            return string_copy(property -> value);
    }
    return NULL;
}
void* hue_parser(void* adapter_v)
{
    adapter_t *adapter = (adapter_t*) adapter_v;
    _hue_context_t *context = malloc(sizeof(_hue_context_t));
    memset(context,0x0,sizeof(_hue_context_t));
    hue_bridge_t *bridge;
    hue_bulb_t *bulb;
    mio_meta_t *bridge_meta, *bulb_meta;
    mio_reference_t *bridge_references, *bulb_references;
    mio_response_t *config_response, *bridge_meta_response,
                   *bridge_reference_response,*bulb_response;
    int err;
    char *type_str, *ip_str, *bridge_id, *bridge_name, *bulb_name;
    _bulb_node_t *bulb_node;
    _hue_user_t *bridge_user;
    int bulb_id;
    mio_packet_t *packet;

    config_response = mio_response_new();

    // get bridge event_nodes
    err = mio_references_query(adapter -> connection, adapter -> config_dir,
                               config_response);
    if (err != MIO_OK)
    {
        printf("error retrieving references\n");
        return NULL;
    }
    packet = (mio_packet_t*) config_response -> response;
    bridge_references = (mio_reference_t*) packet -> payload;

    for ( ; bridge_references != NULL; bridge_references = bridge_references -> next)
    {
        printf("%d%s\n", bridge_references -> type,bridge_references -> name);
        if (bridge_references -> type != MIO_REFERENCE_CHILD)
        {
            printf("continueing\n");
            continue;
        }
        // get bridge meta information
        bridge_meta_response = mio_response_new();
        printf("%s\n", bridge_references -> node);
        err = mio_meta_query(adapter -> connection, bridge_references -> node,
                             bridge_meta_response);

        packet = (mio_packet_t*) bridge_meta_response -> response;
        bridge_meta = (mio_meta_t*) packet -> payload;
        // make sure this is a bridge
        type_str = _mio_property_value_get(bridge_meta -> properties, "type\0");
        if ((type_str == NULL) || (strcmp(type_str,"Hue Bridge\0") != 0))
        {
            mio_response_free(bridge_meta_response);
            continue;
        }
        ip_str = _mio_property_value_get(bridge_meta -> properties, "IP Address\0");
        bridge_id = _mio_property_value_get(bridge_meta -> properties, "Bridge ID\0");
        bridge_name = _mio_property_value_get(bridge_meta -> properties, "Bridge User\0");

        // add bridge, update username table
        bridge = malloc(sizeof(hue_bridge_t));
        memset(bridge,0x0,sizeof(hue_bridge_t));
        bridge -> hh.next = context -> bridges;
        context -> bridges = bridge;

        bridge -> id = bridge_id;
        bridge -> ip = ip_str;

        bridge_user = malloc(sizeof(hue_user_t));
        memset(bridge_user,0x0, sizeof(hue_user_t));
        bridge_user -> next = context -> users;
        context -> users = bridge_user;
        bridge_user -> bridge_id = bridge_id;
        bridge_user -> user = bridge_name;

        // get bulbs associated with bridges
        bridge_reference_response = mio_response_new();
        err = mio_references_query(adapter -> connection, bridge_references -> node,
                                   bridge_reference_response);
        packet = (mio_packet_t *) bridge_reference_response -> response;
        bulb_references = (mio_reference_t*) packet -> payload;

        for (; bulb_references != NULL; bulb_references= bulb_references -> next)
        {
            // make sure event node is a child
            if (bulb_references -> type != MIO_REFERENCE_CHILD)
                continue;

            // get bulb meta
            bulb_response = mio_response_new();

            err = mio_meta_query(adapter -> connection,
                                 bulb_references -> node, bulb_response);
            packet = (mio_packet_t *) bulb_response -> response;
            bulb_meta = (mio_meta_t *) packet -> payload;
            // check type
            mio_response_print(bulb_response);
            type_str = _mio_property_value_get(bulb_meta -> properties, "type\0");
            printf("%s\n",type_str);
            if ((type_str == NULL) || (strcmp(type_str,"Hue Bulb\0") != 0))
            {
                printf("no type\n");
                mio_response_free(bulb_response);
                continue;
            }
            // add  to bulb_nodes
            bulb_name = (_mio_property_value_get(bulb_meta -> properties, "Bulb Name\0"));
            bulb_id = atoi(_mio_property_value_get(bulb_meta -> properties, "Bulb ID\0"));

            bulb = new_hue_bulb();
            bulb -> next = bridge -> bulbs;
            bridge -> bulbs = bulb;
            bulb -> id = bulb_id;
            bulb -> name = bulb_name;

            bulb_node = malloc(sizeof(_bulb_node_t));
            memset(bulb_node, 0x0, sizeof(_bulb_node_t));
            bulb_node -> next = context -> bulb_nodes;
            context -> bulb_nodes = bulb_node;
            bulb_node -> bulb_id = bulb_id;
            bulb_node -> bridge_id = bridge_id;
            bulb_node -> node_id = string_copy(bulb_references -> node);
            mio_response_free(bulb_response);
        }
    }
    mio_response_free(config_response);

    return (void*) context;
}


/********************** HUE utility functions ***************************/

/**
*/
_hue_user_t* _find_user(_hue_user_t *users, char* bridge_id)
{
    _hue_user_t *tmp_user;
    for (tmp_user = users; tmp_user != NULL; tmp_user = tmp_user -> next)
    {
        if (strcmp(bridge_id, tmp_user -> bridge_id) == 0)
            return tmp_user;
    }
    return NULL;
}

char* _find_event(_bulb_node_t* bulb_nodes, char* bridge_id, int bulb_id)
{
    _bulb_node_t *node;
    printf("Bridge ID %s Bulb id %d\n",bridge_id, bulb_id);
    for (node = bulb_nodes; node != NULL; node = node -> next)
    {
        if ((strcmp(node -> bridge_id, bridge_id) == 0) && (node -> bulb_id == bulb_id))
        {
            return  node -> node_id;
        }
    }
    return NULL;
}

/**
*/
static void _publish_hue_bulb(adapter_t* adapter, hue_bulb_t *bulb, char* event_id)
{
    char* time_str;
    char* value = malloc(128);
    //size_t message_len = 0;
    mio_stanza_t *item;
    mio_response_t *publish_response;

    item = mio_pubsub_item_data_new(adapter -> connection);

    time_str = mio_timestamp_create();
    sprintf(value,"%d", bulb -> state -> on);
    mio_item_transducer_value_add(item, NULL, "On", value, value, time_str);
    sprintf(value,"%d", bulb -> state -> bri);
    mio_item_transducer_value_add(item, NULL, "Brightness", value, value, time_str);
    sprintf(value,"%d", bulb -> state -> hue);
    mio_item_transducer_value_add(item, NULL, "Hue", value, value, time_str);
    sprintf(value,"%d", bulb -> state -> sat);
    mio_item_transducer_value_add(item, NULL, "Saturation", value, value, time_str);
    sprintf(value,"%d", bulb -> state -> ct);
    mio_item_transducer_value_add(item, NULL, "Color Temperature", value, value, time_str);
    sprintf(value,"%lf", bulb -> state -> x);
    mio_item_transducer_value_add(item, NULL, "X", value, value, time_str);
    sprintf(value,"%lf", bulb -> state -> y);
    mio_item_transducer_value_add(item, NULL, "Y", value, value, time_str);
    sprintf(value,"%d", bulb -> state -> reachable);
    mio_item_transducer_value_add(item, NULL, "Reachable", value, value, time_str);
    free(value);
    publish_response = mio_response_new();
    while (adapter -> connection -> xmpp_conn -> state != XMPP_STATE_CONNECTED)
        usleep(1000);
    mio_item_publish_data(adapter -> connection,item,event_id,publish_response);
    free(time_str);
    mio_response_free(publish_response);
    mio_stanza_free(item);
}
