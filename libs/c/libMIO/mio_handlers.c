/******************************************************************************
 *  Mortar IO (MIO) Library
 *  Standard Data Handlers
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
#include <expat.h>
#include <mio.h>
#include <mio_handlers.h>
#include <common.h>
#include <string.h>
#include <pthread.h>

extern mio_log_level_t _mio_log_level;

mio_xml_parser_parent_data_t *mio_xml_parser_parent_data_new();
void mio_xml_parser_parent_data_free(mio_xml_parser_parent_data_t *data);
void _mio_xml_data_update_start_element(mio_xml_parser_data_t *xml_data,
		const char* element_name);
void _mio_xml_data_update_end_element(mio_xml_parser_data_t *xml_data,
		const char* element_name);

int _mio_xml_parse(mio_conn_t *conn, mio_stanza_t * const stanza,
		mio_xml_parser_data_t *xml_data, XML_StartElementHandler start,
		XML_CharacterDataHandler char_handler);
mio_xml_parser_data_t *mio_xml_parser_data_new();

mio_xml_parser_data_t *mio_xml_parser_data_new() {
	mio_xml_parser_data_t *xml_data = malloc(sizeof(mio_xml_parser_data_t));
	memset(xml_data, 0, sizeof(mio_xml_parser_data_t));
	return xml_data;
}

void mio_handler_conn_generic(xmpp_conn_t * const conn,
		const xmpp_conn_event_t status, const int error,
		xmpp_stream_error_t * const stream_error, void * const mio_handler_data) {

	mio_handler_data_t *shd = (mio_handler_data_t *) mio_handler_data;
	mio_conn_t *mio_conn = shd->conn;
	if (shd->response == NULL )
		shd->response = mio_response_new();
	mio_debug("In conn_handler");

	if (status == XMPP_CONN_CONNECT) {
		mio_conn->retries = 0;
		shd->response->response_type = MIO_RESPONSE_OK;
		mio_debug("In conn_handler : Connected");
		if (shd->conn_handler != NULL )
			shd->conn_handler(shd->conn, MIO_CONN_CONNECT, shd->response,
					shd->userdata);
		_mio_cond_broadcast(&shd->conn->conn_cond, &shd->conn->conn_mutex,
				&shd->conn->conn_predicate);

		// If the connection fails, try to reconnect up to MIO_CONNECTION_RETRIES times
	}
#ifdef MIO_CONNECTION_RETRIES
	else if (mio_conn->retries < MIO_CONNECTION_RETRIES) {
#else
		else {
#endif
		mio_conn->retries++;
		mio_warn(
				"Disconnected from server, attempting to reconnect, attempt: %u",
				mio_conn->retries);
		mio_disconnect(mio_conn);
		mio_connect(mio_conn->xmpp_conn->jid, mio_conn->xmpp_conn->pass, NULL,
				NULL, mio_conn);
		//xmpp_connect_client(conn, NULL, 0, mio_handler_conn_generic, shd);
		// If the connection still fails, return an error
	}
#ifdef MIO_CONNECTION_RETRIES
	else {
		mio_response_error_t *err;
		mio_error(
				"Disconnected from server, all retries failed. Check connection.");
		err = _mio_response_error_new();
		err->err_num = MIO_ERROR_CONNECTION;
		err->description = strdup("MIO_ERROR_CONNECTION");
		shd->response->response = err;
		shd->response->response_type = MIO_RESPONSE_ERROR;
		_mio_cond_broadcast(&shd->conn->conn_cond, &shd->conn->conn_mutex,
				&shd->conn->conn_predicate);
	}
#endif
}

int mio_handler_generic(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
		void * const mio_handler_data) {

	int err;
	mio_handler_data_t *shd = (mio_handler_data_t*) mio_handler_data;
	mio_stanza_t *s = malloc(sizeof(mio_stanza_t));
	s->xmpp_stanza = stanza;
	err = shd->handler(shd->conn, s, shd->response, shd->userdata);
	//free(s);
	// Returning 0 removes the handler
	if (err == MIO_HANDLER_KEEP) {
		return 1;
	} else if (err == MIO_HANDLER_REMOVE) {
		_mio_handler_data_free(shd);
		return 0;
	}
	return 1;
	/*if (shd->request != NULL ) {
	 // Returning 0 removes the handler
	 _mio_handler_data_free(shd);
	 return 0;
	 } else
	 return 0;*/
}

int mio_handler_generic_id(xmpp_conn_t * const conn,
		xmpp_stanza_t * const stanza, void * const mio_handler_data) {

	int err;
	mio_handler_data_t *shd = (mio_handler_data_t*) mio_handler_data;
	mio_stanza_t *s = malloc(sizeof(mio_stanza_t));
	s->xmpp_stanza = stanza;

	err = shd->handler(shd->conn, s, shd->response, shd->userdata);
	//free(s);

	// Returning 0 removes the handler
	if (err != MIO_OK)
		return 1;
	else {
		_mio_handler_data_free(shd);
		return 0;
	}
	/*if (shd->request != NULL ) {
	 // Returning 0 removes the handler
	 _mio_handler_data_free(shd);
	 return 0;
	 } else
	 return 1;*/
}

int mio_handler_generic_timed(xmpp_conn_t * const conn,
		void * const mio_handler_data) {

	int ret;

	mio_handler_data_t *shd = (mio_handler_data_t*) mio_handler_data;
	ret = shd->handler(shd->conn, NULL, shd->response, shd->userdata);
	if (ret == MIO_HANDLER_KEEP)
		return 1;
	else {
		_mio_handler_data_free(shd);
		return 0;
	}
}

