#define USAGE "aggregator [user] [password] [top node] [transducer_id]"

#define ACCUMULATE "accum"
#define A

typedef struct {
    char* id;
    double val;
    UT_hash_handle hh;
} aggregate_element_t;

typedef struct {
    char* top_node;
    char* aggregate_node;
    double aggregate_val;

    aggregate_element_t *elements;
} aggregate_info_t;

aggregate_element_t* get_aggeregate_elements(mio_conn_t *conn, )
{
    aggregate_element_t *aggregates;
    aggregate_element_t *elemnt;

    mio_response_t *response;
    mio_packet_t *packet;
    int err;

    err = mio_reference_query(conn,top_node,response);
    if (err != MIO_OK)
    {
        return NULL;
    }
    packet = (mio_packet_t*) response -> response;

    return aggregates;
}
int main(int argc, char **argv)
{
    char* user;
    char* password;
    char* top_node;
    aggregate_info
    if (arc


}
