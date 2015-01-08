#include "xml_tools.h"
#include "adapter.h"
#include "libfuse_adapter.h"
#include "expat.h"
#include "uthash.h"
#include <strophe.h>
#include <mio_handlers.h>
#include <mio.h>
#include <stdio.h>
#include <stdlib.h>
#include <common.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <curl/curl.h>
#define TRUE 1

/** 
*/
typedef enum {
    watt_minute = 0,
    va = 1,
    kw = 2,
    rated_amperes = 3,
    panel_id = 4,
    location_id = 5,
    number_of_phases = 6,
    minimum_voltage = 7,
    minimum_current = 8,
    maximum_voltage = 9,
    maximum_current = 10
} _regmap_t;

/**
*/
typedef struct _regmap_element
{
    int branch_id; // enfuse branch id
    int branch_reg[11]; // enfuse branch register
    char* branch_node; // event_node
    enfuse_branch_t* branch;
    UT_hash_handle hh;
} _regmap_element_t;

typedef struct _enfuse_context
{
    int location_id;
    enfuse_session_t* session;
    enfuse_branch_t* branches;
    //_regmap_element_t* regmap;
    _regmap_element_t* nodemap;
} _enfuse_context_t;

/********************** Local Function definitions ********************************/

static void _publish_enfuse_data(adapter_t* adapter, enfuse_data_t* data);
static _enfuse_context_t* _new_enfuse_context();

/* parser functions */
static void _fuse_end(void *data, const char *el);
static void _fuse_start(void *data, const char *element, const char **attribute);

/* END local function definitions */


//static pthread_mutex_t* parser_lock = NULL;
/** Local variables for parsing of enfuse configuration*/
static _enfuse_context_t* parser_context = NULL; 
static int parser_depth = 0;
static int parser_location_id = 0;
static int parser_panel_id = 0;
static int parser_branch_id = 0;
static char* parser_node_id = NULL;
static char* parser_username = NULL;
static char* parser_url = NULL;



/**************************** public enfuse functions ************************/

/**
*/
void* libfuse_data(void* adapter)
{
    time_t last_update=time(NULL);
    time_t time_tmp;
    char start_string[512];
    char end_string[512];
    struct tm * last = localtime(&last_update);
    struct tm * gm;
    _enfuse_context_t* context = (_enfuse_context_t*) (((adapter_t*)adapter) -> context);
    enfuse_branch_t *branch;
    enfuse_data_t* data;
    if (last -> tm_min > 2)
    	last -> tm_min = last -> tm_min - 2;
    while(TRUE)
    {
        sprintf(start_string,"%%22%d-%d-%d%%20%d:%d:%d%%22", 
            1900 +last -> tm_year , last -> tm_mon + 1,
            last -> tm_mday, last -> tm_hour, last -> tm_min, last -> tm_sec);
        time(&time_tmp);
        gm = localtime(&time_tmp);
        sprintf(end_string,"%%22%d-%d-%d%%20%d:%d:%d%%22", 
            1900 +gm -> tm_year, (gm -> tm_mon) + 1 , 
            gm -> tm_mday, gm -> tm_hour, gm -> tm_min, gm -> tm_sec);

        for ( branch = context -> branches; branch; branch = branch -> hh.next)
        {
          data = enfuse_get_branch_data(context -> session,2, start_string,end_string, branch -> branch_id);
          if (data == NULL)
          {
             continue;
          }
          _publish_enfuse_data((adapter_t*)adapter,data);
          release_enfuse_data(data);
        }
	
        last_update = time_tmp;
        last = localtime(&last_update);
        sleep(60 );
    }
}

/**
*/
void* libfuse_parser(void* adapterv)
{
    adapter_t *adapter = (adapter_t*) adapterv;
    char* config_dir = adapter -> config_dir;
    _enfuse_context_t* context;
    parser_context = _new_enfuse_context();

    char fuse_file[512];
    sprintf(fuse_file,"%s/network.xml",(char*)config_dir);

    parser_depth = 0;
    parse_xml(fuse_file, _fuse_start, _fuse_end);

    context = parser_context;
    parser_context = NULL;
    return (void*) context;
}

/* enfuse functions */

/********************** ENFUSE utility functions ***************************/

/**
*/
static _enfuse_context_t* _new_enfuse_context()
{
    _enfuse_context_t* context = (_enfuse_context_t*) malloc(sizeof(_enfuse_context_t));
    memset(context,0x0,sizeof(_enfuse_context_t));
    return context;
}

char* date_to_mio_timestamp(char* date_stamp)
{
    char *time;
    time_t t;
    struct tm *gmt;
    printf("%s\n",date_stamp);
    if (sscanf(date_stamp,"\\/Date(%lu-0000)\\/",&t) < 0)
    {
       return mio_timestamp_create();
    }
    t = t/1000;
    gmt = localtime(&t);
    if (gmt == NULL)
        return NULL;

    if( (time = malloc(100)) == NULL)
        return NULL;
    snprintf(time,100,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2d-0500", 1900 +gmt -> tm_year, gmt -> tm_mon + 1, gmt -> tm_mday, 
        gmt -> tm_hour, gmt -> tm_min, gmt -> tm_sec);
    //free(gmt);
    return time;
}