int mio_handler_keepalive(xmpp_conn_t * const conn,
		xmpp_stanza_t * const stanza, void * const userdata) {
	// Send single space every KEEPALIVE_PERIOD ms to keep connection alive
	xmpp_send_raw(conn, " ", 1);
	return MIO_HANDLER_KEEP;
}

void mio_XML_error_handler(const char *element_name, const char **attr,
		mio_response_t *response) {

	int i;
	mio_response_error_t *err = _mio_response_error_new();

	for (i = 0; attr[i]; i += 2) {
		const char* attr_name = attr[i];
		if (strcmp(attr_name, "type") == 0) {
			err->description = malloc(strlen(attr[i + 1]) + 1);
			strcpy(err->description, attr[i + 1]);
			response->response_type = MIO_RESPONSE_ERROR;
			response->response = err;
		} else if (strcmp(attr_name, "code") == 0) {
			err->err_num = atoi(attr[i + 1]);
		}
	}
}

static void XMLCALL mio_XMLstart_subscribe(void *data, const char *element_name,
		const char **attr) {

	int i;
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	_mio_xml_data_update_start_element(xml_data, element_name);

	if (strcmp(element_name, "subscription") == 0) {
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "subscription") == 0) {
				if (strcmp(attr[i + 1], "subscribed") == 0)
					response->response_type = MIO_RESPONSE_OK;
			}
		}
	} else if (strcmp(element_name, "error") == 0)
		mio_XML_error_handler(element_name, attr, response);
}

static void XMLCALL mio_XMLstart_error(void *data, const char *element_name,
		const char **attr) {

	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	_mio_xml_data_update_start_element(xml_data, element_name);

	if (strcmp(element_name, "error") == 0)
		mio_XML_error_handler(element_name, attr, response);

	if (response->response_type != MIO_RESPONSE_ERROR)
		response->response_type = MIO_RESPONSE_OK;
}

static void XMLCALL mio_XMLstart_subscriptions_query(void *data,
		const char *element_name, const char **attr) {

	char *curr_jid_node = NULL, *curr_sub_id = NULL;
	int i;
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	mio_packet_t *packet = (mio_packet_t*) response->response;
	_mio_xml_data_update_start_element(xml_data, element_name);

	if (strcmp(element_name, "subscriptions") == 0) {
		response->response_type = MIO_RESPONSE_PACKET;
		packet->type = MIO_PACKET_SUBSCRIPTIONS;
	}

	else if (strcmp(element_name, "subscription") == 0) {
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "jid") == 0) {
				if (curr_jid_node != NULL )
					free(curr_jid_node);
				curr_jid_node = malloc(strlen(attr[i + 1]) + 1);
				strcpy(curr_jid_node, attr[i + 1]);
			} else if (strcmp(attr_name, "node") == 0) {
				if (curr_jid_node != NULL )
					free(curr_jid_node);
				curr_jid_node = malloc(strlen(attr[i + 1]) + 1);
				strcpy(curr_jid_node, attr[i + 1]);
			} else if (strcmp(attr_name, "subid") == 0) {
				curr_sub_id = malloc(strlen(attr[i + 1]) + 1);
				strcpy(curr_sub_id, attr[i + 1]);
			}
		}
		_mio_subscription_add(packet, curr_jid_node, curr_sub_id);
		if (curr_jid_node != NULL )
			free(curr_jid_node);
		if (curr_sub_id != NULL )
			free(curr_sub_id);

	} else if (strcmp(element_name, "error") == 0)
		mio_XML_error_handler(element_name, attr, response);
}

static void XMLCALL mio_XMLstart_acl_affiliations_query(void *data,
		const char *element_name, const char **attr) {

	char *curr_jid_node = NULL;
	mio_affiliation_type_t curr_aff = MIO_AFFILIATION_UNKOWN;
	int i;
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	mio_packet_t *packet = (mio_packet_t*) response->response;
	_mio_xml_data_update_start_element(xml_data, element_name);

	if (strcmp(element_name, "affiliations") == 0) {
		response->response_type = MIO_RESPONSE_PACKET;
		packet->type = MIO_PACKET_AFFILIATIONS;
	}

	else if (strcmp(element_name, "affiliation") == 0) {
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "jid") == 0) {
				if (curr_jid_node != NULL )
					free(curr_jid_node);
				curr_jid_node = malloc(strlen(attr[i + 1]) + 1);
				strcpy(curr_jid_node, attr[i + 1]);
			} else if (strcmp(attr_name, "node") == 0) {
				if (curr_jid_node != NULL )
					free(curr_jid_node);
				curr_jid_node = malloc(strlen(attr[i + 1]) + 1);
				strcpy(curr_jid_node, attr[i + 1]);
			} else if (strcmp(attr_name, "affiliation") == 0) {
				if (strcmp(attr[i + 1], "none") == 0)
					curr_aff = MIO_AFFILIATION_NONE;
				else if (strcmp(attr[i + 1], "owner") == 0)
					curr_aff = MIO_AFFILIATION_OWNER;
				else if (strcmp(attr[i + 1], "member") == 0)
					curr_aff = MIO_AFFILIATION_MEMBER;
				else if (strcmp(attr[i + 1], "publisher") == 0)
					curr_aff = MIO_AFFILIATION_PUBLISHER;
				else if (strcmp(attr[i + 1], "publish-only") == 0)
					curr_aff = MIO_AFFILIATION_PUBLISH_ONLY;
				else if (strcmp(attr[i + 1], "outcast") == 0)
					curr_aff = MIO_AFFILIATION_OUTCAST;
			}
		}
		_mio_affiliation_add(packet, curr_jid_node, curr_aff);
		if (curr_jid_node != NULL )
			free(curr_jid_node);

	} else if (strcmp(element_name, "error") == 0)
		mio_XML_error_handler(element_name, attr, response);
}

