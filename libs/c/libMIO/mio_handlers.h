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

#ifndef HANDLERS_H_
#define HANDLERS_H_

#define NAME_MAX_CHARS 128

#include <mio.h>
#include <expat.h>

typedef struct mio_xml_parser_parent_data mio_xml_parser_parent_data_t;

struct mio_xml_parser_parent_data {
	const char *parent;
	mio_xml_parser_parent_data_t* next;
	mio_xml_parser_parent_data_t* prev;
};

typedef struct mio_xml_parser_data {
	mio_response_t *response;
	void* payload;
	XML_Parser *parser;
	const char* curr_element_name;
	const char* prev_element_name;
	const char* curr_attr_name;
	int curr_depth;
	int prev_depth;
	mio_xml_parser_parent_data_t *parent;
} mio_xml_parser_data_t;

void mio_callback_set_publish_success(void *f);

void mio_handler_conn_generic(xmpp_conn_t * const, const xmpp_conn_event_t,
		const int, xmpp_stream_error_t * const, void * const);

int mio_handler_generic(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
		void * const mio_handler_data);

int mio_handler_generic_id(xmpp_conn_t * const conn,
		xmpp_stanza_t * const stanza, void * const mio_handler_data);

int mio_handler_generic_timed(xmpp_conn_t * const conn,
		void * const mio_handler_data);

int mio_handler_error(mio_conn_t * const conn, mio_stanza_t * const stanza,
		mio_response_t *response, void *userdata);

int mio_handler_keepalive(xmpp_conn_t * const, xmpp_stanza_t * const,
		void * const);

size_t mio_handler_admin_user_functions(void*, size_t, size_t, void*);

int mio_handler_subscribe(mio_conn_t * const conn, mio_stanza_t * const stanza,
		mio_response_t *response, void *userdata);

int mio_handler_publisher_add(mio_conn_t* const, mio_stanza_t* const,
		mio_data_t* const);

int mio_handler_publisher_remove(mio_conn_t* const, mio_stanza_t* const,
		mio_data_t* const);

int mio_handler_unsubscribe(mio_conn_t* const, mio_stanza_t* const,
		mio_data_t* const);

int mio_handler_acl_node(mio_conn_t* const, mio_stanza_t* const,
		mio_data_t* const);

int mio_handler_node_delete(mio_conn_t* const, mio_stanza_t* const,
		mio_data_t* const);

int mio_handler_publish(mio_conn_t * const conn, mio_stanza_t * const stanza,
		mio_response_t *response, void *userdata);

int mio_handler_check_jid_registered(mio_conn_t* const, mio_stanza_t* const,
		mio_data_t* const);

int mio_handler_subscriptions_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata);

int mio_handler_subscriptions_query_node_subscribe(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_data_t * const userdata);

int mio_handler_acl_affiliations_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata);

int mio_handler_item_recent_get(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata);

int mio_handler_node_register(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_data_t * const mio_data);

int mio_handler_password_change(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_data_t * const mio_data);

int mio_handler_subscribe_secure(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_data_t * const mio_data);

int mio_handler_pubsub_data_receive(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata);

int mio_conn_handler_presence_send(mio_conn_t * const conn,
		mio_conn_event_t event, mio_response_t *response, void *userdata);

int mio_handler_collection_children_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata);

int mio_handler_collection_parents_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata);

int mio_handler_meta_query(mio_conn_t * const conn, mio_stanza_t * const stanza,
		mio_response_t *response, void *userdata);

int mio_handler_node_type_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata);

int mio_handler_schedule_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata);

int mio_handler_references_query(mio_conn_t * const conn,
		mio_stanza_t * const stanza, mio_response_t *response, void *userdata);

#endif /* HANDLERS_H_ */
