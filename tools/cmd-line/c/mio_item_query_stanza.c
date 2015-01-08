/******************************************************************************
 *  Mortar IO (MIO) Library Library Command Line Tools
 *  Event Node Meta Query Tool
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strophe.h>
#include <common.h>
#include <mio.h>
#include <mio_handlers.h>
#include <stdlib.h>
#include <signal.h>

mio_conn_t *conn;
mio_response_t *response = NULL;

void print_usage(char *prog_name) {
	fprintf(stdout,
			"\"%s\"Command line utility to query the payload of an item and print out the raw XML\n",
			prog_name);
	fprintf(stdout,
			"Usage: %s <-event event_node> <-item item_id> <-u username> <-p password> [-verbose]\n",
			prog_name);
	fprintf(stdout, "Usage: %s -help\n", prog_name);
	fprintf(stdout, "\t-event event_node = name of node to query\n");
	fprintf(stdout, "\t-item item_id = ID of the item to retrieve\n");
	fprintf(stdout,
			"\t-u username = JID (give the full JID, i.e. user@domain)\n");
	fprintf(stdout, "\t-p password = JID user password\n");
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
	char *username = NULL;
	char *password = NULL;
	char *xmpp_server = NULL;
	char pubsub_server[80];
	int xmpp_server_port = 5223;

	int verbose = 0;

	int current_arg_num = 1;
	char *current_arg_name = NULL;
	char *current_arg_val = NULL;
	char *event_node = NULL;
	char *item_id = NULL;
	int err, buflen = 0;
	char **buf = NULL;
	xmpp_stanza_t *item;

	struct sigaction sig_int_handler;

	// Add SIGINT handler
	sig_int_handler.sa_handler = int_handler;
	sigemptyset(&sig_int_handler.sa_mask);
	sig_int_handler.sa_flags = 0;
	sigaction(SIGINT, &sig_int_handler, NULL );

	if (argc == 1 || !strcmp(argv[1], "-help")) {
		print_usage(argv[0]);
		return -1;
	}

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
		} else if (strcmp(current_arg_name, "-item") == 0) {
			item_id = current_arg_val;
		} else if (strcmp(current_arg_name, "-u") == 0) {
			username = current_arg_val;
			xmpp_server = _mio_get_server(username);
			if (xmpp_server == NULL ) {
				fprintf(stderr, "Invalid JID, use format user@domain\n");
				return MIO_ERROR_INVALID_JID;
			}
			strcpy(pubsub_server, "pubsub.");
			strcat(pubsub_server, xmpp_server);
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
	} else if (item_id == NULL ) {
		fprintf(stderr, "Item ID missing\n");
		print_usage(argv[0]);
		return -1;
	} else if (password == NULL ) {
		fprintf(stdout, "%s's ", username);
		fflush(stdout);
		password = getpass("password: ");
		if (password == NULL ) {
			fprintf(stderr, "Invalid password\n");
			print_usage(argv[0]);
		}
	}

	if (verbose) {
		fprintf(stdout, "XMPP Server: %s\n", xmpp_server);
		fprintf(stdout, "XMPP Server Port: %d\n", xmpp_server_port);
		fprintf(stdout, "XMPP PubSub Server: %s\n", pubsub_server);
		fprintf(stdout, "Username: %s\n", username);
		fprintf(stdout, "Event Node: %s\n", event_node);
		fprintf(stdout, "Verbose: YES\n");
		fprintf(stdout, "\n");

		conn = mio_conn_new(MIO_LEVEL_DEBUG);
		mio_connect(username, password, NULL, NULL, conn);

	} else {
		conn = mio_conn_new(MIO_LEVEL_ERROR);
		mio_connect(username, password, NULL, NULL, conn);
	}

	response = mio_response_new();
	err = mio_item_recent_get(conn, event_node, response, 1, item_id,
			(mio_handler*) mio_handler_error);
	if (response->response_type == MIO_OK) {
		item = xmpp_stanza_get_child_by_name(
				response->stanza->xmpp_stanza->children->children, "item");
		if (item != NULL ) {
			xmpp_stanza_to_text(item, (char**) &buf, (size_t*) &buflen);
			fprintf(stdout, "%s\n", (char*) buf);
		} else
			fprintf(stderr, "Error reading item\n");
	} else
		mio_response_print(response);
	free(buf);
	mio_response_free(response);
	mio_disconnect(conn);
	mio_conn_free(conn);
	return err;
}