static void XMLCALL mio_XMLstart_collection_children_query(void *data,
		const char *element_name, const char **attr) {

	int i;
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	mio_packet_t *packet = (mio_packet_t*) response->response;
	char *curr_node = NULL, *curr_name = NULL;
	_mio_xml_data_update_start_element(xml_data, element_name);

	if (strcmp(element_name, "item") == 0) {
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "node") == 0) {
				curr_node = malloc(strlen(attr[i + 1]) + 1);
				strcpy(curr_node, attr[i + 1]);
			} else if (strcmp(attr_name, "name") == 0) {
				curr_name = malloc(strlen(attr[i + 1]) + 1);
				strcpy(curr_name, attr[i + 1]);
			}
		}
		_mio_collection_add(packet, curr_node, curr_name);
		if (curr_node != NULL )
			free(curr_node);
		if (curr_name != NULL )
			free(curr_name);

	} else if (strcmp(element_name, "error") == 0)
		mio_XML_error_handler(element_name, attr, response);
}

static void XMLCALL mio_XMLString_collection_query(void *data,
		const XML_Char *s, int len) {
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_packet_t *packet = xml_data->payload;
	char *curr_node = NULL;

	if (strcmp(xml_data->curr_element_name, "value") == 0) {
		curr_node = malloc(len + 1);
		snprintf(curr_node, len + 1, "%s", s);
		_mio_collection_add(packet, curr_node, NULL );
	}
}

static void XMLCALL mio_XMLstart_collection_parents_query(void *data,
		const char *element_name, const char **attr) {

	int i;
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	mio_packet_t *packet = (mio_packet_t*) response->response;
	_mio_xml_data_update_start_element(xml_data, element_name);

	xml_data->prev_element_name = xml_data->curr_element_name;
	xml_data->curr_element_name = element_name;

	if (strcmp(element_name, "field") == 0) {
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "var") == 0) {
				if (strcmp("pubsub#collection", attr[i + 1]) == 0) {
					xml_data->payload = (void*) packet;
					XML_SetCharacterDataHandler(*xml_data->parser,
							mio_XMLString_collection_query);
				}
			} else
				XML_SetCharacterDataHandler(*xml_data->parser, NULL );
		}
	} else if (strcmp(element_name, "error") == 0)
		mio_XML_error_handler(element_name, attr, response);
}

static void XMLCALL mio_XMLstart_pubsub_data_receive(void *data,
		const char *element_name, const char **attr) {

	int i;
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	mio_packet_t *packet = (mio_packet_t*) response->response;
	mio_data_t *mio_data = (mio_data_t*) packet->payload;
	mio_transducer_value_t *t_value;
	_mio_xml_data_update_start_element(xml_data, element_name);

	if (strcmp(element_name, "items") == 0) {
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "node") == 0) {
				mio_data->event = malloc(strlen(attr[i + 1]) + 1);
				strcpy(mio_data->event, attr[i + 1]);
				// TODO: Support for multiple payloads
				packet->num_payloads++;
			}
		}

	} else if (strcmp(element_name, "transducerValue") == 0
			|| strcmp(element_name, "transducerSetValue") == 0) {
		t_value = mio_transducer_value_new();
		if (strcmp(element_name, "transducerValue") == 0)
			t_value->type = MIO_TRANSDUCER_VALUE;
		else
			t_value->type = MIO_TRANSDUCER_SET_VALUE;

		response->response_type = MIO_RESPONSE_PACKET;

		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "typedValue") == 0) {
				t_value->typed_value = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_value->typed_value, attr[i + 1]);
			} else if (strcmp(attr_name, "rawValue") == 0) {
				t_value->raw_value = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_value->raw_value, attr[i + 1]);
			} else if (strcmp(attr_name, "id") == 0) {
				t_value->name = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_value->name, attr[i + 1]);
			} else if (strcmp(attr_name, "timestamp") == 0) {
				t_value->timestamp = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_value->timestamp, attr[i + 1]);
			} /*else if (strcmp(attr_name, "name") == 0) {
			 t_value->name = malloc(strlen(attr[i + 1]) + 1);
			 strcpy(t_value->name, attr[i + 1]);
			 }*/
		}
		mio_data_transducer_value_add(mio_data, t_value->type, t_value->id,
				t_value->name, t_value->typed_value, t_value->raw_value,
				t_value->timestamp);
		mio_transducer_value_free(t_value);
	}

}

