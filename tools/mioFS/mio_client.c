#include <mio.h>
#include <sys/stat.h>
#include <uthash.h>
#include <utlist.h>
#include <time.h>
#include <pthread.h>

#ifdef __MACH__
#include <sys/time.h>
#endif

#include "mio_client.h"

/* hash table with all the nodes. Is also a tree for collection nodes */
struct mio_node *nodesH = NULL;

/* Root of the file system. Has children for the several types of files below */
struct mio_node *fsRoot = NULL;

/* these are pointers to the head of the lists comming out of the second level mio_node objects
 * These first two levels are not in the hashtable nodesH */

/* List of subscribed? root collection nodes */
struct node_child **l_sub_root_collection_nodes = NULL;
/* List of all the root collection nodes */
struct node_child **l_root_collection_nodes = NULL;
/* List of subscribed nodes */
struct node_child **l_sub_nodes = NULL;

/* debug level */
char mio_debug = 0;

/* how old can data from non-subscribed nodes be before being refreshed */
int data_timeout = 0;

mio_conn_t *conn = NULL;

/*****************************
 * Auxiliary functions
 *****************************/

/* Get current time for linux and macosx
 * Linux has nanosecond precision
 * MacOs has (using this method) microsecond precision */
void get_current_time(struct timespec *ts) {
#ifdef __MACH__
	struct timeval tv;

	gettimeofday( &tv, NULL);
	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000;
#else
	clock_gettime(CLOCK_REALTIME, ts);
#endif
}

/* Compare function for comparing elements in a list with a string id */
int cmp_node_child_str(const struct node_child *a, char *b) {
	return strcmp(a->child->id, b);
}

/*****************************
 * MIO access functions
 *****************************/

/* Determine a nodes type by its id */
int node_id_type(char *id) {
	mio_packet_t *packet;
	int res = -1;
	mio_response_t *response = mio_response_new();
	int err = mio_node_info_query(conn, id, response);

	if (err == MIO_OK && response->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t *) response->response;
		if (packet->type == MIO_PACKET_NODE_TYPE) {
			res = *((mio_node_type_t *) packet->payload);
			if (res != MIO_NODE_TYPE_COLLECTION && res != MIO_NODE_TYPE_LEAF)
				res = -1;
		}
	}
	mio_response_free(response);
	return res;
}

/* Handles response to a subscriptions query
 * For each subscription determines if it is a collection node
 * Creates the structures in the lists and hash tables
 * */
int process_subscriptions_packet(mio_response_t *response) {
	mio_packet_t *packet;
	mio_subscription_t *sub;

	if (response->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t *) response->response;
		if (packet->type == MIO_PACKET_SUBSCRIPTIONS) {
			sub = (mio_subscription_t*) packet->payload;
			while (sub != NULL ) {
				struct mio_node *aux;
				struct node_child *le;
				struct mio_node *node;

				// verify that node is new
				HASH_FIND_STR(nodesH, sub->subscription, aux);
				if (!aux) {
					node = (struct mio_node *) malloc(sizeof(struct mio_node));

					memset(node, 0, sizeof(struct mio_node));
					node->id = strdup(sub->subscription);
					node->type = node_id_type(sub->subscription);

					//check for error
					if (node->type == -1)
						return MIO_ERROR_UNKNOWN_RESPONSE_TYPE;
					node->is_subscribed = 1;

					// insert node into hash table and list of subscribed (collection or not collection) nodes list
					HASH_ADD_KEYPTR(hh, nodesH, node->id, strlen(node->id),
							node);
					le = (struct node_child*) malloc(sizeof(struct node_child));
					le->child = node;
					LL_APPEND(*l_sub_nodes, le);

					//TODO verify if it is a root collection node
					if (node->type == MIO_NODE_TYPE_COLLECTION) {
						le = (struct node_child*) malloc(
								sizeof(struct node_child));
						le->child = node;
						LL_APPEND(*l_sub_root_collection_nodes, le);
					}
				}
				sub = sub->next;
			}
		} else
			return MIO_ERROR_UNKNOWN_PACKET_TYPE;
	} else
		return MIO_ERROR_UNKNOWN_RESPONSE_TYPE;

	return MIO_OK;
}

