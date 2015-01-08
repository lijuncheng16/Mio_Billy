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
			"Usage: %s <-event event_node> <-d_name device_name or -t_name transduce_name> [-accuracy accuracy] [-alt altitude] [-area area] [-bearing bearing] [-building building] [-country country] [-country_code country_code] [-datum datum] [-description description] [-floor floor] [-lat lattitude] [-locality locality] [-lon longitude] [-zip zip_code] [-region region] [-room room] [-speed speed] [-street street] [-text text] [-tzo time_zone_offset] [-uri map_uri] <-u username> <-p password> [-verbose]\n",
			prog_name);
	fprintf(stdout, "Usage: %s -help\n", prog_name);
	fprintf(stdout,
			"\t-event event_node = name of event node to publish to e.g. \"-07:00\"\n");
	fprintf(stdout,
			"\t-t_name transduce_name = name of transducer to add geoloc (if no transducer is specified, geoloc will be added to device registered with the event node)\n");
	fprintf(stdout,
			"\t-accuracy accuracy = horizontal GPS error in meters (decimal) e.g. \"10\"\n");
	fprintf(stdout,
			"\t-alt altitude = altitude in meters above or below sea level (decimal) e.g. \"1609\"\n");
	fprintf(stdout,
			"\t-area area = a named area such as a campus or neighborhood e.g. \"Central Park\"\n");
	fprintf(stdout,
			"\t-bearing bearing = GPS bearing (direction in which the entity is heading to reach its next waypoint), measured in decimal degrees relative to true north (decimal) e.g. \"-07:00\"\n");
	fprintf(stdout,
			"\t-building building = a specific building on a street or in an area e.g. \"The Empire State Building\"\n");
	fprintf(stdout,
			"\t-country country = the nation where the device/transducer is located e.g. \"United States\"\n");
	fprintf(stdout,
			"\t-country_code country_code = the ISO 3166 two-letter country code e.g. \"US\"\n");
	fprintf(stdout, "\t-datum datum = GPS datum e.g. \"-07:00\"\n");
	fprintf(stdout,
			"\t-description description = a natural-language name for or description of the location e.g. \"Bill's house\"\n");
	fprintf(stdout,
			"\t-floor floor = a particular floor in a building e.g. \"102\"\n");
	fprintf(stdout,
			"\t-lat latitude = latitude in decimal degrees north (decimal) e.g. \"39.75\"\n");
	fprintf(stdout,
			"\t-locality locality = a locality within the administrative region, such as a town or city e.g. \"New York City\"\n");
	fprintf(stdout,
			"\t-lon longitude = longitude in decimal degrees East (decimal) e.g. \"-104.99\"\n");
	fprintf(stdout,
			"\t-zip zip_code = a code used for postal delivery e.g. \"10118\"\n");
	fprintf(stdout,
			"\t-region region = an administrative region of the nation, such as a state or province e.g. \"New York\"\n");
	fprintf(stdout,
			"\t-room room = a particular room in a building e.g. \"Observatory\"\n");
	fprintf(stdout,
			"\t-speed speed = the speed at which the device/transducer is moving, in meters per second (decimal) e.g. \"52.69\"\n");
	fprintf(stdout,
			"\t-street street = a thoroughfare within the locality, or a crossing of two thoroughfares e.g. \"350 Fifth Avenue / 34th and Broadway\"\n");
	fprintf(stdout,
			"\t-text text = a catch-all element that captures any other information about the location e.g. \"Northwest corner of the lobby\"\n");
	fprintf(stdout,
			"\t-tzo time_zone_offset = the time zone offset from UTC for the current location e.g. \"-07:00\"\n");
	fprintf(stdout,
			"\t-uri uri = a URI or URL pointing to a map of the location e.g. \"http://www.esbnyc.com/map.jpg\"\n");
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

int main(int argc, char **argv) {
	char *username = NULL;
	char *password = NULL;
	char *xmpp_server = NULL;
	char pubsub_server[80];
	int xmpp_server_port = 5223;
	char *event_node = NULL;
	mio_geoloc_t *geoloc = mio_geoloc_new();
	mio_transducer_meta_t *trans = NULL;
	mio_meta_t *meta = NULL;

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
		} else if (strcmp(current_arg_name, "-t_name") == 0) {
			trans = mio_transducer_meta_new();
			trans->name = strdup(current_arg_val);
			trans->geoloc = geoloc;
		} else if (strcmp(current_arg_name, "-accuracy") == 0) {
			geoloc->accuracy = malloc(sizeof(double));
			sscanf(current_arg_val, "%lf", geoloc->accuracy);
		} else if (strcmp(current_arg_name, "-alt") == 0) {
			geoloc->alt = malloc(sizeof(double));
			sscanf(current_arg_val, "%lf", geoloc->alt);
		} else if (strcmp(current_arg_name, "-area") == 0) {
			geoloc->area = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-bearing") == 0) {
			geoloc->bearing = malloc(sizeof(double));
			sscanf(current_arg_val, "%lf", geoloc->bearing);
		} else if (strcmp(current_arg_name, "-building") == 0) {
			geoloc->building = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-country") == 0) {
			geoloc->country = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-country_code") == 0) {
			geoloc->country_code = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-datum") == 0) {
			geoloc->datum = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-description") == 0) {
			geoloc->description = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-floor") == 0) {
			geoloc->floor = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-lat") == 0) {
			geoloc->lat = malloc(sizeof(double));
			sscanf(current_arg_val, "%lf", geoloc->lat);
		} else if (strcmp(current_arg_name, "-locality") == 0) {
			geoloc->locality = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-lon") == 0) {
			geoloc->lon = malloc(sizeof(double));
			sscanf(current_arg_val, "%lf", geoloc->lon);
		} else if (strcmp(current_arg_name, "-zip") == 0) {
			geoloc->postal_code = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-region") == 0) {
			geoloc->region = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-room") == 0) {
			geoloc->room = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-speed") == 0) {
			geoloc->speed = malloc(sizeof(double));
			sscanf(current_arg_val, "%lf", geoloc->speed);
		} else if (strcmp(current_arg_name, "-street") == 0) {
			geoloc->street = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-text") == 0) {
			geoloc->text = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-tzo") == 0) {
			geoloc->tzo = strdup(current_arg_val);
		} else if (strcmp(current_arg_name, "-uri") == 0) {
			geoloc->uri = strdup(current_arg_val);
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

	geoloc->timestamp = mio_timestamp_create();
	// If no t_name was specified the geoloc is added to the device registerd with the event node
	if (trans == NULL ) {
		meta = mio_meta_new();
		meta->timestamp = strdup(geoloc->timestamp);
		meta->geoloc = geoloc;
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
	mio_meta_merge_publish(conn, event_node, meta, trans, NULL, response);
	mio_response_print(response);
	mio_response_free(response);
	mio_disconnect(conn);
	if (trans != NULL )
		mio_transducer_meta_free(trans);
	if (meta != NULL )
		mio_meta_free(meta);
	mio_conn_free(conn);
	return MIO_OK;
}

