/******************************************************************************
 *  Mortar IO (MIO) Library Command Line Tools
 *  Event Node Configure Tool
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

char *title = NULL;
char *access_model = NULL;
char *event_node = NULL;

void print_usage(char *prog_name) {
	fprintf(stdout,
			"\"%s\" Command line utility to condigure the access_model control list of an event node\n",
			prog_name);
	fprintf(stdout,
			"Usage: %s <-event event_node> [-title node_title] [-access_model access_model_model] <-u username> <-p password> [-verbose]\n",
			prog_name);
	fprintf(stdout, "Usage: %s -help\n", prog_name);
	fprintf(stdout, "\t-event event_node = name of event node to create\n");
	fprintf(stdout, "\t-title node_title = title of event node\n");
	fprintf(stdout,
			"\t-access access_model = access model of the node, either \"open\", \"whitelist\", \"presence\" or \"roster\"\n");
	fprintf(stdout,
			"\t-u username = JID (give the full JID, i.e. user@domain)\n");
	fprintf(stdout, "\t-p password = JID user password\n");
	fprintf(stdout, "\t-help = print this usage and exit\n");
	fprintf(stdout, "\t-verbose = print info\n");
}

void conn_handler(mio_conn_t * const conn, const mio_conn_event_t status,
		mio_userdata_t * const userdata) {

	userdata->ttl = 1;
	if ((userdata->return_value = mio_acl_node_config(conn, event_node, title,
			access_model, userdata)) < 0)
		fprintf(stderr,
				"ERROR: Subscription request stanza could not be sent, error code: %i\n",
				userdata->return_value);
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

	mio_userdata_t userdata;
	mio_conn_t *conn;

	if (argc == 1 || !strcmp(argv[1], "-help")) {
		print_usage(argv[0]);
		return -1;
	}

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

		if (strcmp(current_arg_name, "-title") == 0) {
			title = current_arg_val;
		} else if (strcmp(current_arg_name, "-access") == 0) {
			access_model = current_arg_val;
		} else if (strcmp(current_arg_name, "-event") == 0) {
			event_node = current_arg_val;
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
	} else if (access_model == NULL && title == NULL ) {
		fprintf(stderr, "access model or title required\n");
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
		fprintf(stdout, "Event Node: %s\n", event_node);
		fprintf(stdout, "Verbose: YES\n");
		fprintf(stdout, "\n");

		conn = mio_connect(username, password, (mio_handler_conn) conn_handler,
				MIO_LEVEL_DEBUG, &userdata);
		mio_run(conn, NULL );
		if (userdata.return_value < 0)
			fprintf(stderr, "ERROR: Connection failed\n");

	} else {
		conn = mio_connect(username, password, (mio_handler_conn) conn_handler,
				MIO_LEVEL_ERROR, &userdata);
		mio_run(conn, NULL );
		if (userdata.return_value < 0)
			fprintf(stderr, "ERROR: Connection failed\n");
	}

	return userdata.return_value;
}