static void XMLCALL mio_XMLString_geoloc(void *data, const XML_Char *s, int len) {
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_geoloc_t *geoloc = (mio_geoloc_t*) xml_data->payload;
	char format[32];

	if (strcmp(xml_data->curr_element_name, "accuracy")
			== 0&& geoloc->accuracy == NULL) {
		geoloc->accuracy = malloc(sizeof(double));
		snprintf(format, 32, "%%%ulf", len);
		sscanf(s, format, geoloc->accuracy);
	}
	if (strcmp(xml_data->curr_element_name, "alt") == 0 && geoloc->alt == NULL ) {
		geoloc->alt = malloc(sizeof(double));
		snprintf(format, 32, "%%%ulf", len);
		sscanf(s, format, geoloc->alt);
	}
	if (strcmp(xml_data->curr_element_name, "area")
			== 0&& geoloc->area == NULL) {
		geoloc->area = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->area, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "bearing")
			== 0&& geoloc->bearing == NULL) {
		geoloc->bearing = malloc(sizeof(double));
		snprintf(format, 32, "%%%ulf", len);
		sscanf(s, format, geoloc->bearing);
	}
	if (strcmp(xml_data->curr_element_name, "building")
			== 0&& geoloc->building == NULL) {
		geoloc->building = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->building, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "country")
			== 0&& geoloc->country == NULL) {
		geoloc->country = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->country, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "countrycode")
			== 0&& geoloc->country_code == NULL) {
		geoloc->country_code = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->country_code, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "datum")
			== 0&& geoloc->datum == NULL) {
		geoloc->datum = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->datum, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "description")
			== 0&& geoloc->description == NULL) {
		geoloc->description = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->description, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "floor")
			== 0&& geoloc->floor == NULL) {
		geoloc->floor = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->floor, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "lat") == 0 && geoloc->lat == NULL ) {
		geoloc->lat = malloc(sizeof(double));
		snprintf(format, 32, "%%%ulf", len);
		sscanf(s, format, geoloc->lat);
	}
	if (strcmp(xml_data->curr_element_name, "locality")
			== 0&& geoloc->locality == NULL) {
		geoloc->locality = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->locality, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "lon") == 0 && geoloc->lon == NULL ) {
		geoloc->lon = malloc(sizeof(double));
		snprintf(format, 32, "%%%ulf", len);
		sscanf(s, format, geoloc->lon);
	}
	if (strcmp(xml_data->curr_element_name, "postalcode")
			== 0&& geoloc->postal_code == NULL) {
		geoloc->postal_code = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->postal_code, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "region")
			== 0&& geoloc->region == NULL) {
		geoloc->region = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->region, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "room")
			== 0&& geoloc->room == NULL) {
		geoloc->room = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->room, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "speed")
			== 0&& geoloc->speed == NULL) {
		geoloc->speed = malloc(sizeof(double));
		snprintf(format, 32, "%%%ulf", len);
		sscanf(s, format, geoloc->speed);
	}
	if (strcmp(xml_data->curr_element_name, "street")
			== 0&& geoloc->street == NULL) {
		geoloc->street = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->street, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "text")
			== 0&& geoloc->text == NULL) {
		geoloc->text = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->text, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "timestamp")
			== 0&& geoloc->timestamp == NULL) {
		geoloc->timestamp = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->timestamp, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "tzo") == 0 && geoloc->tzo == NULL ) {
		geoloc->tzo = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->tzo, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "uri") == 0 && geoloc->uri == NULL ) {
		geoloc->uri = malloc(sizeof(char) * len + 1);
		snprintf(geoloc->uri, len + 1, "%s", s);
	}
}

