#include "s2sim.h"
#include <mio.h>
#include <mio_handlers.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

const unsigned int s2sim_message_start = S2SIM_MESSAGE_START;
const unsigned int s2sim_message_end = S2SIM_MESSAGE_END;
const char * client_name = "CMU1";

int s2sim_sys_time = 0;
int s2sim_time_step = 0;
int s2sim_client_id = 0;
int s2sim_price_begin_time = 0;
int s2sim_price_points = 0;
int s2sim_price[128];
int s2sim_num_clients = 0;
int client_consumption = 0;
int seq = 0;

static void *s2sim_receiver(int sock);
char *s2sim_message_to_buf(s2sim_message_t *msg, int *len);
s2sim_message_t *s2sim_message_new();
int s2sim_send(int sock, s2sim_message_t *msg);
void reverse_cpy(char *dst, const char *src, size_t n);
static void *s2sim_consumption_sender(int sock);

int s2sim_connect(char* address, unsigned short port) {
    int sock; /* Socket descriptor */
    struct sockaddr_in serverAddress; /* Echo server address */
    pthread_t receive_thread;
    s2sim_message_t *msg;

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "socket() failed");
        return S2SIM_ERROR;
    }

    /* Construct the server address structure */
    memset(&serverAddress, 0, sizeof(serverAddress)); /* Zero out structure */
    serverAddress.sin_family = AF_INET; /* Internet address family */
    serverAddress.sin_addr.s_addr = inet_addr(address); /* Server IP address */
    serverAddress.sin_port = htons(port); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress))
            < 0) {
        fprintf(stderr, "connect() failed");
        return S2SIM_ERROR;
    }

<<<<<<< HEAD
	/* Connect to S2Sim */
	msg = s2sim_message_new();
	msg->sender_id = S2SIM_NEW_CLIENT;
	msg->receiver_id = S2SIM_ADDRESS;
	msg->message_type = S2SIM_MESSAGE_TYPE_CONN_CONTROL;
	msg->message_id = S2SIM_MESSAGE_ID_SYNC_CLIENT_CONN_REQUEST;
	msg->message_length = strlen(client_name) + 3;
	msg->message_data = malloc(sizeof(char) * strlen(client_name) + 3);
	memcpy(&msg->message_data, &client_name, strlen(client_name));
	s2sim_send(sock, msg);
=======
    /* Connect to S2Sim */
    msg = s2sim_message_new();
    msg->sender_id = S2SIM_NEW_CLIENT;
    msg->receiver_id = S2SIM_ADDRESS;
    msg->message_type = S2SIM_MESSAGE_TYPE_CONN_CONTROL;
    msg->message_id = S2SIM_MESSAGE_ID_SYNC_CLIENT_CONN_REQUEST;
    msg->message_length = strlen(client_name);
    msg->message_data = malloc(sizeof(char) * strlen(client_name));
    memcpy(&msg->message_data, &client_name, strlen(client_name));
    s2sim_send(sock, msg);
>>>>>>> branch 'master' of ssh://mio@mortr.io/~/mio.git

    pthread_create(&receive_thread, NULL, (void *) s2sim_receiver, sock);

    return sock;
}

int s2sim_send(int sock, s2sim_message_t *msg) {
    char *buf;
    int len;
    buf = s2sim_message_to_buf(msg, &len);
    seq++;
    return send(sock, buf, len, 0);
}

s2sim_message_t *s2sim_message_new() {
    s2sim_message_t *msg = malloc(sizeof(s2sim_message_t));
    memset(msg, 0, sizeof(s2sim_message_t));
    return msg;
}

void s2sim_message_free(s2sim_message_t *msg) {
    free(msg);
}

void reverse_cpy(char *dst, const char *src, size_t n) {
    size_t i;

    for (i = 0; i < n; ++i)
        dst[n - 1 - i] = src[i];
}

