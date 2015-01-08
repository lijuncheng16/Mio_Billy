#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strophe.h>
#include <common.h>
#include <sox.h>
#include <sox_handlers.h>
#include <expat.h>

/* Command line tool to publish the Creation of a new Event Node to the CreationNode */

char *event_node = NULL;
char *created_node = NULL;

int err = 0;

void print_usage(char *prog_name) {

	fprintf(stdout,
			"Usage: %s <-u username> <-p password> <-event event_node> <-creation created_node> [-verbose]\n",
			prog_name);
	fprintf(stdout, "Usage: %s -help\n", prog_name);
	fprintf(stdout,
			"\t-u username = JID (give the full JID, i.e. user@domain)\n");
	fprintf(stdout, "\t-p password = JID user password\n");
	fprintf(stdout,
			"\t-creation created_node = name of the event node/UUID whose creation is to be published\n");
	fprintf(stdout,
			"\t-event event_node = name of the corresponding CreationNode\n");
	fprintf(stdout, "\t-help = print this usage and exit\n");
	fprintf(stdout, "\t-verbose = print info\n");
}

void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status,
		const int error, xmpp_stream_error_t * const stream_error,
		void * const userdata) {

	int ret = 0;
	int XMPP_NO_ERROR = 0;
	char buf[2048];
	time_t timestamp;
	sox_userdata_t *ud = (sox_userdata_t *) userdata;
	xmpp_stanza_t *item = sox_new_pubsub_item(conn->ctx);
	char *id;
	char *suffix;
	char *new_event;
	char time_str[30];

	sox_create_timestamp(time_str);

	//Append _meta suffix
	new_event = (char *) malloc((strlen(event_node) + 6) * sizeof(char));
	if ((suffix = strchr(event_node, '_')) != NULL )
		snprintf(new_event, strlen(event_node) - strlen(suffix) + 1, "%s",
				event_node);
	else
		strcpy(new_event, event_node);
	strcat(new_event, "_data");  //Not sure whether to keep this line of code

	//Create id for iq stanza consisting of "pub_" and node name
	//String length is length of server address + length of "pub_" + terminating character
	id = (char *) malloc((strlen(new_event) + 5) * sizeof(char));
	strcpy(id, "pub_");
	strcat(id, new_event);

	if (status == XMPP_CONN_CONNECT) {

		xmpp_id_handler_add(conn, mio_handler_publish, id, ud);

		if ((err = sox_item_add_device(item, created_node, "FIREFLY", "unknown",
				"unknown", time_str)) < 0) // Adding sensor type as FIREFLY by default.
			fprintf(stderr,
					"ERROR: Could not publish Event Node Creation of %s, error code: %i\n",
					created_node, err);

		else if ((err = mio_item_publish(conn, item, new_event)) < 0)
			fprintf(stderr, "ERROR: Could not publish item, error code: %i\n",
					err);

	} else {
		fprintf(stderr, "ERROR: Connection failed!\n");
		xmpp_disconnect(conn);
		xmpp_stop(conn->ctx);
		sox_end_conn(conn);
	}

}

int main(int argc, char **argv) {

	char domain[80];
	char *result = NULL;
	char *username = NULL;
	char *password = NULL;
	char xmpp_server[80];
	char pubsub_server[80];
	int xmpp_server_port = 5223;

	int verbose = 0;
	int current_arg_num = 1;
	char *current_arg_name = NULL;
	char *current_arg_val = NULL;

	xmpp_conn_t *conn = NULL;

	strcpy(xmpp_server, "none");
	strcpy(pubsub_server, "none");

	while (current_arg_num < argc) {
		current_arg_name = argv[current_arg_num++];

		if (strcmp(current_arg_name, "-help") == 0) {
			print_usage(argv[0]);
			return -1;
		}

		if (strcmp(current_arg_name, "-verbose") == 0) {
			verbose = 1;
			continue;
		}

		if (current_arg_name == argc) {
			print_usage(argv[0]);
			return -1;
		}

		current_arg_val = argv[current_arg_num++];

		if (strcmp(current_arg_name, "-event") == 0) {
			event_node = current_arg_val;
		} else if (strcmp(current_arg_name, "-creation") == 0) {
			created_node = current_arg_val;
		} else if (strcmp(current_arg_name, "-u") == 0) {
			username = current_arg_val;
			if (strcmp(xmpp_server, "none") == 0) {
				strcpy(domain, username);
				result = strtok(domain, "@");
				result = strtok(NULL, "@");
				strcpy(xmpp_server, result);
				strcpy(pubsub_server, "pubsub.");
				strcat(pubsub_server, xmpp_server);
			}
		} else if (strcmp(current_arg_name, "-p") == 0) {
			password = current_arg_val;
		} else {
			fprintf(stderr, "Unknown argument: %s\n", current_arg_name);
			print_usage(argv[0]);
			return -1;
		}
	}

	if (username == NULL ) {
		fprintf(stderr, "Username missing\n");
		print_usage(argv[0]);
		return -1;
	} else if (event_node == NULL ) {
		fprintf(stderr, "Name of CreationNode missing\n");
		print_usage(argv[0]);
		return -1;
	} else if (created_node == NULL ) {
		fprintf(stderr, "Name of created event node missing\n");
		print_usage(argv[0]);
		return -1;
	} else if (password == NULL ) {
		fprintf(stdout, "%s's ", username);
		fflush(stdout);
		password = getpass("password: ");
		if (password == NULL ) {
			fprintf(stderr, "Invalid password\n");
			print_usage(argv[0]);
			return -1;
		}
	}

	if (verbose) {
		fprintf(stdout, "XMPP Server: %s\n", xmpp_server);
		fprintf(stdout, "XMPP Server Port: %d\n", xmpp_server_port);
		fprintf(stdout, "XMPP PubSub Server: %s\n", pubsub_server);
		fprintf(stdout, "Username: %s\n", username);
		fprintf(stdout, "Creation Node: %s\n", event_node);
		fprintf(stdout, "Created Node: %s\n", created_node);
		fprintf(stdout, "Verbose: YES\n");
		fprintf(stdout, "\n");

		if ((err = sox_new_conn(username, password, conn_handler,
				XMPP_LEVEL_DEBUG, conn)) < 0)
			fprintf(stderr, "ERROR: Connection failed\n");

	} else if ((err = sox_new_conn(username, password, conn_handler,
			XMPP_LEVEL_ERROR, conn)) < 0)
		fprintf(stderr, "ERROR: Connection failed\n");

	xmpp_shutdown();

	return err;
}