static void XMLCALL mio_XMLstart_pubsub_meta_receive(void *data,
		const char *element_name, const char **attr) {

	int i = 0;
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	mio_packet_t *packet = (mio_packet_t*) response->response;
	mio_meta_t *mio_meta = (mio_meta_t*) packet->payload;
	mio_transducer_meta_t *t_meta = NULL;
	mio_enum_map_meta_t *e_meta = NULL;
	mio_property_meta_t *p_meta = NULL, *p_tail = NULL;
	mio_geoloc_t *geoloc = NULL;
	_mio_xml_data_update_start_element(xml_data, element_name);

	if (strcmp(element_name, "items") == 0) {
		mio_meta = mio_meta_new();
		packet->payload = mio_meta;
		packet->type = MIO_PACKET_META;
		packet->num_payloads++;
		response->response_type = MIO_RESPONSE_PACKET;
	}

	if (strcmp(element_name, "meta") == 0) {
		XML_SetCharacterDataHandler(*xml_data->parser, NULL );
		xml_data->payload = mio_meta;
		for (i = 0; attr[i]; i += 2) {
			xml_data->curr_attr_name = attr[i];
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "name") == 0) {
				mio_meta->name = malloc(strlen(attr[i + 1]) + 1);
				strcpy(mio_meta->name, attr[i + 1]);
			} else if (strcmp(attr_name, "timestamp") == 0) {
				mio_meta->timestamp = malloc(strlen(attr[i + 1]) + 1);
				strcpy(mio_meta->timestamp, attr[i + 1]);
			} else if (strcmp(attr_name, "info") == 0) {
				mio_meta->info = malloc(strlen(attr[i + 1]) + 1);
				strcpy(mio_meta->info, attr[i + 1]);
			} else if (strcmp(attr_name, "type") == 0) {
				if (strcmp(attr[i + 1], "location") == 0)
					mio_meta->meta_type = MIO_META_TYPE_LOCATION;
				if (strcmp(attr[i + 1], "device") == 0)
					mio_meta->meta_type = MIO_META_TYPE_DEVICE;
			}
		}
	} else if (strcmp(element_name, "transducer") == 0) {
		XML_SetCharacterDataHandler(*xml_data->parser, NULL );
		xml_data->curr_attr_name = attr[i];
		t_meta = mio_transducer_meta_new();
		xml_data->payload = t_meta;
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "name") == 0) {
				t_meta->name = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->name, attr[i + 1]);
			} else if (strcmp(attr_name, "type") == 0) {
				t_meta->type = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->type, attr[i + 1]);
			} else if (strcmp(attr_name, "interface") == 0) {
				t_meta->interface = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->interface, attr[i + 1]);
			} else if (strcmp(attr_name, "unit") == 0) {
				t_meta->unit = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->unit, attr[i + 1]);
			} else if (strcmp(attr_name, "manufacturer") == 0) {
				t_meta->manufacturer = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->manufacturer, attr[i + 1]);
			} else if (strcmp(attr_name, "serial") == 0) {
				t_meta->serial = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->serial, attr[i + 1]);
			} else if (strcmp(attr_name, "info") == 0) {
				t_meta->info = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->info, attr[i + 1]);
			} else if (strcmp(attr_name, "minValue") == 0) {
				t_meta->min_value = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->min_value, attr[i + 1]);
			} else if (strcmp(attr_name, "maxValue") == 0) {
				t_meta->max_value = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->max_value, attr[i + 1]);
			} else if (strcmp(attr_name, "resolution") == 0) {
				t_meta->resolution = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->resolution, attr[i + 1]);
			} else if (strcmp(attr_name, "precision") == 0) {
				t_meta->precision = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->precision, attr[i + 1]);
			} else if (strcmp(attr_name, "accuracy") == 0) {
				t_meta->accuracy = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->accuracy, attr[i + 1]);
			}else if (strcmp(attr_name, "interface") == 0) {
				t_meta->interface = malloc(strlen(attr[i + 1]) + 1);
				strcpy(t_meta->interface, attr[i + 1]);
			}
		}
		if (mio_meta->meta_type != MIO_META_TYPE_UKNOWN)
			mio_transducer_meta_add(mio_meta, t_meta);
	} else if (strcmp(element_name, "map") == 0) {
		XML_SetCharacterDataHandler(*xml_data->parser, NULL );
		xml_data->curr_attr_name = attr[i];
		t_meta = mio_transducer_meta_tail_get(mio_meta->transducers);
		if (t_meta->enumeration == NULL ) {
			t_meta->enumeration = mio_enum_map_meta_new();
			e_meta = t_meta->enumeration;
		} else {
			e_meta = mio_enum_map_meta_tail_get(t_meta->enumeration);
			e_meta->next = mio_enum_map_meta_new();
			e_meta = e_meta->next;
		}
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "name") == 0) {
				e_meta->name = malloc(strlen(attr[i + 1]) + 1);
				strcpy(e_meta->name, attr[i + 1]);
			} else if (strcmp(attr_name, "value") == 0) {
				e_meta->value = malloc(strlen(attr[i + 1]) + 1);
				strcpy(e_meta->value, attr[i + 1]);
			}
		}
	} else if (strcmp(element_name, "property") == 0) {
		XML_SetCharacterDataHandler(*xml_data->parser, NULL );
		p_meta = mio_property_meta_new();
		if (xml_data->parent != NULL ) {
			if (strcmp(xml_data->parent->parent, "transducer") == 0) {
				t_meta = (mio_transducer_meta_t*) xml_data->payload;
				p_tail = mio_property_meta_tail_get(t_meta->properties);
				if (p_tail == NULL )
					t_meta->properties = p_meta;
				else
					p_tail->next = p_meta;
			} else {
				p_tail = mio_property_meta_tail_get(mio_meta->properties);
				if (p_tail == NULL )
					mio_meta->properties = p_meta;
				else
					p_tail->next = p_meta;
				//mio_meta->properties = p_meta;
			}
		}
		xml_data->curr_attr_name = attr[i];
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "name") == 0) {
				p_meta->name = malloc(strlen(attr[i + 1]) + 1);
				strcpy(p_meta->name, attr[i + 1]);
			} else if (strcmp(attr_name, "value") == 0) {
				p_meta->value = malloc(strlen(attr[i + 1]) + 1);
				strcpy(p_meta->value, attr[i + 1]);
			}
		}
		//	if (p_meta != NULL )
		//	mio_property_meta_add(mio_meta, p_meta);
	} else if (strcmp(element_name, "geoloc") == 0) {
		geoloc = mio_geoloc_new();
		if (xml_data->prev_element_name != NULL ) {
			if (strcmp(xml_data->prev_element_name, "transducer") == 0) {
				t_meta = (mio_transducer_meta_t*) xml_data->payload;
				t_meta->geoloc = geoloc;
			} else {
				mio_meta = (mio_meta_t*) xml_data->payload;
				mio_meta->geoloc = geoloc;
			}
		}
		xml_data->payload = geoloc;
		XML_SetCharacterDataHandler(*xml_data->parser, mio_XMLString_geoloc);
	}

}

static void XMLCALL mio_XMLstart_node_type_query(void *data,
		const char *element_name, const char **attr) {

	int i;
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	mio_packet_t *packet = (mio_packet_t*) response->response;
	mio_node_type_t *type;
	_mio_xml_data_update_start_element(xml_data, element_name);

	if (strcmp(element_name, "identity") == 0) {
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "type") == 0) {
				response->response_type = MIO_RESPONSE_PACKET;
				packet->type = MIO_PACKET_NODE_TYPE;
				packet->num_payloads = 1;
				packet->payload = malloc(sizeof(mio_node_type_t));
				type = (mio_node_type_t*) packet->payload;
				if (strcmp(attr[i + 1], "collection") == 0)
					*type = MIO_NODE_TYPE_COLLECTION;
				else if (strcmp(attr[i + 1], "leaf") == 0)
					*type = MIO_NODE_TYPE_LEAF;
				else
					*type = MIO_NODE_TYPE_UNKNOWN;
			}
		}
	} else if (strcmp(element_name, "error") == 0)
		mio_XML_error_handler(element_name, attr, response);
}

