/******************************************************************************
 *   Mortar IO (MIO) Library
 *  C Strophe Implementation
 *  Copyright (C) 2014, Carnegie Mellon University
 *  All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2.0 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contributing Authors (specific to this file):
 *  Patrick Lazik
 *******************************************************************************/

#ifndef MIO_H_
#define MIO_H_

#include <common.h>
#include <strophe.h>
#include <uthash.h>
#include <pthread.h>
#include <sys/queue.h>
#include <semaphore.h>
#include <expat.h>

#define KEEPALIVE_PERIOD 300000
#define MIO_MAX_OPEN_REQUESTS 	100

#define MIO_BLOCKING 1
#define MIO_NON_BLOCKING 2

#define MIO_REQUEST_TIMEOUT_S		1000
#define MIO_RECONNECTION_TIMEOUT_S	10
#define MIO_CONNECTION_RETRIES	0	// Comment out to retry indefinitely
#define MIO_SEND_RETRIES			3

#define MIO_RESPONSE_TIMEOUT 10000  //ms
#define MIO_EVENT_LOOP_TIMEOUT 1 //ms
#define MIO_SEND_REQUEST_TIMEOUT 1000 //µs
#define MIO_PUBSUB_RX_QUEUE_MAX_LEN 100

// Handler return codes
#define MIO_HANDLER_KEEP 2
#define MIO_HANDLER_REMOVE 3

// Error codes
#define MIO_OK 					1		// Success
#define MIO_ERROR_RUN_THREAD	-1		// Error starting _mio_run_thread
#define MIO_ERROR_CONNECTION	-2		// Connection error
#define MIO_ERROR_DISCONNECTED	-3		// Error because connection not active
#define MIO_ERROR_REQUEST_NOT_FOUND	-4
#define MIO_ERROR_MUTEX			-5
#define MIO_ERROR_COND_WAIT	-6
#define MIO_ERROR_TOO_MANY_OPEN_REQUESTS -7
#define MIO_ERROR_PREDICATE		-8
#define MIO_ERROR_NULL_STANZA_ID	-9
#define MIO_ERROR_XML_NULL_STANZA -10
#define MIO_ERROR_XML_PARSER_ALLOCATION -11
#define MIO_ERROR_XML_PARSING	-12
#define MIO_ERROR_HANDLER_ADD	-13
#define MIO_ERROR_UNKNOWN_PACKET_TYPE -14
#define MIO_ERROR_UNKNOWN_AFFILIATION_TYPE -15
#define MIO_ERROR_UNKNOWN_RESPONSE_TYPE -16
#define MIO_ERROR_TIMEOUT				-17
#define MIO_ERROR_NULL_STANZA			-18
#define MIO_ERROR_ALREADY_SUBSCRIBED	-19
#define MIO_ERROR_RW_LOCK			-20
#define MIO_ERROR_INVALID_JID -21
#define MIO_ERRROR_EVENT_LOOP_NOT_STARTED -22
#define MIO_ERROR_NO_RESPONSE -23
#define MIO_ERROR_DUPLICATE_ENTRY -24
#define MIO_ERROR_NOT_AFFILIATED -25
#define MIO_ERROR_UNEXPECTED_RESPONSE -26
#define MIO_ERROR_UNEXPECTED_PAYLOAD -27
#define MIO_ERROR_INCOMPLETE_ENUMERATION -28
#define MIO_ERROR_REFERENCE_LOOP -29
#define MIO_ERROR_UNKNOWN_META_TYPE -30;
#define MIO_ERROR_PARSER -31;

// Logging defines
typedef enum {
	MIO_LEVEL_ERROR, MIO_LEVEL_WARN, MIO_LEVEL_INFO, MIO_LEVEL_DEBUG
} mio_log_level_t;

