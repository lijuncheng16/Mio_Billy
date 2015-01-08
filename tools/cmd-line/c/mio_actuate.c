/******************************************************************************
 *  Mortar IO (MIO) Library Command Line Tools
 *  Publish to Event Node Tool
 * 	C Strophe Implementation
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strophe.h>
#include <common.h>
#include <mio.h>
#include <mio_handlers.h>
#include <stdlib.h>
#include <signal.h>

char *event_node = NULL;
char *t_id = NULL;
char *value = NULL;
char *rawvalue = NULL;
char *interface = NULL;
int err = 0;

mio_conn_t *conn;
mio_response_t *response;

void print_usage(char *prog_name) {
	fprintf(stdout,
			"Usage: %s <-event event_node> <-value transducer_typed_value> <-u username> <-p password> [-int interface] [-id transducer_id] [-rawvalue transducer_raw_value] [-verbose]\n",
			prog_name);
	fprintf(stdout, "Usage: %s -help\n", prog_name);
	fprintf(stdout, "\t-event event_node = name of event node to publish to\n");
	fprintf(stdout,
			"\t-value transducer_typed_value = typed value to actuate\n");
	fprintf(stdout,
			"\t-u username = JID (give the full JID, i.e. user@domain)\n");
	fprintf(stdout, "\t-p password = JID user password\n");
	fprintf(stdout,
			"\t-id transducer_id = id (name) of transducer within message\n");
	fprintf(stdout, "\t-int interface = interface to write data to\n");
	fprintf(stdout,
			"\t-rawvalue transducer_raw_value = raw value to actuate\n");
	fprintf(stdout, "\t-help = print this usage and exit\n");
	fprintf(stdout, "\t-verbose = print info\n");
}

// Handler to catch SIGINT (Ctrl+C)
void int_handler(int s) {
	mio_disconnect(conn);
	mio_conn_free(conn);
	if (response != NULL )
		mio_response_free(response);
	exit(1);
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

	char *time_str = NULL;
	mio_stanza_t *item;

	struct sigaction sig_int_handler;

	// Add SIGINT handler
	sig_int_handler.sa_handler = int_handler;
	sigemptyset(&sig_int_handler.sa_mask);
	sig_int_handler.sa_flags = 0;
	sigaction(SIGINT, &sig_int_handler, NULL );

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

		if (current_arg_num == argc) {
			print_usage(argv[0]);
			return -1;
		}

		current_arg_val = argv[current_arg_num++];

		if (strcmp(current_arg_name, "-event") == 0) {
			event_node = current_arg_val;
		} else if (strcmp(current_arg_name, "-int") == 0) {
			interface = current_arg_val;
		} else if (strcmp(current_arg_name, "-id") == 0) {
			t_id = current_arg_val;
		} else if (strcmp(current_arg_name, "-value") == 0) {
			value = current_arg_val;
		} else if (strcmp(current_arg_name, "-rawvalue") == 0) {
			rawvalue = current_arg_val;
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
		fprintf(stderr, "Event Node missing\n");
		print_usage(argv[0]);
		return -1;
	} else if (value == NULL ) {
		fprintf(stderr, "Value missing\n");
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
	} else if (t_id == NULL ) {
		fprintf(stderr, "Value missing\n");
		print_usage(argv[0]);
		return -1;
	}

	if (verbose) {
		fprintf(stdout, "XMPP Server: %s\n", xmpp_server);
		fprintf(stdout, "XMPP Server Port: %d\n", xmpp_server_port);
		fprintf(stdout, "XMPP PubSub Server: %s\n", pubsub_server);
		fprintf(stdout, "Username: %s\n", username);
		fprintf(stdout, "Event Node: %s\n", event_node);
		fprintf(stdout, "Transducer ID: %s\n", t_id);
		fprintf(stdout, "Verbose: YES\n");
		fprintf(stdout, "\n");

		conn = mio_conn_new(MIO_LEVEL_DEBUG);
		mio_connect(username, password, NULL, NULL, conn);

	} else {
		conn = mio_conn_new(MIO_LEVEL_ERROR);
		mio_connect(username, password, NULL, NULL, conn);
	}

	time_str = mio_timestamp_create();

	if (interface != NULL )
		item = mio_pubsub_item_interface_new(conn, interface);
	else
		item = mio_pubsub_item_data_new(conn);
	mio_item_transducer_value_actuate_add(item, event_node, t_id, value, rawvalue, time_str);
	response = mio_response_new();
	mio_item_publish_data(conn, item, event_node, response);
	free(time_str);
	mio_response_print(response);
	mio_response_free(response);
	mio_stanza_free(item);

	mio_disconnect(conn);
	mio_conn_free(conn);

	return MIO_OK;
}