static void XMLCALL mio_XMLString_recurrence(void *data, const XML_Char *s,
		int len) {
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_event_t *event = (mio_event_t*) xml_data->payload;
	mio_recurrence_t *rec = event->recurrence;

	if (strcmp(xml_data->curr_element_name, "freq") == 0 && rec->freq == NULL ) {
		rec->freq = malloc(sizeof(char) * len + 1);
		snprintf(rec->freq, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "interval")
			== 0&& rec->interval == NULL) {
		rec->interval = malloc(sizeof(char) * len + 1);
		snprintf(rec->interval, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "count")
			== 0&& rec->count == NULL) {
		rec->count = malloc(sizeof(char) * len + 1);
		snprintf(rec->count, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "until")
			== 0&& rec->until == NULL) {
		rec->until = malloc(sizeof(char) * len + 1);
		snprintf(rec->until, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "bymonth")
			== 0&& rec->bymonth == NULL) {
		rec->bymonth = malloc(sizeof(char) * len + 1);
		snprintf(rec->bymonth, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "byday")
			== 0&& rec->byday == NULL) {
		rec->byday = malloc(sizeof(char) * len + 1);
		snprintf(rec->byday, len + 1, "%s", s);
	}
	if (strcmp(xml_data->curr_element_name, "exdate")
			== 0&& rec->exdate == NULL) {
		rec->exdate = malloc(sizeof(char) * len + 1);
		snprintf(rec->exdate, len + 1, "%s", s);
	}
}

static void XMLCALL mio_XMLstart_schedule_query(void *data,
		const char *element_name, const char **attr) {

	int i;
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	mio_packet_t *packet = (mio_packet_t*) response->response;
	mio_event_t *event = NULL;
	mio_recurrence_t *recurrence;
	xml_data->prev_element_name = xml_data->curr_element_name;
	xml_data->curr_element_name = element_name;
	_mio_xml_data_update_start_element(xml_data, element_name);

	if (strcmp(element_name, "items") == 0) {
		response->response_type = MIO_RESPONSE_PACKET;
		packet->type = MIO_PACKET_SCHEDULE;
	}
	if (strcmp(element_name, "event") == 0) {
		XML_SetCharacterDataHandler(*xml_data->parser, NULL );
		if (packet->payload == NULL ) {
			packet->payload = mio_event_new();
			event = (mio_event_t*) packet->payload;
		} else {
			event = (mio_event_t*) packet->payload;
			while (event->next != NULL )
				event = event->next;
			event->next = mio_event_new();
			event = event->next;
		}
		xml_data->payload = event;
		packet->num_payloads++;

		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "time") == 0) {
				event->time = strdup(attr[i + 1]);
			} else if (strcmp(attr_name, "info") == 0) {
				event->info = strdup(attr[i + 1]);
			} else if (strcmp(attr_name, "transducerName") == 0) {
				event->tranducer_name = strdup(attr[i + 1]);
			} else if (strcmp(attr_name, "transducerValue") == 0) {
				event->transducer_value = strdup(attr[i + 1]);
			} else if (strcmp(attr_name, "id") == 0) {
				sscanf(attr[i + 1], "%u", &event->id);
			}
		}
	} else if (strcmp(element_name, "recurrence") == 0) {
		event = (mio_event_t*) xml_data->payload;
		recurrence = mio_recurrence_new();
		event->recurrence = recurrence;
		XML_SetCharacterDataHandler(*xml_data->parser,
				mio_XMLString_recurrence);
	} else if (strcmp(element_name, "error") == 0)
		mio_XML_error_handler(element_name, attr, response);
}

static void XMLCALL mio_XMLstart_references_query(void *data,
		const char *element_name, const char **attr) {

	int i;
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	mio_response_t *response = (mio_response_t*) xml_data->response;
	mio_packet_t *packet = (mio_packet_t*) response->response;
	mio_reference_t *ref;
	_mio_xml_data_update_start_element(xml_data, element_name);

	if (strcmp(element_name, "items") == 0) {
		response->response_type = MIO_RESPONSE_PACKET;
		packet->type = MIO_PACKET_REFERENCES;
	}
	if (strcmp(element_name, "reference") == 0) {
		packet->num_payloads++;
		if (packet->payload == NULL ) {
			packet->payload = mio_reference_new();
			ref = (mio_reference_t*) packet->payload;
		} else {
			ref = mio_reference_tail_get((mio_reference_t*) packet->payload);
			ref->next = mio_reference_new();
			ref = ref->next;
		}
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "type") == 0) {
				if (strcmp(attr[i + 1], "child") == 0)
					ref->type = MIO_REFERENCE_CHILD;
				else if (strcmp(attr[i + 1], "parent") == 0)
					ref->type = MIO_REFERENCE_PARENT;
			} else if (strcmp(attr_name, "metaType") == 0) {
				if (strcmp(attr[i + 1], "device") == 0)
					ref->meta_type = MIO_META_TYPE_DEVICE;
				else if (strcmp(attr[i + 1], "location") == 0)
					ref->meta_type = MIO_META_TYPE_LOCATION;
			} else if (strcmp(attr_name, "node") == 0) {
				ref->node = strdup(attr[i + 1]);
			} else if (strcmp(attr_name, "name") == 0) {
				ref->name = strdup(attr[i + 1]);
			}
		}
	} else if (strcmp(element_name, "error") == 0)
		mio_XML_error_handler(element_name, attr, response);
}

