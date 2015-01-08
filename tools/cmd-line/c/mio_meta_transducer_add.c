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

int err = 0;

mio_conn_t *conn;
mio_response_t *response;

void print_usage(char *prog_name) {
	fprintf(stdout,
			"Usage: %s <-event event_node> <-name transducer_name> [-type transducer_type] [-unit transducer_unit] [-enum_names enumeration_unit_names] [-enum_values enumeration_unit_values] [-min transducer_min_value] [-max transducer_max_value] [-resolution transducer_resolution] [-precision transducer_precision] [-accuracy transducer_accuracy] [-serial transducer_serial_number] [-manufacturer transducer_manufacturer] [-info transducer_info] [-interface transducer_interface] <-u username> <-p password> [-verbose]\n",
			prog_name);
	fprintf(stdout, "Usage: %s -help\n", prog_name);
	fprintf(stdout, "\t-event event_node = name of event node to publish to\n");
	fprintf(stdout, "\t-name transducer_name = name of transducer to add\n");
	fprintf(stdout, "\t-type transducer_type = type of transducer to add\n");
	fprintf(stdout,
			"\t-unit transducer_unit = units of output of transducer to add\n");
	fprintf(stdout,
			"\t-enum_names enumeration_unit_names = comma separated names of enum unit values e.g. \"red,green,blue\"\n");
	fprintf(stdout,
			"\t-enum_names enumeration_unit_values = comma separated values of enum unit values e.g. \"0,1,2\"\n");
	fprintf(stdout,
			"\t-min transducer_min_value = minimum output value of transducer to add\n");
	fprintf(stdout,
			"\t-min transducer_max_value = maximum output value of transducer to add\n");
	fprintf(stdout,
			"\t-resolution transducer_resolution = resolution of output of transducer to add\n");
	fprintf(stdout,
			"\t-precision transducer_precision = precision of output of transducer to add\n");
	fprintf(stdout,
			"\t-accuracy transducer_precision = accuracy of output of transducer to add\n");
	fprintf(stdout,
			"\t-serial transducer_serial_number = serial number of the transducer to add\n");
	fprintf(stdout,
			"\t-manufacturer transducer_manufacturer = manufacturer of the transducer to add\n");
	fprintf(stdout,
			"\t-info trans_info = description of the transducer to add\n");
	fprintf(stdout,
			"\t-interface trans_interface = interface of the transducer to add\n");
	fprintf(stdout,
			"\t-u username = JID (give the full JID, i.e. user@domain)\n");
	fprintf(stdout, "\t-p password = JID user password\n");
	fprintf(stdout, "\t-help = print this usage and exit\n");
	fprintf(stdout, "\t-verbose = print info\n");
}

void int_handler(int s) {
	mio_disconnect(conn);
	mio_conn_free(conn);
	if (response != NULL )
		mio_response_free(response);
	exit(1);
}

char** str_split(char* str, const char delim, int *count) {
	char **ret;
	char *ptr, *str_copy = str;
	int i = 0;

	// Determine how many elements are in str and replace delimeters with \0
	for (*count = 0; str_copy[*count];
			str_copy[*count] == delim ? *count = *count + 1 : *str_copy++)
		;
	*count = *count + 1;
	// Malloc enough space for strings
	ret = malloc(sizeof(char*) * *count);

	// Parse the string again and copy each element into ret
	ptr = strtok(str, &delim);
	while (ptr != NULL ) {
		ret[i] = strdup(ptr);
		ptr = strtok(NULL, &delim);
		i++;
	}
	return ret;
}