static void *s2sim_consumption_sender(int sock) {
    s2sim_message_t *msg;
    int i;
    while (1) {
        if (client_consumption > 0) {
            msg = s2sim_message_new();
            msg->message_id = S2SIM_MESSAGE_ID_SYNC_CLIENT_DATA;
            msg->message_type = S2SIM_MESSAGE_TYPE_PRICE_SIGNALING;
            msg->sender_id = s2sim_client_id;
            msg->receiver_id = S2SIM_ADDRESS;
            msg->seq = seq;
            msg->message_data = malloc(sizeof(char) * 4);
            msg->message_length = 4;
            msg->message_data[0] = (client_consumption >> 24) & 0xff;
            msg->message_data[1] = (client_consumption >> 16) & 0xff;
            msg->message_data[2] = (client_consumption >> 8) & 0xff;
            msg->message_data[3] = (client_consumption) & 0xff;

            fprintf(stdout, "Sending consumption of %dW\n", client_consumption);
            //seq = 1;
            s2sim_send(sock, msg);
            s2sim_message_free(msg);
        }
        for (i = 0; i < 60; i++)
            usleep(100000);
    }
}

s2sim_message_t *s2sim_receive_message(int sock) {
    s2sim_message_t *msg;
    int v, i;
    char buf[RCVBUFSIZE];

    v = recv(sock, buf, RCVBUFSIZE - 1, 0);
    if (v < 20)
        return NULL ;

    msg = s2sim_message_new();
    reverse_cpy((char*)&msg->sender_id, &buf[4], 2);
    reverse_cpy((char*)&msg->receiver_id, &buf[6], 2);
    reverse_cpy((char*)&msg->seq, &buf[8], 4);
    memcpy((char*)&msg->seq, (char*)&seq, 4);
    seq++;
    reverse_cpy((char*)&msg->message_type, &buf[12], 2);
    reverse_cpy((char*)&msg->message_id, &buf[14], 2);
    reverse_cpy((char*)&msg->message_length, &buf[16], 4);
    msg->message_data = malloc(sizeof(char) * msg->message_length);
    for (i = 0; i < msg->message_length; i++)
        msg->message_data[i] = buf[i + 20];

    return msg;
}

char *s2sim_message_to_buf(s2sim_message_t *msg, int *len) {
    char* buf = malloc(
                    sizeof(char)
                    * (24 + msg->message_length + msg->message_length % 4));
    memset(buf, 0, 24 + msg->message_length);
    int i, j = 20;

    buf[0] = (s2sim_message_start >> 24) & 0xff;
    buf[1] = (s2sim_message_start >> 16) & 0xff;
    buf[2] = (s2sim_message_start >> 8) & 0xff;
    buf[3] = (s2sim_message_start) & 0xff;

    buf[4] = (msg->sender_id >> 8) & 0xff;
    buf[5] = (msg->sender_id) & 0xff;

    buf[6] = (msg->receiver_id >> 8) & 0xff;
    buf[7] = (msg->receiver_id) & 0xff;

    buf[8] = (seq >> 24) & 0xff;
    buf[9] = (seq >> 16) & 0xff;
    buf[10] = (seq >> 8) & 0xff;
    buf[11] = seq & 0xff;

    buf[12] = (msg->message_type >> 8) & 0xff;
    buf[13] = (msg->message_type) & 0xff;

    buf[14] = (msg->message_id >> 8) & 0xff;
    buf[15] = (msg->message_id) & 0xff;

    for (i = 0; i < msg->message_length; i++) {
        buf[j] = msg->message_data[i];
        j++;
    }

    j += msg->message_length % 4;

    buf[16] = (msg->message_length >> 24) & 0xff;
    buf[17] = (msg->message_length >> 16) & 0xff;
    buf[18] = (msg->message_length >> 8) & 0xff;
    buf[19] = (msg->message_length) & 0xff;

    buf[j] = (s2sim_message_end >> 24) & 0xff;
    buf[j + 1] = (s2sim_message_end >> 16) & 0xff;
    buf[j + 2] = (s2sim_message_end >> 8) & 0xff;
    buf[j + 3] = (s2sim_message_end) & 0xff;

    *len = 24 + msg->message_length + msg->message_length % 4;

    return buf;
}

