/******************************************************************************
 *  Mortar IO (MIO) Library Command Line Tools
 *  Subscribe Listener
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

#include <expat.h>
#include <stdio.h>
#include <string.h>
#include <strophe.h>
#include <common.h>
#include <mio.h>
#include <mio_handlers.h>
#include <stdlib.h>
#include <signal.h>

mio_response_t *response = NULL;
mio_response_error_t *mio_err;
mio_packet_t *mio_packet;
mio_data_t *mio_data;
mio_transducer_value_t *mio_tran;
mio_conn_t *conn;

void print_usage(char *prog_name) {
	fprintf(stdout,
			"\"%s\" Command line utility to login as a JID and print any received messages\n",
			prog_name);
	fprintf(stdout, "Usage: %s <-u username> <-p password> [-verbose]\n",
			prog_name);
	fprintf(stdout, "Usage: %s -help\n", prog_name);
	fprintf(stdout,
			"\t-u username = JID to authenticate (give the full JID, i.e. user@domain)\n");
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

		if (strcmp(current_arg_name, "-u") == 0) {
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
	} else if (password == NULL ) {
		fprintf(stderr, "Password missing\n");
		print_usage(argv[0]);
		return -1;
	}

	if (verbose) {
		fprintf(stdout, "XMPP Server: %s\n", xmpp_server);
		fprintf(stdout, "XMPP Server Port: %d\n", xmpp_server_port);
		fprintf(stdout, "XMPP PubSub Server: %s\n", pubsub_server);
		fprintf(stdout, "Username: %s\n", username);
		fprintf(stdout, "Verbose: YES\n");
		fprintf(stdout, "\n");
	}

	if (verbose == 0){
		conn = mio_conn_new(MIO_LEVEL_ERROR);
		mio_connect(username, password, NULL, NULL, conn);
	}
	else{
		conn = mio_conn_new(MIO_LEVEL_DEBUG);
		mio_connect(username, password, NULL, NULL, conn);
	}

	while (1) {
		response = mio_response_new();
		mio_pubsub_data_receive(conn, response);
		//This is a handy tool for printing response information
		// For the purpose of illustration, we will manually print the elements
		// mio_response_print(response);
		//Parse the response packet type
		switch (response->response_type) {
		//For commands that require an acknowledgment, it can be checked with MIO_RESPONSE_OK
		case MIO_RESPONSE_OK:
			printf("Request Successful\n");
			break;

			//For errors, the code can be printed as described below
		case MIO_RESPONSE_ERROR:
			printf("Response error: ");
			mio_err = response->response;
			fprintf(stderr, "MIO Error:\n");
			fprintf(stderr, "\tError code: %d\n\tError description: %s\n",
					mio_err->err_num, mio_err->description);
			break;

			//If the response contains more complex data, it is encapsulated in a MIO_RESPONSE_PACKET
		case MIO_RESPONSE_PACKET:
			mio_packet = (mio_packet_t *) response->response;
			//Since there are multiple types of responses, you typically first switch on the type
			if (mio_packet->type == MIO_PACKET_DATA) {
				//MIO_PACKET_DATA contains a list of transducer values that contain data
				mio_data = (mio_data_t *) mio_packet->payload;
				printf("MIO Data Packet:\n\tEvent Node:%s\n", mio_data->event);
				mio_tran = mio_data->transducers;
				//Traverse the linked list of sensor values
				while (mio_tran != NULL ) {
					if (mio_tran->type == MIO_TRANSDUCER_VALUE)
						printf("\t\tTrasducerValue:\n");
					else
						printf("\t\tTrasducerSetValue:\n");

					printf(
							"\t\t\tName: %s\n\t\t\tID: %d\n\t\t\tRaw Value: %s\n\t\t\tTyped Value: %s\n\t\t\tTimestamp: %s\n",
							mio_tran->name, mio_tran->id, mio_tran->raw_value,
							mio_tran->typed_value, mio_tran->timestamp);
					mio_tran = mio_tran->next;
				}
			} else
				printf("Unknown MIO response packet\n");
			break;

		default:
			printf("unknown response type\n");

		}

		mio_response_free(response);
	}

	return 0;
}