void process_node_data_packet(struct mio_node *node, mio_response_t *response) {
	mio_packet_t *packet;
	mio_data_t *data;
	mio_transducer_value_t *trans;
	int size, pos;

	//fprintf(stderr,	"Processing data for node %s\n",
	//						data->event ? data->event : "NULL");
//Parse the response packet type
	if (response->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t *) response->response;
		if (packet->type == MIO_PACKET_DATA) {
			//MIO_PACKET_DATA contains a list of transducer values that contain data
			data = (mio_data_t *) packet->payload;
			if (!node && data->event)
				HASH_FIND_STR(nodesH, data->event, node);
			if (!node) {
				// TODO should create a new node or ignore? currently ignoring
				fprintf(stderr,
						"IGNORE. Got data for a node I did not know! %s\n",
						data->event ? data->event : "NULL");
				return;
			}
			//printf("MIO Data Packet:\n\tEvent Node:%s\n", data->event);

			// determine required buff size
			trans = data->transducers;
			size = 1;
			while (trans != NULL ) {
				size += strlen(trans->typed_value) + sizeof("\n") - 1;
				trans = trans->next;
			}
			free(node->data);
			node->data = (char *) malloc(size);
			node->data[0] = '\0';
			pos = 0;

			// copy data
			trans = data->transducers;
			// Set timestamp
			get_current_time(&(node->modification_time));
			//Traverse the linked list of sensor values
			while (trans != NULL ) {
				if (trans->type == MIO_TRANSDUCER_VALUE) {
//						printf("\t\tTrasducerValue:\n");
//						printf(
//								"\t\t\tName: %s\n\t\t\tID: %d\n\t\t\tRaw Value: %s\n\t\t\tTyped Value: %s\n\t\t\tTimestamp: %s\n",
//								trans->name, trans->id, trans->raw_value,
//								trans->typed_value, trans->timestamp);
					pos += snprintf(node->data + pos, size - pos, "%s\n",
							trans->typed_value);
				} //else
				  //printf("\t\tTrasducerSetValue:\n");
				trans = trans->next;
			}
		}
	}
}

/* This function is launched as a new thread.
 * It receives data from subscribed nodes and stores it in the node representation */
void *receive_subscribed_data(void *arg) {
	mio_response_t *response = NULL;

	// TODO handle connection loss to the server
	while (1) {
		response = mio_response_new();
		mio_pubsub_data_receive(conn, response);
		process_node_data_packet(NULL, response);
		mio_response_free(response);
	}

	return NULL ;
}