/**
*/
static void _publish_enfuse_data(adapter_t* adapter, enfuse_data_t* data)
{
    int i;
    int* regs;
    char* node_id;
    char* name;
    char* time_str;
    char* value;
    //size_t message_len = 0;
    mio_stanza_t *item = NULL;
    mio_response_t *publish_response = NULL;
    _regmap_element_t *element = NULL;
    enfuse_data_t* d = NULL;
    _enfuse_context_t* context = (_enfuse_context_t*) adapter -> context;
    printf("publishing\n");
    for (d = data; d != NULL; d = d -> hh.next)
    {

        for (element = context -> nodemap; element != NULL; element = element -> hh.next)
        {
            if (element -> branch_id == d -> branch_id)
                break;
        }

        if ( element == NULL)
        {
            printf("element is NULL %d\n", d -> branch_id);
            continue;
        }
        item = mio_pubsub_item_data_new(adapter -> connection);
        node_id = element -> branch_node;
        regs = element -> branch_reg;

        for (i = 0; i  < 11; i++)
        {
            if (i == (_regmap_t) watt_minute)
            {
                value = malloc(20);
                name = malloc(50);
                sprintf(value,"%f",d -> watt_minute);
                sprintf(name,"Watt Minutes");
            } else if ( i == (_regmap_t) va)
            {
                value = malloc(20);
                name = malloc(50);
                sprintf(value,"%f",d -> va);
                sprintf(name,"va");
            } else if ( i == (_regmap_t) kw)
            {
                value = malloc(20);
                name = malloc(50);
                sprintf(value,"%f",d -> kw);
                sprintf(name,"kw");
            } /*else if (i == (_regmap_t) panel_id)
            {
                value = malloc(10);
                name = malloc(50);
                sprintf(value,"%d", element -> branch -> panel_id);
                sprintf(name,"Panel ID");
            } else if (i == (_regmap_t) location_id)
            {
                value = malloc(10);
                name = malloc(50);
                sprintf(value,"%d", context -> location_id);
                sprintf(name,"Location ID");
            }*/ else if (i == (_regmap_t) minimum_voltage)
            {
                value = malloc(20);
                name = malloc(50);
                sprintf(value,"%f", d -> min_voltage);
                sprintf(name,"Minimum Voltage");
            } else if (i == (_regmap_t) minimum_current)
            {
                value = malloc(20);
                name = malloc(50);
                sprintf(value,"%f", d -> min_current);
                sprintf(name,"Minimum Current");
            } else if (i == (_regmap_t) maximum_voltage)
            {
                value = malloc(20);
                name = malloc(50);
                sprintf(value,"%f", d -> max_voltage);
                sprintf(name,"Maximum Voltage");
            } else if (i == (_regmap_t) maximum_current)
            {
                value = malloc(20);
                name = malloc(50);
                sprintf(value,"%f", d -> max_current);
                sprintf(name,"Maximum Current");
            } else {
                continue;
            }
            time_str = date_to_mio_timestamp(d -> local_date_stamp);
            mio_item_transducer_value_add(item, NULL, name, value, value, time_str);
            free(value);
            free(name);
            free(time_str);
        }
        publish_response = mio_response_new();
        while (adapter -> connection -> xmpp_conn -> state != XMPP_STATE_CONNECTED)
           usleep(1000);
        mio_item_publish_data(adapter -> connection,item,node_id,publish_response);
        mio_response_free(publish_response);
        mio_stanza_free(item);
    }
    printf("done publishing\n");
}

/* END Enfuse Utility functions */

/************************** Configuration parsing functions **********************/

/**
*/
static void _fuse_start(void *data, const char *element, const char **attribute)
{
    int32_t i;
    parser_depth++;
    for (i = 0; attribute[i]; i += 1)
    {
        if (parser_depth == 1)
        {
            if (strcmp("user",attribute[i]) == 0)
            {
                parser_username = string_copy(attribute[i+1]);

                char* pass = getpass("Enfuse Password: ");
                parser_context -> session = enfuse_authenticate(parser_username,pass);
                free(parser_username);
                free(pass);
                i++;
            } else if (strcmp("url",attribute[i]) == 0)
            {
                parser_url = string_copy(attribute[i+1]);
                i++;
            }
        } else if (parser_depth == 2)
        {
            if(strcmp("location_id",attribute[i]) == 0)
            {

                parser_location_id = atoi(attribute[i+1]);
                i++;
            } else if(strcmp("panel_id",attribute[i]) == 0)
            {
                parser_panel_id = atoi(attribute[i+1]);
                i++;
            }
        } else if (parser_depth == 3)
        {
            if(strcmp("node_id",attribute[i]) == 0)
            {
                parser_node_id = string_copy(attribute[i+1]);
                i++;
            } else if (strcmp("branch_id",attribute[i]) == 0)
            {
                parser_branch_id = atoi(attribute[i+1]);
                i++;
            }
        }
    }
}

/**
*/
static void _fuse_end(void *data, const char *el)
{
    enfuse_branch_t* new_branch;
    _regmap_element_t* element;
    if (parser_depth == 1)
    {
        parser_context -> location_id = parser_location_id;
    } else if (parser_depth == 3)
    {
        /*if ((new_branch = enfuse_get_branch(parser_context -> session,parser_branch_id))
            == NULL)
        {
           parser_depth--;
           return;
        }*/
        new_branch = malloc(sizeof(enfuse_branch_t));
        memset(new_branch,0x0,sizeof(enfuse_branch_t));
        new_branch -> branch_id = parser_branch_id;
        new_branch -> node_id = parser_node_id;
        HASH_ADD_INT(parser_context -> branches, branch_id, new_branch);

        element = (_regmap_element_t*) malloc(sizeof(_regmap_element_t));
        element -> branch_id = new_branch -> branch_id;
        element -> branch = new_branch;
        element -> branch_node = string_copy(new_branch -> node_id);

        HASH_ADD_KEYPTR(hh, parser_context -> nodemap, element -> branch_node, strlen(element-> branch_node),element);
        parser_branch_id = 0;
        parser_panel_id = 0;
        parser_node_id = 0;
        parser_location_id = 0;
    }
    parser_depth--;
}

/* END  enfuse parsing functions */