// XMLParser func called whenever an end of element is encountered
static void XMLCALL endElement(void *data, const char *element_name) {
	mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
	if(xml_data != NULL)
		_mio_xml_data_update_end_element(xml_data, element_name);
}

void _mio_xml_data_update_start_element(mio_xml_parser_data_t *xml_data,
		const char* element_name) {
	mio_xml_parser_parent_data_t *parent_data;

	xml_data->prev_element_name = xml_data->curr_element_name;
	xml_data->curr_element_name = element_name;
	xml_data->curr_depth++;
	if (xml_data->prev_depth < xml_data->curr_depth) {
		if (xml_data->parent == NULL ) {
			xml_data->parent = mio_xml_parser_parent_data_new();
			parent_data = xml_data->parent;
		} else {
			if (xml_data->parent->next != NULL )
				mio_xml_parser_parent_data_free(xml_data->parent->next);
			xml_data->parent->next = mio_xml_parser_parent_data_new();
			parent_data = xml_data->parent->next;
			parent_data->prev = xml_data->parent;
		}
		parent_data->parent = xml_data->prev_element_name;
		xml_data->parent = parent_data;
	}
	xml_data->prev_depth = xml_data->curr_depth;
}

void _mio_xml_data_update_end_element(mio_xml_parser_data_t *xml_data,
		const char* element_name) {
	if (xml_data->prev_depth > xml_data->curr_depth) {
		xml_data->parent = xml_data->parent->prev;
		mio_xml_parser_parent_data_free(xml_data->parent->next);
		xml_data->parent->next = NULL;
		if(xml_data->parent->parent == NULL)
			mio_xml_parser_parent_data_free(xml_data->parent);
	}
	xml_data->prev_depth = xml_data->curr_depth;
	xml_data->curr_depth--;
}

mio_xml_parser_parent_data_t *mio_xml_parser_parent_data_new() {
	mio_xml_parser_parent_data_t *data = malloc(
			sizeof(mio_xml_parser_parent_data_t));
	memset(data, 0, sizeof(mio_xml_parser_parent_data_t));
	return data;
}

void mio_xml_parser_parent_data_free(mio_xml_parser_parent_data_t *data) {
	free(data);
}

int _mio_xml_parse(mio_conn_t *conn, mio_stanza_t * const stanza,
		mio_xml_parser_data_t *xml_data, XML_StartElementHandler start,
		XML_CharacterDataHandler char_handler) {

	XML_Parser p;
	char *buf;
	size_t buflen;

	if (stanza == NULL ) {
		mio_error("XML Parser received NULL stanza");
		return MIO_ERROR_XML_NULL_STANZA;
	}

//Convert received stanza to text for XML parser
	xmpp_stanza_to_text(stanza->xmpp_stanza, &buf, &buflen);

//Creates an instance of the XML Parser to parse the event packet
	p = XML_ParserCreate(NULL );
	if (!p) {
		mio_error("Could not allocate XML parser");
		return MIO_ERROR_XML_PARSER_ALLOCATION;
	}
//Sets the handlers to call when parsing the start and end of an XML element
	XML_SetElementHandler(p, start, endElement);
	if (char_handler != NULL )
		XML_SetCharacterDataHandler(p, char_handler);
	XML_SetUserData(p, xml_data);
	xml_data->parser = &p;

	if (XML_Parse(p, buf, strlen(buf), 1) == XML_STATUS_ERROR) {
		mio_error("XML parse error at line %u:\n%s\n",
				(int ) XML_GetCurrentLineNumber(p),
				XML_ErrorString(XML_GetErrorCode(p)));
		return MIO_ERROR_XML_PARSING;
	}

	XML_ParserFree(p);
	xmpp_free(conn->xmpp_conn->ctx, buf);
	free(xml_data);

	return MIO_OK;
}

int mio_conn_handler_presence_send(mio_conn_t * const conn,
		mio_conn_event_t event, mio_response_t *response, void *userdata) {

	xmpp_stanza_t *pres;
	if (event == MIO_CONN_CONNECT) {
//Send initial <presence/> so that we appear online to contacts
		pres = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(pres, "presence");
		xmpp_send(conn->xmpp_conn, pres);
		xmpp_stanza_release(pres);
		return MIO_OK;
	}
	return MIO_ERROR_DISCONNECTED;
}

int mio_handler_subscribe(mio_conn_t * const conn, mio_stanza_t * const stanza,
		mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;

	int err = _mio_xml_parse(conn, stanza, xml_data, mio_XMLstart_subscribe,
			NULL );

	if (err == MIO_OK)
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);

	return err;
}

int mio_handler_error(mio_conn_t * const conn, mio_stanza_t * const stanza,
		mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;

	int err = _mio_xml_parse(conn, stanza, xml_data, mio_XMLstart_error, NULL );

	if (err == MIO_OK)
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);

	return err;
}

int mio_handler_subscriptions_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;

	mio_packet_t *packet = mio_packet_new();
	response->response = packet;
	int err = _mio_xml_parse(conn, stanza, xml_data,
			mio_XMLstart_subscriptions_query, NULL );
	if (err == MIO_OK)
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);

	return err;
}