int mio_client_init(char debug, char *username, char *password, int timeout) {
	mio_response_t *response = NULL;
	int err;
	struct mio_node *n_subs = NULL, *n_subs_col_root = NULL, *n_col_root = NULL;
	struct node_child *l_aux = NULL;
	mio_debug = debug;
	pthread_t subscriptions_thread;
	struct timespec now;

	data_timeout = timeout;

	get_current_time(&now);

// file system root
	fsRoot = (struct mio_node *) malloc(sizeof(struct mio_node));
	memset(fsRoot, 0, sizeof(struct mio_node));
	get_current_time(&(fsRoot->access_time));
	memcpy(&(fsRoot->modification_time), &(fsRoot->access_time),
			sizeof(struct timespec));
	fsRoot->is_subscribed = 1;
	fsRoot->type = MIO_NODE_TYPE_COLLECTION;
	fsRoot->is_miofs_artifact = 1;
	fsRoot->modification_time = fsRoot->access_time = now;

// folder for subscribed nodes
	n_subs = (struct mio_node *) malloc(sizeof(struct mio_node));
	memset(n_subs, 0, sizeof(struct mio_node));
	memcpy(&(n_subs->modification_time), &(fsRoot->access_time),
			sizeof(struct timespec));
	memcpy(&(n_subs->access_time), &(fsRoot->access_time),
			sizeof(struct timespec));
	n_subs->id = strdup("my_subscriptions");
	n_subs->is_subscribed = 1;
	n_subs->type = MIO_NODE_TYPE_COLLECTION;
	l_sub_nodes = &(n_subs->children);
	n_subs->is_miofs_artifact = 1;
	n_subs->modification_time = n_subs->access_time = now;

// folder for subscribed root collection nodes
	n_subs_col_root = (struct mio_node *) malloc(sizeof(struct mio_node));
	memset(n_subs_col_root, 0, sizeof(struct mio_node));
	memcpy(&(n_subs_col_root->modification_time), &(fsRoot->access_time),
			sizeof(struct timespec));
	memcpy(&(n_subs_col_root->access_time), &(fsRoot->access_time),
			sizeof(struct timespec));
	n_subs_col_root->id = strdup("my_collections");
	n_subs_col_root->is_subscribed = 1;
	n_subs_col_root->type = MIO_NODE_TYPE_COLLECTION;
	l_sub_root_collection_nodes = &(n_subs_col_root->children);
	n_subs_col_root->is_miofs_artifact = 1;
	n_subs_col_root->modification_time = n_subs_col_root->access_time = now;

// folder for all root collection nodes
	n_col_root = (struct mio_node *) malloc(sizeof(struct mio_node));
	memset(n_col_root, 0, sizeof(struct mio_node));
	memcpy(&(n_col_root->modification_time), &(fsRoot->access_time),
			sizeof(struct timespec));
	memcpy(&(n_col_root->access_time), &(fsRoot->access_time),
			sizeof(struct timespec));
	n_col_root->id = strdup("collections");
	n_col_root->is_subscribed = 1;
	n_col_root->type = MIO_NODE_TYPE_COLLECTION;
	l_root_collection_nodes = &(n_col_root->children);
	n_col_root->is_miofs_artifact = 1;
	n_col_root->modification_time = n_col_root->access_time = now;

	l_aux = (struct node_child *) malloc(sizeof(struct node_child));
	l_aux->child = n_subs;
	LL_APPEND(fsRoot->children, l_aux);

	l_aux = (struct node_child *) malloc(sizeof(struct node_child));
	l_aux->child = n_subs_col_root;
	LL_APPEND(fsRoot->children, l_aux);

	l_aux = (struct node_child *) malloc(sizeof(struct node_child));
	l_aux->child = n_col_root;
	LL_APPEND(fsRoot->children, l_aux);

	conn = mio_conn_new();

	err = mio_connect(username, password,
			debug ? MIO_LEVEL_DEBUG : MIO_LEVEL_ERROR, NULL, NULL, conn);
	if (err == MIO_OK) {
		response = mio_response_new();
		// learn about subscribed nodes
		err = mio_subscriptions_query(conn, NULL, response);
		if (err == MIO_OK)
			err = process_subscriptions_packet(response);
		mio_response_free(response);
	}

	// start thread to listen to subscription events
	pthread_create(&subscriptions_thread, NULL, receive_subscribed_data, NULL );
	// TODO start thread to listen to data publish events

	return (err == MIO_OK) ? 0 : 1;

}

void mio_cleanup() {
	mio_disconnect(conn);
	mio_conn_free(conn);
}

/*Refresh information on the children of a collection node
 * Returns 0 on success */
