#include "uthash.h"
#include "mio.h"
#include "mio_handlers.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define USAGE "aggregator [user] [password] [transducer_id] [aggregate node]"


typedef struct {
    char id [100];
    double val;
    UT_hash_handle hh;
} aggregate_element_t;

typedef struct {
    char* top_node;
    char* aggregate_node;
    char* tran_id;
    double aggregate_val;
    double value;

    aggregate_element_t *elements;
} aggregate_info_t;


double contains_tran(mio_data_t *data, char* transducer_id)
{
    double value;
    mio_transducer_value_t *tran;
    for (tran = data->transducers; tran != NULL; tran = tran->next) {
        if (tran -> type != MIO_TRANSDUCER_VALUE)
            continue;
        if (strcmp(tran -> name, transducer_id) == 0)
        {
            sscanf(tran -> typed_value, "%lf",&value);
            return value;
        }
    }
    return -1;
}
int main(int argc, char **argv)
{
    mio_conn_t *conn;
    mio_data_t *data;
    mio_response_t *response;
    mio_packet_t *packet;
    double val, prev_val;
    aggregate_info_t *aggs;
    aggregate_element_t *agg_tmp;
    mio_stanza_t *item;
    long t;
    long last_time = time(NULL);
    char time_str[100];
    const char uuid[100];
    char strval[100];
    int err;

    if (argc != 5)
    {
        printf(USAGE);
        return 0;
    }
    conn = mio_conn_new(MIO_LEVEL_ERROR);
    mio_connect(argv[1],argv[2],NULL,NULL,conn);

    aggs = malloc(sizeof(aggregate_info_t));
    memset(aggs,0x0,sizeof(aggregate_info_t));
    aggs -> tran_id = argv[3];
    aggs -> aggregate_node = argv[4];

    while (1)
    {
        if (conn -> xmpp_conn -> state == XMPP_STATE_CONNECTED) {
            response = mio_response_new();
            err = mio_pubsub_data_receive(conn, response);
            if (err != MIO_OK)
            {
                printf("mio_pubsub_data_receive failed with error: %d\n", err);
                continue;
            }
            packet = (mio_packet_t*) response->response;
            data = (mio_data_t*) packet->payload;
            if (( val = contains_tran(data, aggs -> tran_id)) == -1)
                continue;
            if (aggs -> elements == NULL)
            {
                agg_tmp = malloc(sizeof(aggregate_element_t));
                memset(agg_tmp,0x0,sizeof(aggregate_element_t));
                strcpy(agg_tmp -> id, data -> event);
                //HASH_ADD_STR(aggs -> elements, id, agg_tmp);
                aggs -> elements = agg_tmp;
                prev_val = 0;
            } else
            {
                //HASH_FIND_STR(aggs -> elements, data -> event, agg_tmp);
                for (agg_tmp = aggs -> elements; agg_tmp != NULL; agg_tmp = agg_tmp -> hh.next)
                {
                  if (strcmp(data -> event, agg_tmp -> id) == 0)
                    break;
                }

                if (agg_tmp == NULL)
                {
                    agg_tmp = malloc(sizeof(aggregate_element_t));
                    memset(agg_tmp,0x0,sizeof(aggregate_element_t));
                    strcpy(agg_tmp -> id, data -> event);
                    //HASH_ADD_STR(aggs -> elements, id, agg_tmp);
                    agg_tmp -> hh.next = aggs -> elements;
                    aggs -> elements = agg_tmp;
                    prev_val = 0;
                } else
                {
                    prev_val = agg_tmp -> val;
                }
            }
            strcpy(time_str, data -> transducers -> timestamp);
            agg_tmp -> val = val;

            aggs -> value += (val - prev_val);

            sprintf(strval, "%lf", aggs -> value);
            mio_response_free(response);

            t = time(NULL);
            if ((t - last_time) > 60)
            {
            response = mio_response_new();
            item = mio_pubsub_item_data_new(conn);

            mio_item_transducer_value_add(item, NULL, "Aggregate", strval, strval,
                                          time_str);
            while (conn-> xmpp_conn -> state != XMPP_STATE_CONNECTED)
                usleep(1000);
            mio_item_publish_data(conn,item, aggs -> aggregate_node, response);
            mio_response_free(response);
            mio_stanza_free(item);
            last_time = t;
            }
        } else
            usleep(1000);
    }
    return 0;
}