#define mio_debug(...) \
        do { if(_mio_log_level == MIO_LEVEL_DEBUG){ fprintf(stderr, "MIO DEBUG: %s:%d:%s(): ", __FILE__, \
                                __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }} while (0)

#define mio_info(...) \
        do { if(_mio_log_level >= MIO_LEVEL_INFO){ fprintf(stderr, "MIO INFO: %s:%d:%s(): ", __FILE__, \
                                __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }} while (0)

#define mio_warn(...) \
		do { if(_mio_log_level >= MIO_LEVEL_WARN){ fprintf(stderr, "MIO WARNING: %s:%d:%s(): ", __FILE__, \
		                        __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }} while (0)

#define mio_error(...) \
		do { if(_mio_log_level >= MIO_LEVEL_ERROR){ fprintf(stderr, "MIO ERROR: %s:%d:%s(): ", __FILE__, \
		                        __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}} while (0)

typedef enum {
	MIO_CONN_CONNECT, MIO_CONN_DISCONNECT, MIO_CONN_FAIL
} mio_conn_event_t;

typedef enum {
	MIO_PRESENCE_UNKNOWN, MIO_PRESENCE_PRESENT, MIO_PRESENCE_UNAVAILABLE
} mio_presence_status_t;

typedef enum {
	MIO_HANDLER, MIO_HANDLER_ID, MIO_HANDLER_TIMED
} mio_handler_type_t;

typedef enum {
	MIO_RESPONSE_UNKNOWN,
	MIO_RESPONSE_OK,
	MIO_RESPONSE_ERROR,
	MIO_RESPONSE_PACKET
} mio_response_type_t;

typedef struct mio_response_error {
	int err_num;
	char *description;
} mio_response_error_t;

typedef struct {
	xmpp_stanza_t *xmpp_stanza;
	char id[37];
} mio_stanza_t;

typedef struct mio_response {
	char id[37];
	char *ns;
	char *name;
	char *type;
	void *response;
	mio_response_type_t response_type;
	TAILQ_ENTRY(mio_response)
	responses;
	mio_stanza_t *stanza;
} mio_response_t;

typedef struct mio_request mio_request_t;

typedef struct {
	xmpp_conn_t *xmpp_conn;
	mio_presence_status_t presence_status;
	pthread_mutex_t event_loop_mutex, send_request_mutex, conn_mutex,
			pubsub_rx_queue_mutex;
	pthread_cond_t send_request_cond, conn_cond;
	TAILQ_HEAD(mio_pubsub_rx_queue, mio_response)
	pubsub_rx_queue;
	sem_t *mio_open_requests;
	pthread_rwlock_t mio_hash_lock;
	mio_request_t *mio_request_table;
	int pubsub_rx_queue_len, pubsub_rx_listening, send_request_predicate,
			conn_predicate, retries;
} mio_conn_t;

typedef void (*mio_handler_conn)(mio_conn_t * conn,
		const mio_conn_event_t event, const mio_response_t *response,
		const void *userdata);

typedef int (*mio_handler)(mio_conn_t * conn, mio_stanza_t * stanza,
		const mio_response_t *response, const void *userdata);

struct mio_request {
	char id[37];	// UUID of sent stanza as key
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int predicate;
	mio_handler handler; // Handler to be called when response is received
	mio_handler_type_t handler_type;
	mio_response_t *response;
	UT_hash_handle hh;
};

typedef struct mio_handler_data {
	mio_request_t *request;
	mio_response_t *response;
	mio_handler handler;
	mio_handler_conn conn_handler;
	mio_conn_t *conn;
	void *userdata;
} mio_handler_data_t;

typedef enum {
	MIO_TRANSDUCER_VALUE, MIO_TRANSDUCER_SET_VALUE
} mio_transducer_value_type_t;

typedef enum {
	MIO_PACKET_UNKNOWN,
	MIO_PACKET_DATA,
	MIO_PACKET_META,
	MIO_PACKET_SUBSCRIPTIONS,
	MIO_PACKET_AFFILIATIONS,
	MIO_PACKET_COLLECTIONS,
	MIO_PACKET_NODE_TYPE,
	MIO_PACKET_SCHEDULE,
	MIO_PACKET_REFERENCES
} mio_packet_type_t;

typedef enum {
	MIO_AFFILIATION_NONE,
	MIO_AFFILIATION_OWNER,
	MIO_AFFILIATION_MEMBER,
	MIO_AFFILIATION_PUBLISHER,
	MIO_AFFILIATION_PUBLISH_ONLY,
	MIO_AFFILIATION_OUTCAST,
	MIO_AFFILIATION_UNKOWN
} mio_affiliation_type_t;

typedef enum {
	MIO_NODE_TYPE_UNKNOWN,
	MIO_NODE_TYPE_LEAF,
	MIO_NODE_TYPE_COLLECTION,
	MIO_NODE_TYPE_EVENT
} mio_node_type_t;

typedef enum {
	MIO_REFERENCE_UNKOWN, MIO_REFERENCE_CHILD, MIO_REFERENCE_PARENT
} mio_reference_type_t;

typedef enum {
	MIO_META_TYPE_UKNOWN, MIO_META_TYPE_DEVICE, MIO_META_TYPE_LOCATION
} mio_meta_type_t;

enum{
	MIO_NO_REFERENCE_AT_CHILD, MIO_ADD_REFERENCE_AT_CHILD
};

typedef struct mio_reference mio_reference_t;

struct mio_reference {
	mio_reference_type_t type;
	char *node;
	char *name;
	mio_meta_type_t meta_type;
	mio_reference_t *next;
};

typedef struct mio_transducer_value mio_transducer_value_t;

struct mio_transducer_value {
	mio_transducer_value_type_t type;
	int id;
	char *name;
	char *typed_value;
	char *raw_value;
	char *timestamp;
	mio_transducer_value_t *next;
};

typedef struct mio_geoloc {
	double *accuracy;
	double *alt;
	char *area;
	double *bearing;
	char *building;
	char *country;
	char *country_code;
	char *datum;
	char *description;
	char *floor;
	double *lat;
	char *locality;
	double *lon;
	char *postal_code;
	char *region;
	char *room;
	double *speed;
	char *street;
	char *text;
	char *timestamp;
	char *tzo;
	char *uri;
} mio_geoloc_t;

typedef struct mio_enum_map_meta mio_enum_map_meta_t;

struct mio_enum_map_meta {
	char *name;
	char *value;
	mio_enum_map_meta_t *next;
};

typedef struct mio_property_meta mio_property_meta_t;

struct mio_property_meta {
	char* name;
	char* value;
	mio_property_meta_t *next;
};

typedef struct mio_transducer_meta mio_transducer_meta_t;

struct mio_transducer_meta {
	char *name;
	char *type;
	char *interface;
	char *manufacturer;
	char *serial;
	char *info;
	char *unit;
	char *min_value;
	char *max_value;
	char *resolution;
	char *precision;
	char *accuracy;
	mio_enum_map_meta_t *enumeration;
	mio_geoloc_t *geoloc;
	mio_property_meta_t *properties;
	mio_transducer_meta_t *next;
};

typedef enum {
	MIO_DEVICE_TYPE_INDOOR_WEATHER,
	MIO_DEVICE_TYPE_OUTDOOR_WEATHER,
	MIO_DEVICE_TYPE_HVAC,
	MIO_DEVICE_TYPE_OCCUPANCY,
	MIO_DEVICE_TYPE_MULTIMEDIA_INPUT,
	MIO_DEVICE_TYPE_MULTIMEDIA_OUTPUT,
	MIO_DEVICE_TYPE_SCALE,
	MIO_DEVICE_TYPE_VEHICLE,
	MIO_DEVICE_TYPE_RESOURCE_CONSUMPTION,
	MIO_DEVICE_TYPE_RESOURCE_GENERATION,
	MIO_DEVICE_TYPE_OTHER
} mio_device_type_t;

typedef enum {
	MIO_UNIT_METER,
	MIO_UNIT_GRAM,
	MIO_UNIT_SECOND,
	MIO_UNIT_AMPERE,
	MIO_UNIT_KELVIN,
	MIO_UNIT_MOLE,
	MIO_UNIT_CANDELA,
	MIO_UNIT_RADIAN,
	MIO_UNIT_STERADIAN,
	MIO_UNIT_HERTZ,
	MIO_UNIT_NEWTON,
	MIO_UNIT_PASCAL,
	MIO_UNIT_JOULE,
	MIO_UNIT_WATT,
	MIO_UNIT_COULOMB,
	MIO_UNIT_VOLT,
	MIO_UNIT_FARAD,
	MIO_UNIT_OHM,
	MIO_UNIT_SIEMENS,
	MIO_UNIT_WEBER,
	MIO_UNIT_TESLA,
	MIO_UNIT_HENRY,
	MIO_UNIT_LUMEN,
	MIO_UNIT_LUX,
	MIO_UNIT_BECQUEREL,
	MIO_UNIT_GRAY,
	MIO_UNIT_SIEVERT,
	MIO_UNIT_KATAL,
	MIO_UNIT_LITER,
	MIO_UNIT_SQUARE_METER,
	MIO_UNIT_CUBIC_METER,
	MIO_UNIT_METERS_PER_SECOND,
	MIO_UNIT_METERS_PER_SECOND_SQUARED,
	MIO_UNIT_RECIPROCAL_METER,
	MIO_UNIT_KILOGRAM_PER_CUBIC_METER,
	MIO_UNIT_CUBIC_METER_PER_KILOGRAM,
	MIO_UNIT_AMPERE_PER_SQUARE_METER,
	MIO_UNIT_AMPERE_PER_METER,
	MIO_UNIT_MOLE_PER_CUBIC_METER,
	MIO_UNIT_CANDELA_PER_SQUARE_METER,
	MIO_UNIT_KILOGRAM_PER_KILOGRAM,
	MIO_UNIT_VOLT_AMPERE_REACTIVE,
	MIO_UNIT_VOLT_AMPERE,
	MIO_UNIT_WATT_SECOND,
	MIO_UNIT_PERCENT,
	MIO_UNIT_ENUM,
	MIO_UNIT_LAT,
	MIO_UNIT_LON
} mio_unit_t;

typedef struct mio_meta {
	char *name;
	char *timestamp;
	char *info;
	mio_meta_type_t meta_type;
	mio_geoloc_t *geoloc;
	mio_transducer_meta_t *transducers;
	mio_property_meta_t *properties;
} mio_meta_t;

typedef struct mio_data {
	char *event;
	int num_transducers;
	mio_transducer_value_t *transducers;
} mio_data_t;

typedef struct mio_subscription mio_subscription_t;

struct mio_subscription {
	char *subscription;
	char *sub_id;
	mio_subscription_t *next;
};

typedef struct mio_affiliation mio_affiliation_t;

struct mio_affiliation {
	char *affiliation;
	mio_affiliation_type_t type;
	mio_affiliation_t *next;
};

typedef struct mio_packet {
	char *id;
	mio_packet_type_t type;
	int num_payloads;
	void *payload;
} mio_packet_t;

typedef struct mio_collection mio_collection_t;

struct mio_collection {
	char *node;
	char *name;
	mio_collection_t *next;
};

typedef struct mio_parser {
	xmpp_ctx_t *ctx;
	XML_Parser expat;
	void *userdata;
	int depth;
	xmpp_stanza_t *stanza;
} mio_parser_t;

typedef struct mio_recurrence{
	char *freq;
	char *interval;
	char *count;
	char *until;
	char *bymonth;
	char *byday;
	char *exdate;
} mio_recurrence_t;

typedef struct mio_event mio_event_t;

struct mio_event {
	int id;
	char *tranducer_name;
	char *transducer_value;
	char *time;
	char *info;
	mio_recurrence_t *recurrence;
	mio_event_t *next;
};

//typedef struct mio_collection mio_collection_t;

// Function Prototypes
int _mio_cond_signal(pthread_cond_t *cond, pthread_mutex_t *mutex,
		int *predicate);
int _mio_cond_broadcast(pthread_cond_t *cond, pthread_mutex_t *mutex,
		int *predicate);
mio_conn_t *mio_conn_new(mio_log_level_t log_level);
mio_parser_t *mio_parser_new(mio_conn_t *conn);
void mio_parser_free(mio_parser_t *parser);
mio_stanza_t *mio_parse(mio_parser_t *parser, char *string);
void mio_conn_free(mio_conn_t *conn);
char *mio_timestamp_create();
void mio_stanza_free(mio_stanza_t *stanza);
mio_stanza_t *mio_stanza_new(mio_conn_t *conn);
mio_stanza_t *mio_stanza_clone(mio_conn_t *conn, mio_stanza_t *stanza);
mio_response_t *mio_response_new();
void mio_response_free(mio_response_t *response);
mio_packet_t *mio_packet_new();
void mio_packet_free(mio_packet_t* packet);
mio_data_t *mio_data_new();
void mio_data_free(mio_data_t *data);
mio_request_t *_mio_request_get(mio_conn_t *conn, char *id);
void mio_packet_payload_add(mio_packet_t *packet, void *payload,
		mio_packet_type_t type);
int mio_response_print(mio_response_t *response);
int mio_data_transducer_value_add(mio_data_t *data,
		mio_transducer_value_type_t type, int id, char *name, char *typed_value,
		char* raw_value, char *timestamp);
mio_transducer_value_t *_mio_transducer_value_tail_get(
		mio_transducer_value_t *t_value);
int mio_connect(char *jid, char *pass, mio_handler_conn conn_handler,
		void *conn_handler_user_data, mio_conn_t *conn);
int mio_disconnect(mio_conn_t *conn);
int mio_handler_add(mio_conn_t * conn, mio_handler handler, const char *ns,
		const char *name, const char *type, mio_request_t *request,
		void *userdata, mio_response_t *response);
int mio_handler_id_add(mio_conn_t * conn, mio_handler handler, const char *id,
		mio_request_t *request, void *userdata, mio_response_t *response);
int mio_handler_timed_add(mio_conn_t * conn, mio_handler handler, int period,
		void *userdata);
void mio_handler_id_delete(mio_conn_t * conn, mio_handler handler,
		const char *id);
mio_stanza_t *mio_pubsub_item_new(mio_conn_t *conn, char* item_type);
mio_stanza_t *mio_pubsub_item_data_new(mio_conn_t *conn);
mio_stanza_t *mio_pubsub_item_interface_new(mio_conn_t *conn, char* interface);
mio_stanza_t *mio_pubsub_set_stanza_new(mio_conn_t *conn, const char *node);
mio_stanza_t *mio_pubsub_get_stanza_new(mio_conn_t *conn, const char *node);
int mio_subscribe(mio_conn_t * conn, const char *node,
		mio_response_t * response);
/*int mio_item_schedule_event_add(mio_conn_t *conn, mio_stanza_t *item,
 const char *transducer_id, const char *typed_val,
 const char *description, struct tm * time);*/
mio_event_t *mio_schedule_event_remove(mio_event_t *events, int id);
void mio_schedule_event_merge(mio_event_t *event_to_update, mio_event_t *event);
mio_event_t * mio_event_new();
void mio_event_free(mio_event_t *event);
mio_event_t *mio_event_tail_get(mio_event_t *event);
int mio_schedule_query(mio_conn_t *conn, const char *event,
		mio_response_t *response);
int mio_schedule_event_merge_publish(mio_conn_t *conn, char *node,
		mio_event_t *event, mio_response_t *response);
int mio_schedule_event_remove_publish(mio_conn_t *conn, char *node, int id,
		mio_response_t *response);
mio_stanza_t *mio_schedule_event_to_item(mio_conn_t* conn, mio_event_t *event);
int mio_item_publish(mio_conn_t *conn, mio_stanza_t *item, const char *node,
		mio_response_t * response);
int mio_item_publish_nonblocking(mio_conn_t *conn, mio_stanza_t *item,
		const char *node);
int mio_item_transducer_value_actuate_add(mio_stanza_t *item,
		const char *deviceID, const char *id, const char *typed_val,
		const char *raw_val, const char *timestamp);
int mio_item_transducer_value_add(mio_stanza_t *item, const char *deviceID,
		const char *id, const char *typed_val, const char *raw_val,
		const char *timestamp);
int mio_item_publish_data(mio_conn_t *conn, mio_stanza_t *item,
		const char *node, mio_response_t * response);

mio_subscription_t *_mio_subscription_new();
void _mio_subscription_free(mio_subscription_t *sub);
mio_subscription_t *_mio_subscription_tail_get(mio_subscription_t *sub);
void _mio_subscription_add(mio_packet_t *pkt, char *subscription, char *sub_id);
int mio_transducer_value_merge_publish(mio_conn_t* conn,
		mio_transducer_value_t *transducers, const char *node,
		mio_response_t *response);
mio_transducer_value_t * mio_transducer_value_new();
void mio_transducer_value_free(mio_transducer_value_t *t_value);
int _mio_send_nonblocking(mio_conn_t *conn, mio_stanza_t *stanza);
mio_response_error_t *_mio_response_error_new();
void _mio_response_error_free(mio_response_error_t* error);
int mio_pubsub_data_receive(mio_conn_t *conn, mio_response_t *response);
mio_transducer_value_t * mio_transducer_value_new();
void mio_transducer_value_free(mio_transducer_value_t *t_value);
mio_affiliation_t *_mio_affiliation_new();
void _mio_affiliation_free(mio_affiliation_t *affiliation);
mio_affiliation_t *_mio_affiliation_tail_get(mio_affiliation_t *affiliation);
void _mio_affiliation_add(mio_packet_t *pkt, char *affiliation,
		mio_affiliation_type_t type);
mio_event_t * _mio_event_new();
void _mio_event_free(mio_event_t *event);
int mio_subscriptions_query(mio_conn_t *conn, const char *node,
		mio_response_t * response);
int mio_acl_affiliations_query(mio_conn_t *conn, const char *node,
		mio_response_t * response);
int mio_acl_affiliation_set(mio_conn_t *conn, const char *node, const char *jid,
		mio_affiliation_type_t type, mio_response_t * response);
int mio_node_create(mio_conn_t * conn, const char *node, const char* title,
		const char* access, mio_response_t * response);
int mio_node_info_query(mio_conn_t * conn, const char *node,
		mio_response_t * response);
int mio_node_delete(mio_conn_t * conn, const char *node,
		mio_response_t * response);
int mio_unsubscribe(mio_conn_t * conn, const char *node, const char *sub_id,
		mio_response_t * response);
int mio_password_change(mio_conn_t * conn, const char *new_pass,
		mio_response_t * response);
int mio_node_register(mio_conn_t *conn, const char *new_node,
		const char *creation_node, mio_response_t * response);
void _mio_handler_data_free(mio_handler_data_t *shd);
char *_mio_get_server(const char *string);
int mio_listen_start(mio_conn_t *conn);
int mio_listen_stop(mio_conn_t *conn);
int mio_collection_node_create(mio_conn_t * conn, const char *node,
		const char *name, mio_response_t * response);
int mio_collection_node_configure(mio_conn_t * conn, const char *node,
		mio_stanza_t *collection_stanza, mio_response_t * response);
mio_stanza_t *mio_node_config_new(mio_conn_t * conn, const char *node,
		mio_node_type_t type);
int mio_collection_config_child_add(mio_conn_t * conn, const char *child,
		mio_stanza_t *collection_stanza);
int mio_collection_config_parent_add(mio_conn_t * conn, const char *parent,
		mio_stanza_t *collection_stanza);
mio_collection_t *_mio_collection_new();
void _mio_collection_free(mio_collection_t *collection);
mio_collection_t *_mio_collection_tail_get(mio_collection_t *collection);
void _mio_collection_add(mio_packet_t *pkt, char *node, char *name);
int mio_collection_children_query(mio_conn_t * conn, const char *node,
		mio_response_t * response);
int mio_collection_parents_query(mio_conn_t * conn, const char *node,
		mio_response_t * response);
int mio_collection_child_add(mio_conn_t * conn, const char *child,
		const char *parent, mio_response_t *response);
void _mio_pubsub_rx_queue_enqueue(mio_conn_t *conn, mio_response_t *response);
mio_response_t *_mio_pubsub_rx_queue_dequeue(mio_conn_t *conn);
int mio_pubsub_data_listen_start(mio_conn_t *conn);
int mio_pubsub_data_listen_stop(mio_conn_t *conn);
void mio_pubsub_data_rx_queue_clear(mio_conn_t *conn);
mio_geoloc_t *mio_geoloc_new();
void mio_geoloc_free(mio_geoloc_t *geoloc);
mio_transducer_meta_t * mio_transducer_meta_new();
void mio_transducer_meta_free(mio_transducer_meta_t *t_meta);
mio_transducer_meta_t *mio_transducer_meta_tail_get(
		mio_transducer_meta_t *t_meta);
mio_property_meta_t *mio_property_meta_tail_get(mio_property_meta_t *p_meta);
int mio_meta_merge_publish(mio_conn_t *conn, char *node, mio_meta_t *meta,
		mio_transducer_meta_t *transducers, mio_property_meta_t *properties,
		mio_response_t *response);
void mio_transducer_meta_add(mio_meta_t *meta, mio_transducer_meta_t *t_meta);
void mio_enum_map_meta_free(mio_enum_map_meta_t *e_meta);
mio_enum_map_meta_t * mio_enum_map_meta_new();
void mio_enum_map_meta_add(mio_transducer_meta_t *t_meta,
		mio_enum_map_meta_t *e_meta);
mio_enum_map_meta_t *mio_enum_map_meta_tail_get(mio_enum_map_meta_t *e_meta);
mio_enum_map_meta_t *mio_enum_map_meta_clone(mio_enum_map_meta_t *e_meta);
mio_property_meta_t *mio_meta_property_remove(mio_property_meta_t *properties,
		char *property_name);
void mio_meta_merge(mio_meta_t *meta_to_update, mio_meta_t *meta);
void mio_meta_transducer_merge(mio_transducer_meta_t *transducer_to_update,
		mio_transducer_meta_t *transducer);
void mio_meta_property_merge(mio_property_meta_t *property_to_update,
		mio_property_meta_t *property);
mio_transducer_meta_t *mio_meta_transducer_remove(
		mio_transducer_meta_t *transducers, char *transducer_name);
int mio_meta_remove_publish(mio_conn_t *conn, char *node, char *device_name,
		char **transducer_names, int num_transducer_names,
		char **property_names, int num_property_names, mio_response_t *response);
mio_stanza_t *mio_geoloc_to_stanza(mio_conn_t* conn, mio_geoloc_t *geoloc);
void mio_geoloc_merge(mio_geoloc_t *geoloc_to_update, mio_geoloc_t *geoloc);
void mio_meta_geoloc_remove(mio_meta_t *meta,
		mio_transducer_meta_t *transducers, char *transducer_name);
int mio_meta_geoloc_remove_publish(mio_conn_t *conn, char *node,
		char *transducer_name, mio_response_t *response);
mio_stanza_t *mio_meta_to_item(mio_conn_t* conn, mio_meta_t *meta);
mio_meta_t *mio_meta_new();
void mio_meta_free(mio_meta_t * meta);
mio_property_meta_t *mio_property_meta_new();
void mio_property_meta_free(mio_property_meta_t *p_meta);
void mio_property_meta_add(mio_meta_t *meta, mio_property_meta_t *p_meta);
mio_property_meta_t *mio_property_meta_tail_get(mio_property_meta_t *p_meta);
int mio_meta_query(mio_conn_t* conn, const char *node, mio_response_t *response);
int mio_item_recent_get(mio_conn_t* conn, const char *node,
		mio_response_t * response, int max_items, const char *item_id,
		mio_handler *handler);
mio_response_t *_mio_response_get(mio_conn_t *conn, char *id);
int mio_stanza_to_text(mio_stanza_t *stanza, char **buf, int *buflen);
mio_response_t *mio_stanza_to_response(mio_conn_t *conn, mio_stanza_t *stanza);
mio_reference_t *mio_reference_new();
void mio_reference_free(mio_reference_t *ref);
mio_reference_t *mio_reference_tail_get(mio_reference_t *ref);
mio_stanza_t *mio_references_to_item(mio_conn_t* conn, mio_reference_t *ref);
void mio_reference_add(mio_conn_t* conn, mio_reference_t *refs,
		mio_reference_type_t type, mio_meta_type_t meta_type, char *node);
mio_reference_t *mio_reference_remove(mio_conn_t* conn, mio_reference_t *refs,
		mio_reference_type_t type, char *node);
int mio_reference_child_remove(mio_conn_t *conn, char* parent, char *child,
		mio_response_t *response);
int mio_reference_child_add(mio_conn_t *conn, char* parent, char *child,
		int add_reference_at_child, mio_response_t *response);
int mio_references_query(mio_conn_t *conn, char *node, mio_response_t *response);
int mio_node_type_query(mio_conn_t * conn, const char *node,
		mio_response_t * response);
int mio_collection_child_remove(mio_conn_t * conn, const char *child,
		const char *parent, mio_response_t *response);
mio_recurrence_t *mio_recurrence_new();
void mio_recurrence_free(mio_recurrence_t *rec);
#endif /* MIO_H_ */