void s2sim_get_price(int sock) {
    s2sim_message_t *msg = s2sim_message_new();
    msg->sender_id = s2sim_client_id;
    msg->receiver_id = S2SIM_ADDRESS;
    msg->message_length = 0;
    msg->message_type = S2SIM_MESSAGE_TYPE_PRICE_SIGNALING;
    msg->message_id = S2SIM_MESSAGE_ID_GET_PRICE;
    s2sim_send(sock, msg);
    s2sim_message_free(msg);
}

static void *s2sim_receiver(int sock) {
    s2sim_message_t *msg;
    int i, j = 0;
    pthread_t sender_thread;

    while (1) {
        msg = s2sim_receive_message(sock);
        if (msg == NULL )
            continue;

        switch (msg->message_type) {

        case S2SIM_MESSAGE_TYPE_CONN_CONTROL:
            switch (msg->message_id) {

            case S2SIM_MESSAGE_ID_SYNC_CLIENT_CONN_RESPONSE:
                fprintf(stdout,
                        "Received Synchronous Client Connection Response (Message ID: 0x0005)\n");
                if (msg->message_data[3] == 1) {
                    fprintf(stderr,
                            "Connection request not accepted, object ID not found\n");
                    break;
                }
                memcpy(&s2sim_client_id, &msg->receiver_id, sizeof(int));
                reverse_cpy((char*) &s2sim_sys_time, &msg->message_data[4], 4);
                reverse_cpy((char*)&s2sim_num_clients, &msg->message_data[8], 2);
                reverse_cpy((char*)&s2sim_time_step, &msg->message_data[12], 4);
                fprintf(stdout, "Got Client ID: %d\n", s2sim_client_id);
                pthread_create(&sender_thread, NULL,
                               (void *) s2sim_consumption_sender, sock);
                //s2sim_get_price(sock);
                break;

            default:
                fprintf(stderr,
                        "Received message of unknown ID (Message ID: %d)\n",
                        msg->message_id);
                break;
            }
            break;

        case S2SIM_MESSAGE_TYPE_PRICE_SIGNALING:
            switch (msg->message_id) {

            case S2SIM_MESSAGE_ID_SET_CURRENT_PRICE:
                fprintf(stdout,
                        "Received Set Current Price (Message ID: 0x0002)\n");
                reverse_cpy((char*) &s2sim_price_begin_time, &msg->message_data[0], 4);
                reverse_cpy((char*) &s2sim_price_points, &msg->message_data[4], 4);
                j = 8;
                for (i = 0; i < s2sim_price_points; i++) {
                    reverse_cpy((char*) &s2sim_price[i],&msg->message_data[j], 4);
                    j++;
                }
                for (i = 0; i < s2sim_price_points; i++)
                    fprintf(stdout, "Price: %d\n", s2sim_price[i]);
                fprintf(stdout, "\n");
                break;

            default:
                fprintf(stderr,
                        "Received message of unknown ID (Message ID: %d)\n",
                        msg->message_id);
                break;
            }
            break;

        default:
            fprintf(stderr,
                    "Received message of unknown type (Message type: %d)\n",
                    msg->message_type);
            break;
        }
    }
    return NULL;
}

int main() {
    int sock;
    char *input;
    pthread_t *sender_thread;
    sock = s2sim_connect("132.239.95.209", 26999);
    if (sock == S2SIM_ERROR)
        fprintf(stderr, "Connection failed\n");
    else
        fprintf(stdout, "Connection successful\n");

    // Get current price
    //pthread_create(&sender_thread, NULL, (void *) s2sim_consumption_sender,
    //		sock);

    while (1) {
        fflush(stdout);
        input = getpass("");
        if (strcmp(input, "on") == 0)
            fprintf(stdout, "Actuating on\n");
        else if (strcmp(input, "off") == 0)
            fprintf(stdout, "Actuating off\n");
        else
            client_consumption = atoi(input);
    }

    return S2SIM_OK;
}