int update_children(struct mio_node *node) {
	mio_response_t *response;
	mio_packet_t *packet;
	mio_collection_t *coll;
	struct node_child *l_old = NULL, *l_aux;
	struct mio_node* node_aux;
	int err;
	struct timespec now;

	// verify if node is from MIO or a miofs artifact
	if (node->is_miofs_artifact)
		return 0;

	// use timeout period to avoid reading info every time (calculation ignores ns)
	get_current_time(&now);
	if (node->modification_time.tv_sec + data_timeout > now.tv_sec)
		return 0;
	node->modification_time = now;

	response = mio_response_new();
	err = mio_collection_children_query(conn, node->id, response);

	if (err == MIO_OK && response->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t *) response->response;
		if (packet->type == MIO_PACKET_COLLECTIONS) {
			coll = (mio_collection_t*) packet->payload;
//			fprintf(stdout, "MIO Collections Packet:\n");

			// remove old children in order to verify if they still exist
			l_old = node->children;
			node->children = NULL;

			while (coll != NULL ) {
				// check if the item has a node string. Apparently, empty collection nodes have one item inside!
				if (coll->node) {
//					fprintf(stdout, "\tName: %s\n", coll->name);
//					fprintf(stdout, "\tNode: %s\n", coll->node);
//					fprintf(stdout, "\tJID: %s\n\n", coll->jid);

					// See if this node is one of the old childs
					LL_SEARCH(l_old, l_aux, coll->node, cmp_node_child_str);
					if (l_aux)
						LL_DELETE(l_old, l_aux);
					else {
						// See if node object already exists
						HASH_FIND_STR(nodesH, coll->node, node_aux);
						if (!node_aux) {
							node_aux = (struct mio_node *) malloc(
									sizeof(struct mio_node));

							memset(node_aux, 0, sizeof(struct mio_node));
							node_aux->id = strdup(coll->node);
							node_aux->type = node_id_type(coll->node);
							node_aux->is_subscribed = 0;

							// insert node into hash table
							HASH_ADD_KEYPTR(hh, nodesH, node_aux->id,
									strlen(node_aux->id), node_aux);
						}
						l_aux = (struct node_child*) malloc(
								sizeof(struct node_child));
						l_aux->child = node_aux;
					}
					LL_APPEND(node->children, l_aux);
				}
				coll = coll->next;
			}
			// TODO remove leftover nodes in l_old

			// TODO possible memory leak because nodes which no longer exist will not be removed from nodesH hashtable
			// to correct need to keep reference counter in each node
			err = 0;
		} else
			// TODO set appropriate error code
			err = 1;
	} else
		// TODO set appropriate error code
		err = 1;

	mio_response_free(response);
	return err;
}

/* Verifies if data from a node is up to date
 * Returns 0 on success.
 */
int update_node_data(struct mio_node *node) {
	struct timespec now;
	mio_response_t *response;
	int err = 0;

	printf("updating node ->%s\n", node->id);
	// do not update data for subscribed nodes which have already received data
	if (node->is_subscribed && node->modification_time.tv_sec)
		return 0;
	printf("node is not subscribed or has received no data yet\n");
	// verify if data is already up to date
	get_current_time(&now);
	if (node->modification_time.tv_sec + data_timeout > now.tv_sec)
		return 0;
	node->modification_time = now;
	printf("will have to update\n");
	// read last data from node
	response = mio_response_new();
	err = mio_item_recent_get(conn, node->id, response, 1, NULL);
	process_node_data_packet(node, response);
//	mio_response_print(response);
	mio_response_free(response);
	printf("update done %d\n", err);
	return err;
}

/* Writes value to a transducer
 * Returns 0 on success
 * TODO check for failures
 */
int actuate( struct mio_node *node, const char *transducer, const char* value) {
	char *time_str = NULL;
	mio_stanza_t *item;
	mio_response_t *response;

	time_str = mio_timestamp_create();
	item = mio_pubsub_item_data_new(conn);
	response = mio_response_new();

	mio_item_transducer_value_actuate_add(item, NULL, transducer, transducer, value,
				value, time_str);
	response = mio_response_new();
	mio_item_publish_data(conn, item, node->id, response);
	free(time_str);
	// TODO check response
	//mio_response_print(response);
	mio_response_free(response);
	mio_stanza_free(item);
	return 0;
}