int main(int argc, char **argv) {
	char *username = NULL;
	char *password = NULL;
	char *xmpp_server = NULL;
	char pubsub_server[80];
	int xmpp_server_port = 5223;
	char *event_node = NULL;
	char **enum_names = NULL, **enum_values = NULL;
	int enum_names_count = 0, enum_values_count = 0, i;
	mio_transducer_meta_t *trans = mio_transducer_meta_new();
	mio_enum_map_meta_t *e_meta;

	int verbose = 0;

	int current_arg_num = 1;
	char *current_arg_name = NULL;
	char *current_arg_val = NULL;

	struct sigaction sig_int_handler;
	mio_response_t *response;

	// Add SIGINT handler
	sig_int_handler.sa_handler = int_handler;
	sigemptyset(&sig_int_handler.sa_mask);
	sig_int_handler.sa_flags = 0;
	sigaction(SIGINT, &sig_int_handler, NULL );

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
		} else if (strcmp(current_arg_name, "-name") == 0) {
			trans->name = malloc(sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->name, current_arg_val);
		} else if (strcmp(current_arg_name, "-type") == 0) {
			trans->type = malloc(sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->type, current_arg_val);
		} else if (strcmp(current_arg_name, "-enum_names") == 0) {
			enum_names = str_split(current_arg_val, ',', &enum_names_count);
		} else if (strcmp(current_arg_name, "-enum_values") == 0) {
			enum_values = str_split(current_arg_val, ',', &enum_values_count);
		} else if (strcmp(current_arg_name, "-unit") == 0) {
			trans->unit = malloc(sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->unit, current_arg_val);
		} else if (strcmp(current_arg_name, "-min") == 0) {
			trans->min_value = malloc(
					sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->min_value, current_arg_val);
		} else if (strcmp(current_arg_name, "-max") == 0) {
			trans->max_value = malloc(
					sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->max_value, current_arg_val);
		} else if (strcmp(current_arg_name, "-resolution") == 0) {
			trans->resolution = malloc(
					sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->resolution, current_arg_val);
		} else if (strcmp(current_arg_name, "-precision") == 0) {
			trans->precision = malloc(
					sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->precision, current_arg_val);
		} else if (strcmp(current_arg_name, "-accuracy") == 0) {
			trans->accuracy = malloc(
					sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->accuracy, current_arg_val);
		} else if (strcmp(current_arg_name, "-serial") == 0) {
			trans->serial = malloc(
					sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->serial, current_arg_val);
		} else if (strcmp(current_arg_name, "-manufacturer") == 0) {
			trans->manufacturer = malloc(
					sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->manufacturer, current_arg_val);
		} else if (strcmp(current_arg_name, "-interface") == 0) {
			trans->interface = malloc(
					sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->interface, current_arg_val);
		} else if (strcmp(current_arg_name, "-info") == 0) {
			trans->info = malloc(sizeof(char) * (strlen(current_arg_val) + 1));
			strcpy(trans->info, current_arg_val);
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
	} else if (trans->name == NULL ) {
		fprintf(stderr, "Transducer name missing\n");
		print_usage(argv[0]);
		return -1;
	} /*else if (trans->type == NULL ) {
		fprintf(stderr, "Transducer type missing\n");
		print_usage(argv[0]);
		return -1;
	}*/ else if (enum_names_count != enum_values_count) {
		fprintf(stderr, "Enum names and enum values not of same length\n");
		print_usage(argv[0]);
		return -1;
	} /*else if (trans->unit == NULL ) {
		fprintf(stderr, "Transducer units missing\n");
		print_usage(argv[0]);
		return -1;
	}*/ else if (password == NULL ) {
		fprintf(stdout, "%s's ", username);
		fflush(stdout);
		password = getpass("password: ");
		if (password == NULL ) {
			fprintf(stderr, "Invalid password\n");
			print_usage(argv[0]);
			return -1;
		}
	}

	// Add enum map if one was inputted
	if (enum_names_count > 0) {
		for (i = 0; i < enum_names_count; i++) {
			e_meta = mio_enum_map_meta_new();
			e_meta->name = enum_names[i];
			e_meta->value = enum_values[i];
			mio_enum_map_meta_add(trans, e_meta);
		}
		// Overwrite unit to "enum" since enum values are specified
		free(trans->unit);
		trans->unit = malloc(sizeof(char) * 5);
		strcpy(trans->unit, "enum");
	}

	if (verbose) {
		fprintf(stdout, "XMPP Server: f%s\n", xmpp_server);
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
	mio_meta_merge_publish(conn, event_node, NULL, trans, NULL, response);
	mio_response_print(response);
	mio_response_free(response);
	mio_transducer_meta_free(trans);
	mio_disconnect(conn);
	mio_conn_free(conn);
	return MIO_OK;
}