int mio_handler_acl_affiliations_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;

	mio_packet_t *packet = mio_packet_new();
	response->response = packet;
	int err = _mio_xml_parse(conn, stanza, xml_data,
			mio_XMLstart_acl_affiliations_query, NULL );

	if (err == MIO_OK)
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);

	return err;
}

int mio_handler_collection_children_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;

	mio_packet_t *packet = mio_packet_new();
	response->response = packet;
	int err = _mio_xml_parse(conn, stanza, xml_data,
			mio_XMLstart_collection_children_query, NULL );

	if (err == MIO_OK) {
		response->response_type = MIO_RESPONSE_PACKET;
		packet->type = MIO_PACKET_COLLECTIONS;
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
	}

	return err;
}

int mio_handler_collection_parents_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;

	mio_packet_t *packet = mio_packet_new();
	response->response = packet;
	int err = _mio_xml_parse(conn, stanza, xml_data,
			mio_XMLstart_collection_parents_query, NULL );

	if (err == MIO_OK) {
		response->response_type = MIO_RESPONSE_PACKET;
		packet->type = MIO_PACKET_COLLECTIONS;
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
	}

	return err;
}

int mio_handler_pubsub_data_receive(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	mio_request_t *request = _mio_request_get(conn, "pubsub_data_rx");

	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_packet_t *packet = mio_packet_new();
	mio_data_t *data = mio_data_new();
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();

	if (response != NULL )
		request = _mio_request_get(conn, response->id);
	else
		request = _mio_request_get(conn, "pubsub_data_rx");

	mio_packet_payload_add(packet, (void*) data, MIO_PACKET_DATA);

	if (response == NULL ) {
		response = mio_response_new();
		strcpy(response->id, "pubsub_data_rx");
		response->ns = malloc(
				strlen("http://jabber.org/protocol/pubsub#event") + 1);
		strcpy(response->ns, "http://jabber.org/protocol/pubsub#event");
	}
	response->response = packet;
	xml_data->response = response;

	int err = _mio_xml_parse(conn, stanza, xml_data,
			mio_XMLstart_pubsub_data_receive, NULL );

	if (err != MIO_OK)
		return err;
	else if (response->response_type == MIO_RESPONSE_PACKET && request != NULL ) {
		stanza_copy = mio_stanza_clone(conn, stanza);
		response->stanza = stanza_copy;
		_mio_pubsub_rx_queue_enqueue(conn, response);
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
		return MIO_HANDLER_KEEP;
	} else {
		mio_packet_free(packet);
		return MIO_HANDLER_KEEP;
	}
}

/*int mio_handler_item_recent_get(mio_conn_t * const conn,
 mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
 mio_request_t *request = _mio_request_get(conn, response->id);
 if (request == NULL ) {
 mio_error("Request with id %s not found, aborting handler",
 response->id);
 return MIO_ERROR_REQUEST_NOT_FOUND;
 }
 mio_packet_t *packet = mio_packet_new();
 mio_data_t *data = mio_data_new();
 mio_packet_payload_add(packet, (void*) data, MIO_PACKET_DATA);
 mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
 xml_data->response = response;

 response->response = packet;
 int err = _mio_xml_parse(conn, stanza, xml_data,
 mio_XMLstart_pubsub_data_receive, NULL );

 if (err == MIO_OK) {
 response->response_type = MIO_RESPONSE_PACKET;
 _mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
 }

 return err;
 }
 */
int mio_handler_meta_query(mio_conn_t * const conn, mio_stanza_t * const stanza,
		mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_packet_t *packet = mio_packet_new();
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();

	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;
	response->response = packet;
	int err = _mio_xml_parse(conn, stanza, xml_data,
			mio_XMLstart_pubsub_meta_receive, mio_XMLString_geoloc);

	if (err == MIO_OK) {
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
	}

	return err;
}

int mio_handler_node_type_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_packet_t *packet = mio_packet_new();
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();

	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;
	response->response = packet;
	int err = _mio_xml_parse(conn, stanza, xml_data,
			mio_XMLstart_node_type_query, NULL );

	if (err == MIO_OK)
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);

	return err;
}

int mio_handler_item_recent_get(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_packet_t *packet = mio_packet_new();
	mio_data_t *data = mio_data_new();
	mio_packet_payload_add(packet, (void*) data, MIO_PACKET_DATA);
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;

	response->response = packet;
	int err = _mio_xml_parse(conn, stanza, xml_data,
			mio_XMLstart_pubsub_data_receive, NULL );

	if (err == MIO_OK) {
		response->response_type = MIO_RESPONSE_PACKET;
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
	}

	return err;
}

int mio_handler_schedule_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	int err;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_packet_t *packet = mio_packet_new();
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;

	response->response = packet;
	err = _mio_xml_parse(conn, stanza, xml_data, mio_XMLstart_schedule_query,
			NULL );

	if (err == MIO_OK) {
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
	}

	return err;
}

int mio_handler_references_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
	mio_stanza_t *stanza_copy;
	int err;
	mio_request_t *request = _mio_request_get(conn, response->id);
	if (request == NULL ) {
		mio_error("Request with id %s not found, aborting handler",
				response->id);
		return MIO_ERROR_REQUEST_NOT_FOUND;
	}
	mio_packet_t *packet = mio_packet_new();
	mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
	xml_data->response = response;
	stanza_copy = mio_stanza_clone(conn, stanza);
	response->stanza = stanza_copy;

	response->response = packet;
	err = _mio_xml_parse(conn, stanza, xml_data, mio_XMLstart_references_query,
			NULL );

	if (err == MIO_OK) {
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
	}

	return err;
}
