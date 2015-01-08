/******************************************************************************
 *  Mortar IO (MIO) Library
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

/**
 * @addtogroup Authentication
 * @addtogroup PubSub
 * @addtogroup Meta
 * @addtogroup Connection
 * @addtogroup Communication
 * @addtogroup Geolocation
 * @addtogroup Actuate
 * @addtogroup Collection
 * @addtogroup Core
 * @addtogroup Internal
 * @addtogroup Stanza
 * @addtogroup Scheduler
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mio.h>
#include <uuid/uuid.h>
#include <fcntl.h>
#include <errno.h>
#include <mio_handlers.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif

// Function prototypes
static void *_mio_run(mio_handler_data_t *mio_handler_data);
static int _mio_request_add(mio_conn_t *conn, const char *id,
		mio_handler handler, mio_handler_type_t handler_type,
		mio_response_t *response, mio_request_t *request);
static mio_request_t *_mio_request_new();
static void _mio_request_free(mio_request_t *request);
static mio_handler_data_t *_mio_handler_data_new();
static int _mio_request_delete(mio_conn_t *conn, char *id);
static int _mio_send_blocking(mio_conn_t *conn, mio_stanza_t *stanza,
		mio_handler handler, mio_response_t * response);
static int _mio_item_transducer_value_add(mio_stanza_t *item,
		const char *deviceID, const char *id, const char *typed_val,
		const char *raw_val, const char *timestamp, int actuate);
static mio_stanza_t *_mio_pubsub_stanza_new(mio_conn_t *conn, const char *node);
static mio_stanza_t *_mio_pubsub_set_stanza_new(mio_conn_t *conn,
		const char *node);
static mio_stanza_t *_mio_pubsub_get_stanza_new(mio_conn_t *conn,
		const char *node);
static mio_stanza_t *_mio_pubsub_iq_stanza_new(mio_conn_t *conn,
		const char *node);
static mio_stanza_t *_mio_pubsub_iq_get_stanza_new(mio_conn_t *conn,
		const char *node);
//static mio_stanza_t *_mio_pubsub_iq_set_stanza_new(mio_conn_t *conn, const char *node);
static void _mio_geoloc_element_add_double(mio_conn_t *conn,
		xmpp_stanza_t *geoloc_stanza, char* element_name, double* element_value);
static void _mio_geoloc_element_add_string(mio_conn_t *conn,
		xmpp_stanza_t *geoloc_stanza, char* element_name, char* element_text);
static void _mio_geoloc_print(mio_geoloc_t *geoloc, char* tabs);
static void _mio_event_renumber(mio_event_t *events);
static int _mio_reference_meta_type_overwrite_publish(mio_conn_t *conn,
		char *node, char *ref_node, mio_reference_type_t ref_type,
		mio_meta_type_t ref_meta_type, mio_response_t *response);
static void _mio_recurrence_element_add(mio_conn_t *conn, xmpp_stanza_t *parent,
		char *name, char *text);
static mio_stanza_t *_mio_recurrence_to_stanza(mio_conn_t *conn,
		mio_recurrence_t* rec);
static void _mio_recurrence_merge(mio_recurrence_t* rec_to_update,
		mio_recurrence_t *rec);

mio_log_level_t _mio_log_level = MIO_LEVEL_ERROR;

/**
 * @ingroup Internal
 * Function to unblock a single thread threads waiting on the condition cond. Releases the mutex and updates the predicate.
 *
 * @param cond Condition to signal
 * @param mutex Mutex to release
 * @param predicate Predicate to update
 * @returns 0 on success, otherwise non-zero value indicating error.
 */
int _mio_cond_signal(pthread_cond_t *cond, pthread_mutex_t *mutex,
		int *predicate) {
	int err;
	err = pthread_mutex_lock(mutex);
	if (err)
		return err;
	*predicate = 1;
	err = pthread_cond_signal(cond);
	if (err)
		return err;
	err = pthread_mutex_unlock(mutex);
	return err;
}

/**
 * @ingroup Internal
 * Function to unblock all threads waiting on the condition cond. Releases the mutex and updates the predicate.
 *
 * @param cond Condition to signal
 * @param mutex Mutex to release
 * @param predicate Predicate to update
 * @returns 0 on success, otherwise non-zero value indicating error.
 */
int _mio_cond_broadcast(pthread_cond_t *cond, pthread_mutex_t *mutex,
		int *predicate) {
	int err;
	err = pthread_mutex_lock(mutex);
	if (err)
		return err;
	*predicate = 1;
	err = pthread_cond_broadcast(cond);
	if (err)
		return err;
	err = pthread_mutex_unlock(mutex);
	return err;
}

/**
 * @ingroup Core
 * Function to extract the XMPP server address from a JID.
 *
 * @param jid The JID to extract the server address from
 * @returns A pointer to the first character of the server address inside of the JID that was provided.
 */
char *_mio_get_server(const char *jid) {

	// Locate @ symbol in jid
	char *loc = strpbrk(jid, "@");
	if (loc != NULL )
		return loc += 1;
	else
		return NULL ;
}

/**
 * @ingroup Stanza
 * Internal function to set attributes of an xmpp stanza struct.
 *
 * @param stanza An XMPP stanza struct to add attrubutes to
 * @param attrs A 2D array of attributes
 */
static void _mio_parser_set_attributes(xmpp_stanza_t *stanza,
		const XML_Char **attrs) {
	int i;

	if (!attrs)
		return;

	for (i = 0; attrs[i]; i += 2) {
		xmpp_stanza_set_attribute(stanza, attrs[i], attrs[i + 1]);
	}
}

/**
 * @ingroup Stanza
 * Internal function called by expat to parse each element of an XML string
 *
 * @param userdata A pointer to user specified data passed into expat. Unused.
 * @param name The name of the current element.
 * @param attrs A 2D array of the attributes associated with the current element.
 */
static void _mio_parser_start_element(void *userdata, const XML_Char *name,
		const XML_Char **attrs) {
	mio_parser_t *parser = (mio_parser_t *) userdata;
	xmpp_stanza_t *child;

	if (!parser->stanza) {
		/* starting a new toplevel stanza */
		parser->stanza = xmpp_stanza_new(parser->ctx);
		if (!parser->stanza) {
			/* FIXME: can't allocate, disconnect */
		}
		xmpp_stanza_set_name(parser->stanza, name);
		_mio_parser_set_attributes(parser->stanza, attrs);
	} else {
		/* starting a child of parser->stanza */
		child = xmpp_stanza_new(parser->ctx);
		if (!child) {
			/* FIXME: can't allocate, disconnect */
		}
		xmpp_stanza_set_name(child, name);
		_mio_parser_set_attributes(child, attrs);

		/* add child to parent */
		xmpp_stanza_add_child(parser->stanza, child);

		/* the child is owned by the toplevel stanza now */
		xmpp_stanza_release(child);

		/* make child the current stanza */
		parser->stanza = child;
	}
	parser->depth++;
}

/**
 * @ingroup Stanza
 * Internal function called by expat when the end of an element being parsed was reached
 *
 * @param userdata A pointer to user specified data passed into expat. Unused.
 * @param name The name of the current element.
 */
static void _mio_parser_end_element(void *userdata, const XML_Char *name) {
	mio_parser_t *parser = (mio_parser_t *) userdata;

	parser->depth--;

	if (parser->depth != 0 && parser->stanza->parent) {
		/* we're finishing a child stanza, so set current to the parent */
		parser->stanza = parser->stanza->parent;
	}
}

/**
 * @ingroup Stanza
 * Internal function called by expat to parse an XML string into an xmpp stanza.
 *
 * @param userdata A pointer to user specified data passed into expat. Unused.
 * @param s Pointer to the string currently being being parsed.
 * @param len The length of the string currently being parsed
 */
static void _mio_parser_characters(void *userdata, const XML_Char *s, int len) {
	mio_parser_t *parser = (mio_parser_t *) userdata;
	xmpp_stanza_t *stanza;

	if (parser->depth < 2)
		return;

	/* create and populate stanza */
	stanza = xmpp_stanza_new(parser->ctx);
	if (!stanza) {
		/* FIXME: allocation error, disconnect */
		return;
	}
	xmpp_stanza_set_text_with_size(stanza, s, len);

	xmpp_stanza_add_child(parser->stanza, stanza);
	xmpp_stanza_release(stanza);
}

/**
 * @ingroup Stanza
 * Internal function to reset the mio parser.
 *
 * @param parser A pointer to the parser to be reset.
 * @returns MIO_OK on success, MIO_ERROR_PARSER otherwise.
 */
int _mio_parser_reset(mio_parser_t *parser) {
	if (parser->expat)
		XML_ParserFree(parser->expat);

	if (parser->stanza)
		xmpp_stanza_release(parser->stanza);

	parser->expat = XML_ParserCreate(NULL );
	if (!parser->expat)
		return MIO_ERROR_PARSER;

	parser->depth = 0;
	parser->stanza = NULL;

	XML_SetUserData(parser->expat, parser);
	XML_SetElementHandler(parser->expat, _mio_parser_start_element,
			_mio_parser_end_element);
	XML_SetCharacterDataHandler(parser->expat, _mio_parser_characters);

	return MIO_OK;
}

/**
 * @ingroup Stanza
 * Allocate a new mio parser. Resets the parser on creation.
 *
 * @param conn An active mio connection struct which will be used to allocate the stanzas being produced.
 * @returns A new mio parser.
 */
mio_parser_t *mio_parser_new(mio_conn_t *conn) {
	mio_parser_t *parser;

	parser = malloc(sizeof(mio_parser_t));
	if (parser != NULL ) {
		parser->ctx = conn->xmpp_conn->ctx;
		parser->expat = NULL;
		parser->userdata = conn->xmpp_conn;
		parser->depth = 0;
		parser->stanza = NULL;

		_mio_parser_reset(parser);
	}
	return parser;
}

/**
 * @ingroup Stanza
 * Frees a mio parser.
 *
 * @param parser mio parser to be freed.
 */
void mio_parser_free(mio_parser_t *parser) {
	if (parser->expat)
		XML_ParserFree(parser->expat);
	free(parser);
}

/**
 * @ingroup Stanza
 * Parses an XML string into a mio_stanza_t struct.
 *
 * @param parser mio parser to do the parsing.
 * @param string XML string to be parsed.
 * @returns The parsed XML string as a mio stanza.
 */
mio_stanza_t *mio_parse(mio_parser_t *parser, char *string) {
	mio_stanza_t *stanza = malloc(sizeof(mio_stanza_t));
	XML_Parse(parser->expat, string, strlen(string), 0);
	stanza->xmpp_stanza = parser->stanza;
	parser->stanza = NULL;
	parser->depth = 0;
	return stanza;
}

/**
 * @ingroup Stanza
 * Converts a mio stanza to a string.
 *
 * @param stanza The mio stanza to be converted to a string.
 * @param buf A pointer to an unallocated 2D character buffer to store the string.
 * @param buflen A pointer to an integer to contain the length of the outputted string.
 * @returns 0 on success, otherwise non-zero integer on error.
 */
int mio_stanza_to_text(mio_stanza_t *stanza, char **buf, int *buflen) {
	return xmpp_stanza_to_text(stanza->xmpp_stanza, buf, (size_t*) buflen);
}

/**
 * @ingroup Stanza
 * Parses a mio stanza into a mio response.

 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @param stanza The mio stanza to be converted to a mio response.
 * @returns A mio response containing the parsed stanza.
 */
mio_response_t *mio_stanza_to_response(mio_conn_t *conn, mio_stanza_t *stanza) {
	mio_response_t *response = mio_response_new();
	mio_handler_pubsub_data_receive(conn, stanza, response, NULL );
	return response;
}

/*void mio_item_schedule_event_add(mio_conn_t *conn, mio_stanza_t *item,
 const char *transducer_name, const char *transducer_value,
 const char *info, const char *time, const char *recurrence) {
 xmpp_stanza_t *event = NULL;

 // Create a new event stanza
 event = xmpp_stanza_new(conn->xmpp_conn->ctx);
 xmpp_stanza_set_name(event, "event");
 xmpp_stanza_set_attribute(event, "time", time);
 xmpp_stanza_set_attribute(event, "name", transducer_name);
 xmpp_stanza_set_attribute(event, "value", transducer_value);
 xmpp_stanza_set_attribute(event, "info", info);
 xmpp_stanza_set_attribute(event, "recurrence", recurrence);

 // Build xmpp message
 xmpp_stanza_add_child(item->xmpp_stanza->children, event);
 xmpp_stanza_release(event);
 }*/

/**
 * @ingroup Scheduler
 * Allocates a new mio recurrence. All values inside the returned struct are zeroed.
 *
 * @returns A pointer to a newly allocated mio recurrence.
 */
mio_recurrence_t *mio_recurrence_new() {
	mio_recurrence_t *rec = malloc(sizeof(mio_recurrence_t));
	memset(rec, 0, sizeof(mio_recurrence_t));
	return rec;
}

/**
 * @ingroup Scheduler
 * Frees a mio recurrence.
 *
 * @param rec mio recurrence to free.
 */
void mio_recurrence_free(mio_recurrence_t *rec) {
	if (rec->byday != NULL )
		free(rec->byday);
	if (rec->bymonth != NULL )
		free(rec->bymonth);
	if (rec->count != NULL )
		free(rec->count);
	if (rec->freq != NULL )
		free(rec->freq);
	if (rec->interval != NULL )
		free(rec->interval);
	if (rec->until != NULL )
		free(rec->until);
	if (rec->exdate != NULL )
		free(rec->exdate);
	free(rec);
}

/**
 * @ingroup Scheduler
 * Internal function to add an element to mio recurrence.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @param parent The parent xmpp stanza to add the element to
 * @param name A string containing the name of the element to add.
 * @param text A string containing the text of the element to add.
 */
static void _mio_recurrence_element_add(mio_conn_t *conn, xmpp_stanza_t *parent,
		char *name, char *text) {
	xmpp_stanza_t *child, *child_text;
	child = xmpp_stanza_new(conn->xmpp_conn->ctx);
	child_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(child, name);
	xmpp_stanza_set_text(child_text, text);
	xmpp_stanza_add_child(child, child_text);
	xmpp_stanza_add_child(parent, child);
	xmpp_stanza_release(child_text);
	xmpp_stanza_release(child);
}

/**
 * @ingroup Scheduler
 * Internal function to merge a new occurrence with an existing one. Any members populated in rec will be used to overwrite the corresponding members in rec_to_update.
 *
 * @param rec_to_update The mio reference to be updated
 * @param rec_to_update The mio reference containing the up to date data.
 */
static void _mio_recurrence_merge(mio_recurrence_t* rec_to_update,
		mio_recurrence_t *rec) {
	if (rec->byday != NULL ) {
		if (rec_to_update->byday != NULL )
			free(rec_to_update->byday);
		rec_to_update->byday = strdup(rec->byday);
	}
	if (rec->bymonth != NULL ) {
		if (rec_to_update->bymonth != NULL )
			free(rec_to_update->bymonth);
		rec_to_update->bymonth = strdup(rec->bymonth);
	}
	if (rec->count != NULL ) {
		if (rec_to_update->count != NULL )
			free(rec_to_update->count);
		rec_to_update->count = strdup(rec->count);
	}
	if (rec->freq != NULL ) {
		if (rec_to_update->freq != NULL )
			free(rec_to_update->freq);
		rec_to_update->freq = strdup(rec->freq);
	}
	if (rec->interval != NULL ) {
		if (rec_to_update->interval != NULL )
			free(rec_to_update->interval);
		rec_to_update->interval = strdup(rec->interval);
	}
	if (rec->until != NULL ) {
		if (rec_to_update->until != NULL )
			free(rec_to_update->until);
		rec_to_update->until = strdup(rec->until);
	}
	if (rec->exdate != NULL ) {
		if (rec_to_update->exdate != NULL )
			free(rec_to_update->exdate);
		rec_to_update->exdate = strdup(rec->exdate);
	}
}

/**
 * @ingroup Scheduler
 * Internal function to convert a mio recurrence to a mio stanza.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @param rec The mio reference to be converted to a mio stanza
 * @returns A mio stanza containing the recurrence.
 */
static mio_stanza_t *_mio_recurrence_to_stanza(mio_conn_t *conn,
		mio_recurrence_t* rec) {
	xmpp_stanza_t *recurrence;
	mio_stanza_t *ret = mio_stanza_new(conn);

	// Create recurrence stanza, children and add them
	recurrence = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(recurrence, "recurrence");
	if (rec->freq != NULL )
		_mio_recurrence_element_add(conn, recurrence, "freq", rec->freq);
	if (rec->interval != NULL )
		_mio_recurrence_element_add(conn, recurrence, "interval",
				rec->interval);
	if (rec->count != NULL )
		_mio_recurrence_element_add(conn, recurrence, "count", rec->count);
	if (rec->until != NULL )
		_mio_recurrence_element_add(conn, recurrence, "until", rec->until);
	if (rec->bymonth != NULL )
		_mio_recurrence_element_add(conn, recurrence, "bymonth", rec->bymonth);
	if (rec->byday != NULL )
		_mio_recurrence_element_add(conn, recurrence, "byday", rec->bymonth);
	if (rec->exdate != NULL )
		_mio_recurrence_element_add(conn, recurrence, "exdate", rec->exdate);

	// Add recurrence stanza to mio_stanza_t struct and return
	ret->xmpp_stanza = recurrence;
	return ret;
}

/**
 * @ingroup Scheduler
 * Converts a linked list of mio events to a mio item stanza
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @param event A linked list of mio events to be converted.
 * @returns A mio stanza containing the events.
 */
mio_stanza_t *mio_schedule_event_to_item(mio_conn_t* conn, mio_event_t *event) {
	mio_event_t *curr;
	xmpp_stanza_t *event_stanza, *schedule_stanza;
	char id[8];
	mio_stanza_t *recurrence;
	mio_stanza_t *item = mio_pubsub_item_new(conn, "schedule");

	schedule_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(schedule_stanza, "schedule");
	for (curr = event; curr != NULL ; curr = curr->next) {
		event_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(event_stanza, "event");

		// Populate stanza with attributes from event
		snprintf(id, 8, "%u", curr->id);
		xmpp_stanza_set_attribute(event_stanza, "id", id);
		if (curr->info != NULL )
			xmpp_stanza_set_attribute(event_stanza, "info", curr->info);
		if (curr->time != NULL )
			xmpp_stanza_set_attribute(event_stanza, "time", curr->time);
		if (curr->tranducer_name != NULL )
			xmpp_stanza_set_attribute(event_stanza, "transducerName",
					curr->tranducer_name);
		if (curr->transducer_value != NULL )
			xmpp_stanza_set_attribute(event_stanza, "transducerValue",
					curr->transducer_value);
		if (curr->recurrence != NULL ) {
			recurrence = _mio_recurrence_to_stanza(conn, curr->recurrence);
			xmpp_stanza_add_child(event_stanza, recurrence->xmpp_stanza);
		}

		xmpp_stanza_add_child(schedule_stanza, event_stanza);
		xmpp_stanza_release(event_stanza);
	}
	xmpp_stanza_add_child(item->xmpp_stanza, schedule_stanza);
	xmpp_stanza_release(schedule_stanza);

	return item;
}

/**
 * @ingroup Scheduler
 * Internal function to renumber a linked list of mio events from 0 to n.
 *
 * @param events A linked list of mio events to be renumbered.
 */
static void _mio_event_renumber(mio_event_t *events) {
	mio_event_t *curr;
	int i = 0;
	for (curr = events; curr != NULL ; curr = curr->next) {
		curr->id = i;
		i++;
	}
}

/**
 * @ingroup Scheduler
 * Removes an event according to its ID from a linked list of mio events.
 *
 * @param events The linked list of mio events to remove the event from
 * @param id The integer ID of the event to be removed.
 * @returns A pointer to the first element of the linked list of mio events which had the event removed. Will return NULL if no events remain in the list.
 */
mio_event_t *mio_schedule_event_remove(mio_event_t *events, int id) {
	mio_event_t *prev = NULL, *curr;
	curr = events;
	while (curr != NULL ) {
		if (curr->id == id) {
			if (prev == NULL ) {
				curr = curr->next;
				_mio_event_renumber(curr);
				return curr;
			}
			prev->next = curr->next;
			curr->next = NULL;
			mio_event_free(curr);
			_mio_event_renumber(events);
			return events;
		}
		prev = curr;
		curr = curr->next;
	}
	_mio_event_renumber(events);
	return events;
}

/**
 * @ingroup Scheduler
 * Merge a new event with an existing one contained in a linked list of mio event. Events are merged by ID.
 *
 * @param event_to_update A linked list of mio events to be updated.
 * @param event The mio event containing the up to date data.
 */
void mio_schedule_event_merge(mio_event_t *event_to_update, mio_event_t *event) {
	mio_event_t *curr;
	while (event != NULL ) {
		for (curr = event_to_update; curr != NULL ; curr = curr->next) {
			if (curr->id == event->id)
				break;
		}
		if (curr == NULL ) {
			curr = mio_event_tail_get(event_to_update);
			curr->next = mio_event_new();
			curr = curr->next;
		}
		if (event->info != NULL ) {
			if (curr->info != NULL )
				free(curr->info);
			curr->info = strdup(event->info);
		}
		if (event->recurrence != NULL ) {
			if (curr->recurrence != NULL )
				mio_recurrence_free(curr->recurrence);
			curr->recurrence = mio_recurrence_new();
			_mio_recurrence_merge(curr->recurrence, event->recurrence);
		}
		if (event->time != NULL ) {
			if (curr->time != NULL )
				free(curr->time);
			curr->time = strdup(event->time);
		}
		if (event->tranducer_name != NULL ) {
			if (curr->tranducer_name != NULL )
				free(curr->tranducer_name);
			curr->tranducer_name = strdup(event->tranducer_name);
		}
		if (event->transducer_value != NULL ) {
			if (curr->transducer_value != NULL )
				free(curr->transducer_value);
			curr->transducer_value = strdup(event->transducer_value);
		}
		event = event->next;
	}
// Renumber event IDs
	_mio_event_renumber(event_to_update);
}

/**
 * @ingroup Scheduler
 * Merges an existing schedule at an XMPP event node with a linked list of events. Events are merged by ID.
 *
 * @param conn A pointer to an active mio connection.
 * @param node A string containing the JID of the event node to publish to.
 * @param event A linked list of events containing the up to date data to be merged.
 * @param response A pointer to an allocated mio response which will contain the XMPP server's response to the publication being sent out.
 * @returns MIO_OK on success, otherwise a non-zero integer error number.
 */
int mio_schedule_event_merge_publish(mio_conn_t *conn, char *node,
		mio_event_t *event, mio_response_t *response) {
	int err;
	mio_packet_t *packet;
	mio_event_t *query_events;
	mio_stanza_t *item;
	mio_response_t *query_response = mio_response_new();

// Query existing schedule
	mio_schedule_query(conn, node, query_response);
	if (query_response->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t*) query_response->response;
		// If the node has a schedule, merge it with the new events
		if (packet->type != MIO_PACKET_SCHEDULE) {
			response->response = query_response->response;
			response->response_type = query_response->response_type;
			query_response->response = NULL;
			mio_response_free(query_response);
			return MIO_ERROR_UNEXPECTED_PAYLOAD;
		}
		query_events = (mio_event_t*) packet->payload;
		if(query_events != NULL){
			mio_schedule_event_merge(query_events, event);
			item = mio_schedule_event_to_item(conn, query_events);
		}else{
			_mio_event_renumber(event);
			item = mio_schedule_event_to_item(conn, event);
		}
	} else {
		// Ensure that events are properly numbered
		_mio_event_renumber(event);
		item = mio_schedule_event_to_item(conn, event);
	}
	err = mio_item_publish(conn, item, node, response);
	mio_response_free(query_response);
	return err;
}

/**
 * @ingroup Scheduler
 * Removes an event from an existing schedule at an XMPP event node.
 *
 * @param conn A pointer to an active mio connection.
 * @param node A string containing the JID of the event node to publish to.
 * @param id The integer ID of the event to be removed.
 * @param response A pointer to an allocated mio response which will contain the XMPP server's response to the publication being sent out.
 * @returns MIO_OK on success, otherwise a non-zero integer error number.
 */
int mio_schedule_event_remove_publish(mio_conn_t *conn, char *node, int id,
		mio_response_t *response) {
	int err;
	mio_packet_t *packet;
	mio_event_t *query_events;
	mio_stanza_t *item;
	mio_response_t *query_response = mio_response_new();

// Query existing schedule
	mio_schedule_query(conn, node, query_response);
	if (query_response->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t*) query_response->response;
		// If the node has a schedule, merge it with the new events
		if (packet->type != MIO_PACKET_SCHEDULE) {
			response->response = query_response->response;
			response->response_type = query_response->response_type;
			query_response->response = NULL;
			mio_response_free(query_response);
			return MIO_ERROR_UNEXPECTED_PAYLOAD;
		}
		query_events = (mio_event_t*) packet->payload;
		query_events = mio_schedule_event_remove(query_events, id);
		packet->payload = query_events;
		item = mio_schedule_event_to_item(conn, query_events);
	} else {
		response->response = query_response->response;
		response->response_type = query_response->response_type;
		query_response->response = NULL;
		mio_response_free(query_response);
		return MIO_ERROR_UNEXPECTED_RESPONSE;
	}
	err = mio_item_publish(conn, item, node, response);
	mio_response_free(query_response);
	return err;
}

/**
 * @ingroup Scheduler
 * Queries an XMPP event node for its current schedule.
 *
 * @param conn A pointer to an active mio connection.
 * @param node A string containing the JID of the event node to publish to.
 * @param response A pointer to an allocated mio response which will contain the XMPP server's response to the query being sent out.
 * @returns MIO_OK on success, otherwise a non-zero integer error number.
 */
int mio_schedule_query(mio_conn_t *conn, const char *node,
		mio_response_t *response) {

	return mio_item_recent_get(conn, node, response, 1, "schedule",
			(mio_handler) mio_handler_schedule_query);
}

/**
 * @ingroup Connection
 * Creates a new mio connection, which will output logging data at the level specified in log_level.
 *
 * @param log_level MIO_LEVEL_ERROR for only logging errors, MIO_LEVEL_WARN for logging warnings and errors, MIO_LEVEL_INFO for logging info, warnings and errors, MIO_LEVEL_DEBUG for logging debug information, info, warnings and errors.
 * @returns A newly allocated and initialized mio conn.
 */
mio_conn_t *mio_conn_new(mio_log_level_t log_level) {
	mio_conn_t *conn = malloc(sizeof(mio_conn_t));
	xmpp_ctx_t *ctx;
	xmpp_log_t *log;

// Make conn_mutex recursive so that it can be locked multiple times by the same thread
	pthread_mutexattr_t conn_mutex_attr;
	pthread_mutexattr_init(&conn_mutex_attr);
	pthread_mutexattr_settype(&conn_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	conn->xmpp_conn = NULL;
	conn->presence_status = MIO_PRESENCE_UNKNOWN;
	pthread_mutex_init(&conn->event_loop_mutex, NULL );
	pthread_mutex_init(&conn->send_request_mutex, NULL );
	pthread_mutex_init(&conn->pubsub_rx_queue_mutex, NULL );
	pthread_mutex_init(&conn->conn_mutex, NULL );
//pthread_mutexattr_destroy(&conn_mutex_attr);
	pthread_cond_init(&conn->send_request_cond, NULL );
	pthread_cond_init(&conn->conn_cond, NULL );
	pthread_rwlock_init(&conn->mio_hash_lock, NULL );
	conn->send_request_predicate = 0;
	conn->conn_predicate = 0;
	conn->retries = 0;
	TAILQ_INIT(&conn->pubsub_rx_queue);
	conn->pubsub_rx_queue_len = 0;
	conn->pubsub_rx_listening = 0;
	conn->mio_request_table = NULL;
	conn->mio_open_requests = NULL;

	switch (log_level) {
	case MIO_LEVEL_DEBUG:
		log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);
		break;
	case MIO_LEVEL_ERROR:
		log = xmpp_get_default_logger(XMPP_LEVEL_ERROR);
		break;
	case MIO_LEVEL_INFO:
		log = xmpp_get_default_logger(XMPP_LEVEL_INFO);
		break;
	case MIO_LEVEL_WARN:
		log = xmpp_get_default_logger(XMPP_LEVEL_WARN);
		break;
	default:
		log = xmpp_get_default_logger(XMPP_LEVEL_ERROR);
		break;
	}

	_mio_log_level = log_level;
	ctx = xmpp_ctx_new(NULL, log);
// Create a connection
	conn->xmpp_conn = xmpp_conn_new(ctx);

	return conn;
}

/**
 * @ingroup Connection
 * Frees a mio conn.
 *
 * @param conn A pointer to the allocated mio conn to be freed.
 * */
void mio_conn_free(mio_conn_t *conn) {
	if (conn->xmpp_conn != NULL ) {
		//if(conn->xmpp_conn->ctx != NULL)
		//	xmpp_ctx_free(conn->xmpp_conn->ctx);
		xmpp_conn_release(conn->xmpp_conn);
	}
	pthread_mutex_destroy(&conn->event_loop_mutex);
	pthread_mutex_destroy(&conn->send_request_mutex);
	pthread_mutex_destroy(&conn->conn_mutex);
	pthread_cond_destroy(&conn->send_request_cond);
	pthread_cond_destroy(&conn->conn_cond);
	free(conn);
}

/**
 * @ingroup Core
 * Allocates and initializes a new mio response.
 *
 * @returns A pointer to a newly allocated an initialized mio response.
 */
mio_response_t *mio_response_new() {
	mio_response_t *response = (mio_response_t *) malloc(
			sizeof(mio_response_t));
	response->ns = NULL;
	response->name = NULL;
	response->type = NULL;
	response->response = NULL;
	response->response_type = MIO_RESPONSE_UNKNOWN;
	response->stanza = NULL;
	strcpy(response->id, "");
	return response;
}

/**
 * @ingroup Core
 * Frees an allocated mio response.
 *
 * @param A pointer to the mio response to be freed.
 */
void mio_response_free(mio_response_t *response) {
// TODO: free other response types
	switch (response->response_type) {
	case MIO_RESPONSE_ERROR:
		_mio_response_error_free(response->response);
		break;
	case MIO_RESPONSE_PACKET:
		mio_packet_free(response->response);
		break;
	default:
		break;
	}

	if (response->name != NULL )
		free(response->name);
	if (response->ns != NULL )
		free(response->ns);
	if (response->type != NULL )
		free(response->type);
	if (response->stanza != NULL )
		mio_stanza_free(response->stanza);
	free(response);
}

/**
 * @ingroup Core
 * Allocates and initializes a new mio error response.
 *
 * @returns A pointer to a newly allocated an initialized mio response.
 */
mio_response_error_t *_mio_response_error_new() {
	mio_response_error_t *error = malloc(sizeof(mio_response_error_t));
	return error;
}

/**
 * @ingroup Core
 * Frees an allocated mio error response.
 *
 * @param A pointer to the mio error response to be freed.
 */
void _mio_response_error_free(mio_response_error_t* error) {
	free(error->description);
	free(error);
}

/**
 * @ingroup Internal
 * Internal function to enqueue a mio response to the received pubsub queue.
 *
 * @param conn A pointer to an active mio connection.
 * @param response A pointer to the mio response to enqueue
 */
void _mio_pubsub_rx_queue_enqueue(mio_conn_t *conn, mio_response_t *response) {
	mio_response_t *temp = NULL;

	if (conn->pubsub_rx_queue_len >= MIO_PUBSUB_RX_QUEUE_MAX_LEN) {
		mio_warn(
				"Pubsub RX queue has reached it's maximum length, dropping oldest response");
		pthread_mutex_lock(&conn->pubsub_rx_queue_mutex);
		temp = (mio_response_t*) TAILQ_FIRST(&conn->pubsub_rx_queue);
		TAILQ_REMOVE(&conn->pubsub_rx_queue, temp, responses);
		TAILQ_INSERT_TAIL(&conn->pubsub_rx_queue, response, responses);
		pthread_mutex_unlock(&conn->pubsub_rx_queue_mutex);
	} else {
		pthread_mutex_lock(&conn->pubsub_rx_queue_mutex);
		TAILQ_INSERT_TAIL(&conn->pubsub_rx_queue, response, responses);
		conn->pubsub_rx_queue_len++;
		pthread_mutex_unlock(&conn->pubsub_rx_queue_mutex);
	}
}

/**
 * @ingroup Internal
 * Internal function to pop a mio response off the received pubsub queue.
 *
 * @param conn A pointer to an active mio connection.
 * @returns A pointer to the mio response that was popped off the received pubsub queue.
 */
mio_response_t *_mio_pubsub_rx_queue_dequeue(mio_conn_t *conn) {
	mio_response_t *response = NULL;
	pthread_mutex_lock(&conn->pubsub_rx_queue_mutex);
	if (conn->pubsub_rx_queue_len > 0) {
		response = TAILQ_FIRST(&conn->pubsub_rx_queue);
		TAILQ_REMOVE(&conn->pubsub_rx_queue, response, responses);
		conn->pubsub_rx_queue_len--;
		pthread_mutex_unlock(&conn->pubsub_rx_queue_mutex);
		return response;
	} else {
		pthread_mutex_unlock(&conn->pubsub_rx_queue_mutex);
		return NULL ;
	}
}

/**
 * @ingroup Internal
 * Internal function to allocate an initialize a new mio request
 *
 * @returns A newly allocated and initialized mio request
 */
static mio_request_t *_mio_request_new() {
	mio_request_t *request = (mio_request_t*) malloc(sizeof(mio_request_t));
	request->predicate = 0;
	request->handler_type = 0;
	request->response = NULL;
	request->handler = NULL;
	strcpy(request->id, "");
	pthread_cond_init(&request->cond, NULL );
	pthread_mutex_init(&request->mutex, NULL );

	return request;
}

/**
 * @ingroup Internal
 * Internal function to free an initialized mio request.
 *
 * @param request The allocated mio request to be freed.
 */
static void _mio_request_free(mio_request_t *request) {
	pthread_cond_destroy(&request->cond);
	pthread_mutex_destroy(&request->mutex);
	free(request);
}

/**
 * @ingroup Stanza
 * Frees a mio stanza.
 *
 * @param The pointer to the mio stanza to be freed.
 */
void mio_stanza_free(mio_stanza_t *stanza) {
	xmpp_stanza_release(stanza->xmpp_stanza);
	free(stanza);
}

/**
 * @ingroup Stanza
 * Allocates and initializes a new mio stanza.
 *
 * @returns A pointer to the newly allocated and initialized mio stanza
 */
mio_stanza_t *mio_stanza_new(mio_conn_t *conn) {
	mio_stanza_t *stanza = (mio_stanza_t*) malloc(sizeof(mio_stanza_t));
	stanza->xmpp_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
	return stanza;
}

/**
 * @ingroup Stanza
 * Performs a deep copy of a mio stanza.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @returns A pointer to the cloned mio stanza.
 */
mio_stanza_t *mio_stanza_clone(mio_conn_t *conn, mio_stanza_t *stanza) {
	mio_stanza_t *clone = mio_stanza_new(conn);
	clone->xmpp_stanza = xmpp_stanza_clone(stanza->xmpp_stanza);
	return clone;
}

/**
 * @ingroup Internal
 * Internal function to add a mio request to the request hash table.
 *
 * @param conn A pointer to an active mio connection to which the request should be added to.
 * @param id A string containing the UUID of the request being added.
 * @param handler A pointer to the mio handler associated with the request.
 * @param handler_type MIO_HANDLER_ID for a handler that is executed when a message matching its ID is received, MIO_HANLDER_TIMED for a handler that is executed based at fixed time intervals.
 * @param response A pointer to an allocated mio response, which will contain the server's response.
 * @param request A pointer to the mio request to be added.
 * @returns MIO_OK on success, otherwise a non-zero integer error number.
 */
static int _mio_request_add(mio_conn_t *conn, const char *id,
		mio_handler handler, mio_handler_type_t handler_type,
		mio_response_t *response, mio_request_t *request) {
	mio_request_t *find_request = NULL;

//sem_getvalue is not implemented in OSX
#ifdef __linux__
	int sem_value;
	sem_getvalue(conn->mio_open_requests, &sem_value);
	mio_debug("Remaining requests: %d", sem_value);
	if (sem_value <= 0) {
		mio_error("Cannot add request for id %s, open request table full", id);
		return MIO_ERROR_TOO_MANY_OPEN_REQUESTS;
	}
#endif
// Check if ID is already in table
	if (pthread_rwlock_wrlock(&conn->mio_hash_lock) != 0) {
		mio_error("Can't get hash table rw lock");
		return MIO_ERROR_RW_LOCK;
	}
	HASH_FIND_STR(conn->mio_request_table, id, find_request);
	if (find_request == NULL ) {
		if (strcmp(request->id, "") == 0)
			strcpy(request->id, id);
		if (response != NULL ) {
			if (strcmp(response->id, "") == 0)
				strcpy(response->id, id);
		}
		// Add given request to hash table
		HASH_ADD_STR(conn->mio_request_table, id, request);
	}
	pthread_rwlock_unlock(&conn->mio_hash_lock);
// Update members of struct
	request->handler = handler;
	request->handler_type = handler_type;
	request->response = response;
	return MIO_OK;
}

/**
 * @ingroup Internal
 * Internal function to delete a mio request from the request hash table.
 *
 * @param conn A pointer to an active mio connection from which the request should be removed from.
 * @param id A string containing the UUID of the request being deleted.
 * @returns MIO_OK on success, otherwise a non-zero integer error number.
 */
static int _mio_request_delete(mio_conn_t *conn, char *id) {
	mio_request_t *r;
	if (pthread_rwlock_wrlock(&conn->mio_hash_lock) != 0) {
		mio_error("Can't get hash table rw lock");
		return MIO_ERROR_RW_LOCK;
	}
	HASH_FIND_STR(conn->mio_request_table, id, r);
	if (r != NULL ) {
		HASH_DEL(conn->mio_request_table, r);
		pthread_rwlock_unlock(&conn->mio_hash_lock);
		_mio_request_free(r);
		sem_post(conn->mio_open_requests);
		return MIO_OK;
	}
	pthread_rwlock_unlock(&conn->mio_hash_lock);
	return MIO_ERROR_REQUEST_NOT_FOUND;
}

/**
 * @ingroup Internal
 * Internal function to get a mio request from a request hash table.
 *
 * @param conn A pointer to an active mio connection from which the request should be fetched from.
 * @param id A string containing the UUID of the request to be fetched.
 * @returns A pointer to the mio request fetched from the request hash or NULL if the request could not be found.
 */
mio_request_t *_mio_request_get(mio_conn_t *conn, char *id) {
	mio_request_t *r;
	if (pthread_rwlock_rdlock(&conn->mio_hash_lock) != 0) {
		mio_error("Can't get hash table rd lock");
		return NULL ;
	}
	HASH_FIND_STR(conn->mio_request_table, id, r);
	pthread_rwlock_unlock(&conn->mio_hash_lock);
	return r;
}

/**
 * @ingroup Internal
 * Internal function to get a mio response from a mio request.
 *
 * @param conn A pointer to an active mio connection from which the request should be fetched from.
 * @param id A string containing the UUID of the request to be fetched.
 * @returns A pointer to the mio response fetched from the specified request or NULL if the request could not be found.
 */
mio_response_t *_mio_response_get(mio_conn_t *conn, char *id) {
	mio_request_t *r = _mio_request_get(conn, id);
	if (r != NULL )
		return r->response;
	else
		return NULL ;
}

/**
 * @ingroup Internal
 * Internal function to allocate and initialize a new mio handler data struct.
 *
 * @returns A pointer to a newly allocated and initialized mio handler data struct
 */
static mio_handler_data_t *_mio_handler_data_new() {
	mio_handler_data_t *shd = (mio_handler_data_t*) malloc(
			sizeof(mio_handler_data_t));
	shd->conn = NULL;
	shd->request = NULL;
	shd->response = NULL;
	shd->handler = NULL;
	shd->conn_handler = NULL;
	shd->userdata = NULL;
	return shd;
}

/**
 * @ingroup Internal
 * Internal function to free a mio handler data struct.
 *
 * @param A pointer to the allocated mio handler data struct to be freed.
 */
void _mio_handler_data_free(mio_handler_data_t *shd) {
	free(shd);
}

/**
 * @ingroup Geolocation
 * Allocates and initializes a new mio geoloc
 *
 * @returns A pointer to the newly allocated and initialized mio geoloc.
 */
mio_geoloc_t *mio_geoloc_new() {
	mio_geoloc_t *geoloc = malloc(sizeof(mio_geoloc_t));
	memset(geoloc, 0, sizeof(mio_geoloc_t));
	return geoloc;
}

/**
 * @ingroup Geolocation
 * Frees a mio geoloc.
 *
 * @param A pointer to the allocated mio geoloc to be freed.
 */
void mio_geoloc_free(mio_geoloc_t *geoloc) {
	if (geoloc->accuracy != NULL )
		free(geoloc->accuracy);
	if (geoloc->alt != NULL )
		free(geoloc->alt);
	if (geoloc->area != NULL )
		free(geoloc->area);
	if (geoloc->bearing != NULL )
		free(geoloc->bearing);
	if (geoloc->building != NULL )
		free(geoloc->building);
	if (geoloc->country != NULL )
		free(geoloc->country);
	if (geoloc->country_code != NULL )
		free(geoloc->country_code);
	if (geoloc->datum != NULL )
		free(geoloc->datum);
	if (geoloc->description != NULL )
		free(geoloc->description);
	if (geoloc->floor != NULL )
		free(geoloc->floor);
	if (geoloc->lat != NULL )
		free(geoloc->lat);
	if (geoloc->locality != NULL )
		free(geoloc->locality);
	if (geoloc->lon != NULL )
		free(geoloc->lon);
	if (geoloc->postal_code != NULL )
		free(geoloc->postal_code);
	if (geoloc->region != NULL )
		free(geoloc->region);
	if (geoloc->room != NULL )
		free(geoloc->room);
	if (geoloc->speed != NULL )
		free(geoloc->speed);
	if (geoloc->street != NULL )
		free(geoloc->street);
	if (geoloc->text != NULL )
		free(geoloc->text);
	if (geoloc->timestamp != NULL )
		free(geoloc->timestamp);
	if (geoloc->tzo != NULL )
		free(geoloc->tzo);
	if (geoloc->uri != NULL )
		free(geoloc->uri);
	free(geoloc);
}

/**
 * @ingroup Meta
 * Allocates and initializes a new mio meta struct.
 *
 * @returns A pointer to the newly allocated and initialized mio meta struct.
 */
mio_meta_t *mio_meta_new() {
	mio_meta_t *meta = malloc(sizeof(mio_meta_t));
	memset(meta, 0, sizeof(mio_meta_t));
	return meta;
}

/**
 * @ingroup Meta
 * Frees a mio meta struct.
 *
 * @param A pointer to the allocated mio meta struct to be freed.
 */
void mio_meta_free(mio_meta_t * meta) {
	mio_transducer_meta_t *t_curr, *t_next;
	mio_property_meta_t *p_curr, *p_next;
	for (t_curr = meta->transducers; t_curr != NULL ; t_curr = t_next){
		t_next = t_curr->next;
		mio_transducer_meta_free(t_curr);
	}
	for (p_curr = meta->properties; p_curr != NULL ; p_curr = p_next){
		p_next = p_curr->next;
		mio_property_meta_free(p_curr);
	}
	if (meta->geoloc != NULL )
		mio_geoloc_free(meta->geoloc);
	if (meta->name != NULL )
		free(meta->name);
	if (meta->info != NULL )
		free(meta->info);
	if (meta->timestamp != NULL )
		free(meta->timestamp);
	free(meta);
}

/**
 * @ingroup PubSub
 * Allocates and initializes a new mio transducer value struct.
 *
 * @returns A pointer to the newly allocated and initialized mio transducer value struct.
 */
mio_transducer_value_t * mio_transducer_value_new() {
	mio_transducer_value_t *t_value = malloc(sizeof(mio_transducer_value_t));
	t_value->id = 0;
	t_value->name = NULL;
	t_value->raw_value = NULL;
	t_value->timestamp = NULL;
	t_value->type = 0;
	t_value->typed_value = NULL;
	t_value->next = NULL;
	return t_value;
}

/**
 * @ingroup PubSub
 * Frees a mio transducer value struct. Does not free any successive members in the linked list.
 *
 * @param A pointer to the allocated mio transducer value struct to be freed.
 */
void mio_transducer_value_free(mio_transducer_value_t *t_value) {
	if (t_value->name != NULL )
		free(t_value->name);
	if (t_value->raw_value != NULL )
		free(t_value->raw_value);
	if (t_value->timestamp != NULL )
		free(t_value->timestamp);
	if (t_value->typed_value != NULL )
		free(t_value->typed_value);
	free(t_value);
}

/**
 * @ingroup PubSub
 * Returns the tail of a mio transducer value linked list
 *
 * @param A pointer to an element in a mio transducer value linked list.
 * @returns A pointer to the tail of a mio transducer value linked list.
 */
mio_transducer_value_t *_mio_transducer_value_tail_get(
		mio_transducer_value_t *t_value) {
	mio_transducer_value_t *tail = NULL;

	while (t_value != NULL ) {
		tail = t_value;
		t_value = t_value->next;
	}
	return tail;
}

/**
 * @ingroup Meta
 * Allocates and initializes a new mio transducer meta struct.
 *
 * @returns A pointer to the newly allocated and initialized mio transducer meta struct.
 */
mio_transducer_meta_t * mio_transducer_meta_new() {
	mio_transducer_meta_t *t_meta = malloc(sizeof(mio_transducer_meta_t));
	memset(t_meta, 0, sizeof(mio_transducer_meta_t));
	return t_meta;
}

/**
 * @ingroup Meta
 * Frees a mio transducer meta struct. Does not free any successive members in the linked list.
 *
 * @param A pointer to the allocated mio transducer meta struct to be freed.
 */
void mio_transducer_meta_free(mio_transducer_meta_t *t_meta) {
	mio_enum_map_meta_t *e_meta, *e_meta_free;
	mio_property_meta_t *p_meta, *p_meta_free;

	if (t_meta->accuracy != NULL )
		free(t_meta->accuracy);
	if (t_meta->max_value != NULL )
		free(t_meta->max_value);
	if (t_meta->min_value != NULL )
		free(t_meta->min_value);
	if (t_meta->precision != NULL )
		free(t_meta->precision);
	if (t_meta->resolution != NULL )
		free(t_meta->resolution);
	if (t_meta->type != NULL )
		free(t_meta->type);
	if (t_meta->name != NULL )
		free(t_meta->name);
	if (t_meta->unit != NULL )
		free(t_meta->unit);
	if (t_meta->manufacturer != NULL )
		free(t_meta->manufacturer);
	if (t_meta->serial != NULL )
		free(t_meta->serial);
	if (t_meta->info != NULL )
		free(t_meta->info);
	if (t_meta->geoloc != NULL )
		mio_geoloc_free(t_meta->geoloc);
	e_meta = t_meta->enumeration;
	while (e_meta != NULL ) {
		e_meta_free = e_meta;
		e_meta = e_meta->next;
		mio_enum_map_meta_free(e_meta_free);
	}
	p_meta = t_meta->properties;
	while (p_meta != NULL ) {
		p_meta_free = p_meta;
		p_meta = p_meta->next;
		mio_property_meta_free(p_meta_free);
	}
	free(t_meta);
}

/**
 * @ingroup Meta
 * Allocates and initializes a new mio enum map meta struct.
 *
 * @returns A pointer to the newly allocated and initialized mio enum map meta struct.
 */
mio_enum_map_meta_t * mio_enum_map_meta_new() {
	mio_enum_map_meta_t *e_meta = malloc(sizeof(mio_enum_map_meta_t));
	memset(e_meta, 0, sizeof(mio_enum_map_meta_t));
	return e_meta;
}

/**
 * @ingroup Meta
 * Frees a mio enum map meta meta struct. Does not free any successive members in the linked list.
 *
 * @param A pointer to the allocated mio enum map meta struct to be freed.
 */
void mio_enum_map_meta_free(mio_enum_map_meta_t *e_meta) {
	if (e_meta->name != NULL )
		free(e_meta->name);
	if (e_meta->value != NULL )
		free(e_meta->value);
	free(e_meta);
}

/**
 * @ingroup Meta
 * Returns the tail of a mio enum map meta linked list.
 *
 * @param e_meta A pointer to an element in a mio enum map meta linked list.
 * @returns A pointer to the tail of a mio enum map meta linked list.
 */
mio_enum_map_meta_t *mio_enum_map_meta_tail_get(mio_enum_map_meta_t *e_meta) {
	mio_enum_map_meta_t *tail = NULL;

	while (e_meta != NULL ) {
		tail = e_meta;
		e_meta = e_meta->next;
	}
	return tail;
}

/**
 * @ingroup Meta
 * Performs a deep copy of a mio enum map meta linked list.
 *
 * @param e_meta A pointer to the mio enum map meta linked list to be cloned
 * @returns A pointer to the first element of the copy of the enum map meta linked list.
 */
mio_enum_map_meta_t *mio_enum_map_meta_clone(mio_enum_map_meta_t *e_meta) {
	mio_enum_map_meta_t *copy, *curr;
	copy = mio_enum_map_meta_new();
	curr = copy;
	while (e_meta != NULL ) {
		if (e_meta->name != NULL ) {
			curr->name = malloc(sizeof(char) * strlen(e_meta->name) + 1);
			strcpy(curr->name, e_meta->name);
		} else {
			mio_enum_map_meta_free(copy);
			return NULL ;
		}
		if (e_meta->value != NULL ) {
			curr->value = malloc(sizeof(char) * strlen(e_meta->value) + 1);
			strcpy(curr->value, e_meta->value);
		} else {
			mio_enum_map_meta_free(copy);
			return NULL ;
		}
		e_meta = e_meta->next;
		if (e_meta != NULL ) {
			curr->next = mio_enum_map_meta_new();
			curr = curr->next;
		}
	}
	return copy;
}

/**
 * @ingroup Meta
 * Allocates and initializes a new mio propery meta struct.
 *
 * @returns A pointer to the newly allocated and initialized mio property meta struct.
 */
mio_property_meta_t *mio_property_meta_new() {
	mio_property_meta_t *p_meta = malloc(sizeof(mio_property_meta_t));
	memset(p_meta, 0, sizeof(mio_property_meta_t));
	return p_meta;
}

/**
 * @ingroup Meta
 * Frees a mio property meta struct. Does not free any successive members in the linked list.
 *
 * @param p_meta A pointer to the allocated mio property meta struct to be freed.
 */
void mio_property_meta_free(mio_property_meta_t *p_meta) {
	if (p_meta->name != NULL )
		free(p_meta->name);
	if (p_meta->value != NULL )
		free(p_meta->value);
	free(p_meta);
}

/**
 * @ingroup Meta
 * Adds a mio property to a mio meta struct
 *
 * @param meta A pointer to the mio meta struct to which the property should be added to.
 * @param p_meta A pointer to the mio property meta struct to be added to the meta struct.
 */
void mio_property_meta_add(mio_meta_t *meta, mio_property_meta_t *p_meta) {
	mio_property_meta_t *curr = mio_property_meta_tail_get(meta->properties);
	if (curr == NULL )
		meta->properties = p_meta;
	else
		curr->next = p_meta;
}

/**
 * @ingroup Meta
 * Returns the tail of a mio property meta linked list
 *
 * @param A pointer to an element in a mio property meta linked list.
 * @returns A pointer to the tail of a mio property meta linked list.
 */
mio_property_meta_t *mio_property_meta_tail_get(mio_property_meta_t *p_meta) {
	mio_property_meta_t *tail = NULL;

	while (p_meta != NULL ) {
		tail = p_meta;
		p_meta = p_meta->next;
	}
	return tail;
}

/**
 * @ingroup Meta
 * Returns the tail of a mio transducer meta linked list
 *
 * @param A pointer to an element in a mio transducer meta linked list.
 * @returns A pointer to the tail of a mio transducer meta linked list.
 */
mio_transducer_meta_t *mio_transducer_meta_tail_get(
		mio_transducer_meta_t *t_meta) {
	mio_transducer_meta_t *tail = NULL;

	while (t_meta != NULL ) {
		tail = t_meta;
		t_meta = t_meta->next;
	}
	return tail;
}

/**
 * @ingroup Internal
 * Internal function to send out an XMPP message in a blocking fashion. The function returns once the server's response has been processed or an error occurs.
 *
 * @param conn A pointer to an active mio conn.
 * @param stanza A pointer to the mio stanza to be sent.
 * @param handler A pointer to the mio handler which should parse the server's response.
 * @param response A pointer to an allocated mio response struct which will be populated with the server's response.
 * @returns MIO_OK on success, otherwise an error.
 */
static int _mio_send_blocking(mio_conn_t *conn, mio_stanza_t *stanza,
		mio_handler handler, mio_response_t * response) {

	mio_request_t *request;
	struct timespec ts, ts_reconnect;
	struct timeval tp;
	int err;
	int send_attempts = 0;

	request = _mio_request_new();

// Set timeout
	gettimeofday(&tp, NULL );
	ts.tv_sec = tp.tv_sec;
	ts.tv_nsec = tp.tv_usec * 1000;
	ts.tv_sec += MIO_REQUEST_TIMEOUT_S;

// If we have too many open requests, wait
	sem_wait(conn->mio_open_requests);

	if (stanza->id != NULL )
		mio_handler_id_add(conn, handler, stanza->id, request, NULL, response);
	else {
		mio_error("ID of stanza to be sent is NULL");
		return MIO_ERROR_NULL_STANZA_ID;
	}

// Send out the stanza, then wait for response
	while (_mio_send_nonblocking(conn, stanza) == MIO_ERROR_CONNECTION) {
		// If sending fails, wait for connection to be reestablished
		send_attempts++;
		if (send_attempts >= MIO_SEND_RETRIES) {
			xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id,
					request->id);
			_mio_request_delete(conn, request->id);
			return MIO_ERROR_CONNECTION;
		}

		// Set timeout
		gettimeofday(&tp, NULL );
		ts_reconnect.tv_sec = tp.tv_sec;
		ts_reconnect.tv_nsec = tp.tv_usec * 1000;
		ts_reconnect.tv_sec += MIO_RECONNECTION_TIMEOUT_S;

		pthread_mutex_lock(&conn->conn_mutex);
		// Wait for connection to be established before returning
		while (!conn->conn_predicate) {
			err = pthread_cond_timedwait(&conn->conn_cond, &conn->conn_mutex,
					&ts_reconnect);
			if (err == ETIMEDOUT) {
				pthread_mutex_unlock(&conn->conn_mutex);
				xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id,
						request->id);
				_mio_request_delete(conn, request->id);
				mio_error("Sending attempt timed out");
				return MIO_ERROR_TIMEOUT;
			} else if (err != 0) {
				pthread_mutex_unlock(&conn->conn_mutex);
				xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id,
						request->id);
				_mio_request_delete(conn, request->id);
				mio_error("Conditional wait for connection attempt timed out");
				return MIO_ERROR_COND_WAIT;
			} else {
				pthread_mutex_unlock(&conn->conn_mutex);
			}
		}
	}

	while (!request->predicate) {
		err = pthread_mutex_lock(&request->mutex);
		if (err != 0) {
			mio_error("Unable to lock mutex for request %s", request->id);
			return MIO_ERROR_MUTEX;
		}
		// Wait for response from server
		err = pthread_cond_timedwait(&request->cond, &request->mutex, &ts);
		if (err == ETIMEDOUT) {
			xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id,
					request->id);
			pthread_mutex_unlock(&request->mutex);
			_mio_request_delete(conn, request->id);
			mio_error("Request with id %s timed out", request->id);
			return MIO_ERROR_TIMEOUT;
		} else if (err != 0) {
			xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id,
					request->id);
			pthread_mutex_unlock(&request->mutex);
			_mio_request_delete(conn, request->id);
			mio_error("Conditional wait for request with id %s failed",
					request->id);
			return MIO_ERROR_COND_WAIT;
		} else {
			mio_debug("Got response for request with id %s", request->id);
			response = request->response;
			pthread_mutex_unlock(&request->mutex);
			_mio_request_delete(conn, request->id);
			return MIO_OK;
		}
	}
	mio_error("Predicate of request with ID %s == 0", request->id);
	return MIO_ERROR_PREDICATE;
}

/**
 * @ingroup Internal
 * Internal function to send out an XMPP message in a non-blocking fashion. The function returns once the message has been sent out or an error occurs. No handlers are added.
 *
 * @param conn A pointer to an active mio conn.
 * @param stanza A pointer to the mio stanza to be sent.
 * @returns MIO_OK on success, otherwise an error.
 */
int _mio_send_nonblocking(mio_conn_t *conn, mio_stanza_t *stanza) {

	char *buf = NULL;
	size_t len = 0;
	int err;

// Render stanza first instead of using xmpp_send so that we can keep the mutex unlocked longer
	if (xmpp_stanza_to_text(stanza->xmpp_stanza, &buf, &len) == 0) {
		if (!conn->xmpp_conn->authenticated) {
			xmpp_free(conn->xmpp_conn->ctx, buf);
			mio_error("Sending failed because not connected to server");
			return MIO_ERROR_CONNECTION;
		}
		// Make sure we are the only one writing to the send queue
		err = pthread_mutex_lock(&conn->event_loop_mutex);
		if (err != 0) {
			mio_error("Unable to lock event loop mutex");
			return MIO_ERROR_MUTEX;
		}
		// Send request to server and forget it
		xmpp_send_raw(conn->xmpp_conn, buf, len);
		pthread_mutex_unlock(&conn->event_loop_mutex);
		xmpp_debug(conn->xmpp_conn->ctx, "conn", "SENT: %s", buf);
		xmpp_free(conn->xmpp_conn->ctx, buf);
		// Signal the _mio_run thread that it should run the event loop in case it is waiting
		_mio_cond_signal(&conn->send_request_cond, &conn->send_request_mutex,
				&conn->send_request_predicate);
	}
	return MIO_OK;
}

/**
 * @ingroup Core
 * Gets the current time and date and returns them as a string.
 *
 * @returns Pointer to a string containing the current time and date. String needs to be freed when no longer in use.
 */
char *mio_timestamp_create() {

	char fmt[64], buf[64];
	struct timeval tv;
	struct tm *tm;
	char *time_str = NULL;

	gettimeofday(&tv, NULL );
	if ((tm = localtime(&tv.tv_sec)) != NULL ) {
		strftime(fmt, sizeof fmt, "%Y-%m-%dT%H:%M:%S.%%06u%z", tm);
		time_str = malloc(strlen(fmt) + 7);
		snprintf(time_str, sizeof(buf), fmt, tv.tv_usec);
		return time_str;
	}
	return NULL ;
}

/**
 * @ingroup Internal
 * Main event loop thread which connects to the XMPP server passed into mio_connect() and processes incoming and outgoing communication.
 *
 * @param mio_handler_data mio handler data struct containing the mio conn used in the connection.
 */
static void *_mio_run(mio_handler_data_t *mio_handler_data) {

	struct timespec ts;
	struct timeval tp, tp_add;
	int err;
	mio_conn_t *conn = mio_handler_data->conn;
	xmpp_ctx_t *ctx = conn->xmpp_conn->ctx;

	tp_add.tv_sec = 0;
	tp_add.tv_usec = MIO_SEND_REQUEST_TIMEOUT;

// Initiate connection
	mio_info("Connecting to XMPP server %s using JID %s",
			mio_handler_data->conn->xmpp_conn->domain,
			mio_handler_data->conn->xmpp_conn->jid);
	err = xmpp_connect_client(mio_handler_data->conn->xmpp_conn, NULL, 0,
			mio_handler_conn_generic, mio_handler_data);
	if (err < 0) {
		mio_error("Error connecting to XMPP server %s using JID %s",
				mio_handler_data->conn->xmpp_conn->domain,
				mio_handler_data->conn->xmpp_conn->jid);
		pthread_exit((void*) MIO_ERROR_CONNECTION);
	}

// Add keepalive handler
	mio_handler_timed_add(mio_handler_data->conn,
			(mio_handler) mio_handler_keepalive, KEEPALIVE_PERIOD, NULL );

	mio_debug("Starting Event Loop");
	conn->presence_status = MIO_PRESENCE_UNKNOWN;

// Run event loop
	if (ctx->loop_status != XMPP_LOOP_NOTSTARTED) {
		mio_error("Cannot run event loop because not started");
		pthread_exit((void*) MIO_ERRROR_EVENT_LOOP_NOT_STARTED);
	}

	ctx->loop_status = XMPP_LOOP_RUNNING;
	while (ctx->loop_status == XMPP_LOOP_RUNNING) {
		// Only run the event loop if we have locked down the mutex
		// This prevents interleaving on the send queue
		err = pthread_mutex_lock(&conn->event_loop_mutex);
		if (err != 0) {
			mio_error("Unable to lock event loop mutex");
			pthread_exit((void*) MIO_ERROR_MUTEX);
		}
		xmpp_run_once(ctx, MIO_EVENT_LOOP_TIMEOUT);
		pthread_mutex_unlock(&conn->event_loop_mutex);

		// Set timeout
		gettimeofday(&tp, NULL );
		ts.tv_sec = tp.tv_sec;
		timeradd(&tp, &tp_add, &tp);
		ts.tv_nsec = tp.tv_usec * 1000;

		err = pthread_mutex_lock(&conn->send_request_mutex);
		if (err != 0) {
			mio_error("Unable to lock send request mutex");
			pthread_exit((void*) MIO_ERROR_MUTEX);
		}
		// Wait for MIO_SEND_REQUEST_TIMEOUT or until new send request comes in (whichever is first) to run event loop again
		while (!conn->send_request_predicate) {
			err = pthread_cond_timedwait(&conn->send_request_cond,
					&conn->send_request_mutex, &ts);
			// If we time out, run the event loop
			if (err == ETIMEDOUT)
				conn->send_request_predicate = 1;
			// If waiting for the condition fails other than for a timeout, sleep instead
			else if (err != 0)
				usleep(MIO_SEND_REQUEST_TIMEOUT);
		}
		conn->send_request_predicate = 0;
		pthread_mutex_unlock(&conn->send_request_mutex);
	}
	mio_debug("Event loop completed");

	pthread_exit((void*) MIO_OK);
}

/**
 * @ingroup Core
 * Set up a connection to an XMPP server and start running the internal event loop.
 *
 * @param jid The full JID of the user connecting to the XMPP server.
 * @param pass The password of the user connecting to the XMPP server.
 * @param conn_handler A pointer to a user defined connection handler (optional). Pass NULL if no user defined connection handler should be used.
 * @param conn_handler_user_data A pointer to user defined data to be passed to the user defined connection handler (optional).
 * @param conn A pointer to an inactive mio conn struct.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_connect(char *jid, char *pass, mio_handler_conn conn_handler,
		void *conn_handler_user_data, mio_conn_t *conn) {

	int err = MIO_OK;
	mio_handler_data_t *shd;
	pthread_t mio_run_thread;
	mio_response_t *response;
	mio_response_error_t *response_err;
	struct timespec ts;
	struct timeval tp;
	pthread_attr_t attr;

	shd = _mio_handler_data_new();
	shd->conn = conn;
	shd->userdata = conn_handler_user_data;
	shd->conn_handler = conn_handler;

	sem_unlink(jid);
	conn->mio_open_requests = sem_open(jid, O_CREAT, S_IROTH | S_IWOTH,
			MIO_MAX_OPEN_REQUESTS);

// Initialize libstrophe
	xmpp_initialize();

// Setup authentication information
	xmpp_conn_set_jid(conn->xmpp_conn, jid);
	xmpp_conn_set_pass(conn->xmpp_conn, pass);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	err = pthread_create(&mio_run_thread, &attr, (void *) _mio_run, shd);
	pthread_attr_destroy(&attr);

	if (err == 0)
		mio_debug("mio_run thread successfully spawned.");
	else {
		mio_error("Error spawning mio_run thread, pthread error code %d", err);
		return MIO_ERROR_RUN_THREAD;
	}

// Set timeout
	gettimeofday(&tp, NULL );
	ts.tv_sec = tp.tv_sec;
	ts.tv_nsec = tp.tv_usec * 1000;
	ts.tv_sec += MIO_REQUEST_TIMEOUT_S;

	pthread_mutex_lock(&conn->conn_mutex);
// Wait for connection to be established before returning
	while (!conn->conn_predicate) {
		err = pthread_cond_timedwait(&conn->conn_cond, &conn->conn_mutex, &ts);
		if (err == ETIMEDOUT) {
			mio_error("Connection attempt timed out");
			pthread_mutex_unlock(&conn->conn_mutex);
			_mio_handler_data_free(shd);
			conn->conn_predicate = 0;
			return MIO_ERROR_TIMEOUT;
		} else if (err != 0) {
			mio_error("Conditional wait for connection attempt timed out");
			pthread_mutex_unlock(&conn->conn_mutex);
			_mio_handler_data_free(shd);
			conn->conn_predicate = 0;
			return MIO_ERROR_COND_WAIT;
		} else {
			// On success, do not free shd until disconnected
			response = shd->response;
			if (response->response_type == MIO_RESPONSE_OK) {
				mio_info("Connected to XMPP server %s using JID %s",
						conn->xmpp_conn->domain, conn->xmpp_conn->jid);
				err = MIO_OK;
			} else {
				response_err = response->response;
				err = response_err->err_num;
				mio_response_print(response);
			}
			pthread_mutex_unlock(&conn->conn_mutex);
			conn->conn_predicate = 0;
			mio_response_free(response);
			return err;
		}
	}
	_mio_handler_data_free(shd);
	return MIO_ERROR_PREDICATE;
}

/**
 * @ingroup Core
 * Disconnect from the XMPP server and stop running all mio threads.
 * @param conn A pointer to an active mio conn.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_disconnect(mio_conn_t *conn) {

	mio_listen_stop(conn);
	mio_pubsub_data_listen_stop(conn);
	mio_debug("Stopping libstrophe...");
// Free mio_handler_data in connection struct
//	if (conn->xmpp_conn->userdata != NULL )
//		_mio_handler_data_free(conn->xmpp_conn->userdata);
	xmpp_stop(conn->xmpp_conn->ctx);
	if (conn->xmpp_conn->authenticated) {
		mio_info("Disconnecting from XMPP server %s", conn->xmpp_conn->domain);
		xmpp_disconnect(conn->xmpp_conn);
		mio_info("Disconnected");
	}
	xmpp_shutdown();
	sem_close(conn->mio_open_requests);
	sem_unlink(conn->xmpp_conn->jid);
	pthread_mutex_unlock(&conn->event_loop_mutex);
	return MIO_OK;
}

/**
 * @ingroup Core
 * Add a handler to an active mio conn. The handler will be executed if a message matching the inputted namespace ns, type type or name name is received. Either a namespace, name or type must be passed as a parameter.
 * @param conn A pointer to an active mio conn.
 * @param handler A pointer to the mio handler to execute.
 * @param ns A string containing the XMPP namespace to listen for.
 * @param name A string containing the XMPP name to listen for.
 * @param type A string containing the XMPP type to listen for.
 * @param request A pointer to the mio request to be passed to the handler.
 * @param userdata A pointer to any user data, which should be passed to the handler.
 * @param response A pointer to an allocated mio response struct, which will be passed to the handler.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_handler_add(mio_conn_t * conn, mio_handler handler, const char *ns,
		const char *name, const char *type, mio_request_t *request,
		void *userdata, mio_response_t *response) {

	int err = MIO_OK;
	uuid_t uuid;
	mio_handler_data_t *handler_data = _mio_handler_data_new();

	if (ns == NULL && type == NULL && name == NULL ) {
		mio_error("Cannot add handler without namespace, name or type");
		return MIO_ERROR_HANDLER_ADD;
	}

	handler_data->conn = conn;
	handler_data->handler = handler;
	handler_data->userdata = userdata;
	handler_data->response = response;

// Copy handler trigger data into response
	if (response != NULL ) {
		if (ns != NULL ) {
			response->ns = malloc(strlen(ns) + 1);
			strcpy(response->ns, ns);
		}
		if (type != NULL ) {
			response->type = malloc(strlen(type) + 1);
			strcpy(response->type, type);
		}
		if (name != NULL ) {
			response->type = malloc(strlen(name) + 1);
			strcpy(response->name, name);
		}
	}

	if (request != NULL ) {
// Create id for request so we can hash it
		if (request->id == NULL ) {
			uuid_generate(uuid);
			uuid_unparse(uuid, request->id);
		}
		err = _mio_request_add(conn, request->id, handler, MIO_HANDLER,
				handler_data->response, request);
	}

	if (err == MIO_OK) {
		// Lock the event loop mutex so that we don't interleave addition/deletion of handlers
		pthread_mutex_lock(&conn->event_loop_mutex);
		xmpp_handler_add(conn->xmpp_conn, mio_handler_generic, ns, name, type,
				handler_data);
		pthread_mutex_unlock(&conn->event_loop_mutex);
	}

	else
		mio_error("Cannot add handler for id %s", request->id);

	return err;
}

/**
 * @ingroup Core
 * Add an ID handler to an active mio conn. The handler will be executed if a message matching the inputed XMPP message ID id, is received.
 * @param conn A pointer to an active mio conn.
 * @param handler A pointer to the mio handler to execute.
 * @param ns A string containing the XMPP mesage ID to listen for.
 * @param request A pointer to the mio request to be passed to the handler.
 * @param userdata A pointer to any user data, which should be passed to the handler.
 * @param response A pointer to an allocated mio response struct, which will be passed to the handler.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_handler_id_add(mio_conn_t * conn, mio_handler handler, const char *id,
		mio_request_t *request, void *userdata, mio_response_t *response) {

	int err = MIO_OK;
	mio_handler_data_t *handler_data;
	handler_data = _mio_handler_data_new();

	strcpy(request->id, id);

	if (request != NULL )
		err = _mio_request_add(conn, id, handler, MIO_HANDLER_ID, response,
				request);

	handler_data->conn = conn;
	handler_data->handler = handler;
	handler_data->userdata = userdata;
	handler_data->request = request;
	handler_data->response = response;

	strcpy(handler_data->request->id, id);
	strcpy(handler_data->response->id, id);

	if (err == MIO_OK) {
		// Lock the event loop mutex so that we don't interleve addition/deletion of handlers
		pthread_mutex_lock(&conn->event_loop_mutex);
		xmpp_id_handler_add(conn->xmpp_conn, mio_handler_generic_id, id,
				(void*) handler_data);
		pthread_mutex_unlock(&conn->event_loop_mutex);
	} else
		mio_error("Cannot add id handler for id %s", id);

	return err;
}

/**
 * @ingroup Core
 * Add a timed handler to an active mio conn. The handler will be executed periodically every period seconds.
 * @param conn A pointer to an active mio conn.
 * @param handler A pointer to the mio handler to execute.
 * @param period The period in seconds in which the handler should be executed.
 * @param userdata A pointer to any user data, which should be passed to the handler.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_handler_timed_add(mio_conn_t * conn, mio_handler handler, int period,
		void *userdata) {
	mio_handler_data_t *handler_data = _mio_handler_data_new();
	handler_data->handler = handler;
	handler_data->conn = conn;
	handler_data->userdata = userdata;
// Lock the event loop mutex so that we don't interleve addition/deletion of handlers
	pthread_mutex_lock(&conn->event_loop_mutex);
	xmpp_timed_handler_add(conn->xmpp_conn,
			(xmpp_timed_handler) mio_handler_generic_timed, period,
			handler_data);
	pthread_mutex_unlock(&conn->event_loop_mutex);
	return MIO_OK;
}

/**
 * @ingroup Core
 * Remove an existing mio handler by ID.
 *
 * @param conn A pointer to the active mio conn containing the handler.
 * @param handler A pointer to the mio handler to delete.
 * @param id A string containing the ID of the handler to delete.
 *
 * @returns MIO_OK on success, otherwise an error.
 */
void mio_handler_id_delete(mio_conn_t * conn, mio_handler handler,
		const char *id) {
// Lock the event loop mutex so that we don't interleve addition/deletion of handlers
	pthread_mutex_lock(&conn->event_loop_mutex);
	xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id, id);
	pthread_mutex_unlock(&conn->event_loop_mutex);
}

/**
 * @ingroup PubSub
 * Allocates and initializes a new mio stanza containing an item stanza.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @param type A string containing the item type to be created e.g. "data", "meta", etc.
 *
 * @returns MIO_OK on success, otherwise an error.
 */
mio_stanza_t *mio_pubsub_item_new(mio_conn_t *conn, char* item_type) {

	mio_stanza_t * item = mio_stanza_new(conn);

	xmpp_stanza_set_name(item->xmpp_stanza, "item");
	xmpp_stanza_set_id(item->xmpp_stanza, item_type);
	return item;
}

/**
 * @ingroup PubSub
 * Allocates and initializes a new mio stanza containing an item stanza of type data.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 *
 * @returns MIO_OK on success, otherwise an error.
 */
mio_stanza_t *mio_pubsub_item_data_new(mio_conn_t *conn) {

	xmpp_stanza_t *data = NULL;
	mio_stanza_t *item = mio_pubsub_item_new(conn, "data");
	data = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(data, "data");

	xmpp_stanza_add_child(item->xmpp_stanza, data);
	xmpp_stanza_release(data);

	return item;
}

/**
 * @ingroup PubSub
 * Allocates and initializes a new mio stanza containing an item stanza of type interface.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 *
 * @returns MIO_OK on success, otherwise an error.
 */
mio_stanza_t *mio_pubsub_item_interface_new(mio_conn_t *conn, char* interface) {

	xmpp_stanza_t *data = NULL;
	mio_stanza_t *item = mio_pubsub_item_new(conn, interface);
	data = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(data, "data");

	xmpp_stanza_add_child(item->xmpp_stanza, data);
	xmpp_stanza_release(data);

	return item;
}

/**
 * @ingroup PubSub
 *
 * Allocates and initializes a new mio packet.
 *
 * @returns The newly allocated and initialized mio packet.
 */
mio_packet_t *mio_packet_new() {
	mio_packet_t *packet = malloc(sizeof(mio_packet_t));
	packet->id = NULL;
	packet->payload = NULL;
	packet->type = MIO_PACKET_UNKNOWN;
	packet->num_payloads = 0;
	return packet;
}

/**
 * @ingroup PubSub
 * Frees an allocated mio packet.
 *
 * @param packet A pointer to the allocated mio packet to be freed.
 */
void mio_packet_free(mio_packet_t* packet) {
	mio_data_t *data;
	mio_subscription_t *sub, *curr_sub;
	mio_affiliation_t *aff, *curr_aff;
	mio_collection_t *coll, *curr_coll;
	mio_event_t *event, *curr_event;
	mio_reference_t *ref, *curr_ref;
	mio_meta_t *meta;

	if (packet->payload != NULL ) {
		switch (packet->type) {
		case MIO_PACKET_DATA:
			data = packet->payload;
			mio_data_free(data);
			break;

		case MIO_PACKET_SUBSCRIPTIONS:
			curr_sub = (mio_subscription_t*) packet->payload;
			while (curr_sub != NULL ) {
				sub = curr_sub->next;
				_mio_subscription_free(curr_sub);
				curr_sub = sub;
			}
			break;

		case MIO_PACKET_AFFILIATIONS:
			curr_aff = (mio_affiliation_t*) packet->payload;
			while (curr_aff != NULL ) {
				aff = curr_aff->next;
				_mio_affiliation_free(curr_aff);
				curr_aff = aff;
			}
			break;

		case MIO_PACKET_COLLECTIONS:
			curr_coll = (mio_collection_t*) packet->payload;
			while (curr_coll != NULL ) {
				coll = curr_coll->next;
				_mio_collection_free(curr_coll);
				curr_coll = coll;
			}
			break;
		case MIO_PACKET_META:
			meta = packet->payload;
			mio_meta_free(meta);
			break;

		case MIO_PACKET_NODE_TYPE:
			free(packet->payload);
			break;

		case MIO_PACKET_SCHEDULE:
			curr_event = (mio_event_t*) packet->payload;
			while (curr_event != NULL ) {
				event = curr_event->next;
				mio_event_free(curr_event);
				curr_event = event;
			}
			break;

		case MIO_PACKET_REFERENCES:
			curr_ref = (mio_reference_t*) packet->payload;
			while (curr_ref != NULL ) {
				ref = curr_ref->next;
				mio_reference_free(curr_ref);
				curr_ref = ref;
			}
			break;

		default:
			mio_error("Cannot free packet of unknown type");
			return;
			break;
		}
	}

	if (packet->id != NULL )
		free(packet->id);

	free(packet);
}

/**
 * @ingroup PubSub
 *
 * Allocates and initializes a new mio data struct.
 *
 * @returns The newly allocated and initialized mio data struct.
 */
mio_data_t *mio_data_new() {
	mio_data_t *data = malloc(sizeof(mio_data_t));
	data->event = NULL;
	data->num_transducers = 0;
	data->transducers = NULL;
	return data;
}

/**
 * @ingroup PubSub
 * Frees an allocated mio data struct.
 *
 * @param data A pointer to the allocated mio data struct to be freed.
 */
void mio_data_free(mio_data_t *data) {
	mio_transducer_value_t *t, *curr_t;
	curr_t = (mio_transducer_value_t*) data->transducers;
	while (curr_t != NULL ) {
		t = curr_t->next;
		mio_transducer_value_free(curr_t);
		curr_t = t;
	}
	if (data->event != NULL )
		free(data->event);
	free(data);
}

/**
 * @ingroup PubSub
 * Internal function to allocate and initialize a new mio subscription struct.
 *
 * @returns A pointer to a newly allocated and initialized mio subscription struct.
 */
mio_subscription_t *_mio_subscription_new() {
	mio_subscription_t *sub = malloc(sizeof(mio_subscription_t));
	sub->subscription = NULL;
	sub->sub_id = NULL;
	sub->next = NULL;
	return sub;
}

/**
 * @ingroup PubSub
 * Frees an allocated mio subscription struct.
 *
 * @param data A pointer to the allocated mio data struct to be freed.
 */
void _mio_subscription_free(mio_subscription_t *subscription) {
	free(subscription->sub_id);
	free(subscription->subscription);
	free(subscription);
}

/**
 * @ingroup PubSub
 * Returns the tail of a mio subscription linked list
 *
 * @param subscription A pointer to an element in a mio subscription linked list.
 * @returns A pointer to the tail of a mio subscription linked list.
 */
mio_subscription_t *_mio_subscription_tail_get(mio_subscription_t *subscription) {
	mio_subscription_t *tail = NULL;

	while (subscription != NULL ) {
		tail = subscription;
		subscription = subscription->next;
	}
	return tail;
}

/**
 * @ingroup PubSub
 * Create a mio subscription sctruct and add it to a mio packet.
 *
 * @param pkt A pointer the mio packet to which the subscription should be added to.
 * @param subscription A string containing the JID of the node that is subscribed.
 * @param sub_id A string containing the subscription ID of subscription.
 */
void _mio_subscription_add(mio_packet_t *pkt, char *subscription, char *sub_id) {

	mio_subscription_t *sub;
	sub = _mio_subscription_tail_get((mio_subscription_t*) pkt->payload);

	if (sub == NULL ) {
		sub = _mio_subscription_new();
		mio_packet_payload_add(pkt, sub, MIO_PACKET_SUBSCRIPTIONS);
	} else {
		sub->next = _mio_subscription_new();
		sub = sub->next;
	}

	sub->subscription = malloc((strlen(subscription) + 1) * sizeof(char));
	strcpy(sub->subscription, subscription);

// If no subscription ID is specified, fill it with a blank.
	if (sub_id != NULL ) {
		sub->sub_id = malloc((strlen(sub_id) + 1) * sizeof(char));
		strcpy(sub->sub_id, sub_id);
	} else {
		sub->sub_id = malloc(sizeof(char));
		strcpy(sub->sub_id, "");
	}

	pkt->num_payloads++;
}

/**
 * @ingroup PubSub
 * Internal function to allocate and initialize a new mio affiliation struct.
 *
 * @returns A pointer to a newly allocated and initialized mio affiliation struct.
 */
mio_affiliation_t *_mio_affiliation_new() {
	mio_affiliation_t *aff = malloc(sizeof(mio_affiliation_t));
	aff->affiliation = NULL;
	aff->type = MIO_AFFILIATION_NONE;
	aff->next = NULL;
	return aff;
}

/**
 * @ingroup PubSub
 * Frees an allocated mio affiliation struct.
 *
 * @param affiliation A pointer to the allocated mio affiliation struct to be freed.
 */
void _mio_affiliation_free(mio_affiliation_t *affiliation) {
	free(affiliation->affiliation);
	free(affiliation);
}

/**
 * @ingroup PubSub
 * Returns the tail of a mio affiliation linked list
 *
 * @param affiliation A pointer to an element in a mio affiliation linked list.
 * @returns A pointer to the tail of a mio affiliation linked list.
 */
mio_affiliation_t *_mio_affiliation_tail_get(mio_affiliation_t *affiliation) {
	mio_affiliation_t *tail = NULL;

	while (affiliation != NULL ) {
		tail = affiliation;
		affiliation = affiliation->next;
	}
	return tail;
}

/**
 * @ingroup PubSub
 * Adds a mio_subscription to a mio packet.
 *
 * @param pkt A pointer the mio packet to which the affiliation should be added to.
 * @param affiliation A string containing the JID of the node or user that is affiliated.
 * @param type The type of affiliation to be added.
 */
void _mio_affiliation_add(mio_packet_t *pkt, char *affiliation,
		mio_affiliation_type_t type) {
	mio_affiliation_t *aff;
	aff = _mio_affiliation_tail_get((mio_affiliation_t*) pkt->payload);
	if (aff == NULL ) {
		aff = _mio_affiliation_new();
		mio_packet_payload_add(pkt, aff, MIO_PACKET_AFFILIATIONS);
	} else {
		aff->next = _mio_affiliation_new();
		aff = aff->next;
	}

	aff->affiliation = malloc((strlen(affiliation) + 1) * sizeof(char));
	strcpy(aff->affiliation, affiliation);

	aff->type = type;

	pkt->num_payloads++;
}

/**
 * @ingroup Collections
 * Internal function to allocate and initialize a new mio collection struct.
 *
 * @returns A pointer to a newly allocated and initialized mio collection struct.
 */
mio_collection_t *_mio_collection_new() {
	mio_collection_t *coll = malloc(sizeof(mio_collection_t));
	memset(coll, 0, sizeof(mio_collection_t));
	return coll;
}

/**
 * @ingroup Collections
 * Internal function to free an allocated mio collection struct.
 *
 * @param collection A pointer to the allocated mio collection struct to be freed.
 */
void _mio_collection_free(mio_collection_t *collection) {
	if (collection->name != NULL )
		free(collection->name);
	if (collection->node != NULL )
		free(collection->node);
	free(collection);
}

/**
 * @ingroup Collections
 * Returns the tail of a mio affiliation linked list
 *
 * @param collection A pointer to an element in a mio collection linked list.
 * @returns A pointer to the tail of a mio collection linked list.
 */
mio_collection_t *_mio_collection_tail_get(mio_collection_t *collection) {
	mio_collection_t *tail = NULL;

	while (collection != NULL ) {
		tail = collection;
		collection = collection->next;
	}
	return tail;
}

/**
 * @ingroup Collections
 * Creates and adds a mio collection to a mio packet.
 *
 * @param pkt A pointer the mio packet to which the collection should be added to.
 * @param node A string containing the JID of the node that is part of the collection.
 * @param name A string containing the name of the node that is part of the collection.
 */
void _mio_collection_add(mio_packet_t *pkt, char *node, char *name) {
	mio_collection_t *coll;
	coll = _mio_collection_tail_get((mio_collection_t*) pkt->payload);

	if (coll == NULL ) {
		coll = _mio_collection_new();
		mio_packet_payload_add(pkt, coll, MIO_PACKET_COLLECTIONS);
	} else {
		coll->next = _mio_collection_new();
		coll = coll->next;
	}
	if (name != NULL ) {
		coll->name = malloc((strlen(name) + 1) * sizeof(char));
		strcpy(coll->name, name);
	}
	if (node != NULL ) {
		coll->node = malloc((strlen(node) + 1) * sizeof(char));
		strcpy(coll->node, node);
	}

	pkt->num_payloads++;
}

/**
 * @ingroup Scheduler
 * Allocate and initialize a new mio event struct.
 *
 * @returns A pointer to a newly allocated and initialized mio event struct.
 */
mio_event_t * mio_event_new() {
	mio_event_t *event = malloc(sizeof(mio_event_t));
	memset(event, 0, sizeof(mio_event_t));
	event->id = -1;
	return event;
}


/**
 * @ingroup Scheduler
 * Frees an allocated mio event struct.
 *
 * @param event A pointer to the allocated mio event struct to be freed.
 */
void mio_event_free(mio_event_t *event) {
	if (event->info != NULL )
		free(event->info);
	if (event->time != NULL )
		free(event->time);
	if (event->tranducer_name != NULL )
		free(event->tranducer_name);
	if (event->transducer_value != NULL )
		free(event->transducer_value);
	if (event->recurrence != NULL )
		mio_recurrence_free(event->recurrence);
	free(event);
}


/**
 * @ingroup Scheduler
 * Returns the tail of a mio event linked list
 *
 * @param event A pointer to an element in a mio event linked list.
 * @returns A pointer to the tail of a mio event linked list.
 */
mio_event_t *mio_event_tail_get(mio_event_t *event) {
	mio_event_t *tail = NULL;

	while (event != NULL ) {
		tail = event;
		event = event->next;
	}
	return tail;
}

/**
 * @ingroup Core
 * Prints the data contained in a mio geoloc struct to stdout.
 *
 * @param geoloc A pointer to the mio_geoloc struct to be printed.
 * @param tabs A string containing the formatting to be prefixed to each line being printed e.g. "\t\t"
 */
static void _mio_geoloc_print(mio_geoloc_t *geoloc, char* tabs) {
	fprintf(stdout, "%sGeoloc:\n", tabs);
	if (geoloc->accuracy != NULL )
		fprintf(stdout, "%s\tAccuracy: %lf\n", tabs, *geoloc->accuracy);
	if (geoloc->alt != NULL )
		fprintf(stdout, "%s\tAltitude: %lf\n", tabs, *geoloc->alt);
	if (geoloc->area != NULL )
		fprintf(stdout, "%s\tArea: %s\n", tabs, geoloc->area);
	if (geoloc->bearing != NULL )
		fprintf(stdout, "%s\tBearing: %lf\n", tabs, *geoloc->bearing);
	if (geoloc->building != NULL )
		fprintf(stdout, "%s\tBuilding: %s\n", tabs, geoloc->building);
	if (geoloc->country != NULL )
		fprintf(stdout, "%s\tCountry: %s\n", tabs, geoloc->country);
	if (geoloc->country_code != NULL )
		fprintf(stdout, "%s\tCountry Code: %s\n", tabs, geoloc->country_code);
	if (geoloc->datum != NULL )
		fprintf(stdout, "%s\tDatum: %s\n", tabs, geoloc->datum);
	if (geoloc->description != NULL )
		fprintf(stdout, "%s\tDescription: %s\n", tabs, geoloc->description);
	if (geoloc->floor != NULL )
		fprintf(stdout, "%s\tFloor: %s\n", tabs, geoloc->floor);
	if (geoloc->lat != NULL )
		fprintf(stdout, "%s\tLatitude: %lf\n", tabs, *geoloc->lat);
	if (geoloc->lon != NULL )
		fprintf(stdout, "%s\tLongitude: %lf\n", tabs, *geoloc->lon);
	if (geoloc->locality != NULL )
		fprintf(stdout, "%s\tLocality: %s\n", tabs, geoloc->locality);
	if (geoloc->postal_code != NULL )
		fprintf(stdout, "%s\tPostal Code: %s\n", tabs, geoloc->postal_code);
	if (geoloc->region != NULL )
		fprintf(stdout, "%s\tRegion: %s\n", tabs, geoloc->region);
	if (geoloc->room != NULL )
		fprintf(stdout, "%s\tRoom: %s\n", tabs, geoloc->room);
	if (geoloc->speed != NULL )
		fprintf(stdout, "%s\tSpeed: %lf\n", tabs, *geoloc->speed);
	if (geoloc->street != NULL )
		fprintf(stdout, "%s\tStreet: %s\n", tabs, geoloc->street);
	if (geoloc->text != NULL )
		fprintf(stdout, "%s\tText: %s\n", tabs, geoloc->text);
	if (geoloc->timestamp != NULL )
		fprintf(stdout, "%s\tTimestamp: %s\n", tabs, geoloc->timestamp);
	if (geoloc->uri != NULL )
		fprintf(stdout, "%s\tURI: %s\n", tabs, geoloc->uri);
}

/**
 * @ingroup Core
 * Prints the data contained in a mio response to stdout.
 *
 * @param response A pointer to the mio response to be printed.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_response_print(mio_response_t *response) {
	mio_packet_t *packet;
	mio_response_error_t *err;
	mio_data_t *data;
	mio_subscription_t *sub;
	mio_affiliation_t *aff;
	mio_collection_t *coll;
	mio_meta_t *meta;
	mio_transducer_meta_t *t_meta;
	mio_enum_map_meta_t *e_meta;
	mio_property_meta_t *p_meta;
	mio_transducer_value_t *t;
	mio_node_type_t *type;
	mio_event_t *event;
	mio_reference_t *ref;

	switch (response->response_type) {
	case MIO_RESPONSE_OK:
		fprintf(stdout, "\nRequest successful\n\n");
		break;
	case MIO_RESPONSE_ERROR:
		err = response->response;
		fprintf(stderr, "MIO Error:\n");
		fprintf(stderr, "\tError code: %d\n\tError description: %s\n",
				err->err_num, err->description);
		break;
	case MIO_RESPONSE_PACKET:
		packet = (mio_packet_t *) response->response;
		switch (packet->type) {
		case MIO_PACKET_DATA:
			data = (mio_data_t*) packet->payload;
			fprintf(stdout, "MIO Data Packet:\n\tEvent Node:%s\n", data->event);
			t = data->transducers;
			while (t != NULL ) {
				if (t->type == MIO_TRANSDUCER_VALUE)
					fprintf(stdout, "\t\tTrasducerValue:\n");
				else
					fprintf(stdout, "\t\tTrasducerSetValue:\n");
				fprintf(stdout,
						"\t\t\tName: %s\n\t\t\tID: %d\n\t\t\tRaw Value: %s\n\t\t\tTyped Value: %s\n\t\t\tTimestamp: %s\n",
						t->name, t->id, t->raw_value, t->typed_value,
						t->timestamp);
				t = t->next;
			}
			break;
		case MIO_PACKET_SUBSCRIPTIONS:
			sub = (mio_subscription_t*) packet->payload;
			fprintf(stdout, "MIO Subscriptions Packet:\n");
			if (sub == NULL ) {
				fprintf(stdout, "\t No subscriptions\n");
				break;
			}
			while (sub != NULL ) {
				fprintf(stdout, "\tSubscription: %s\n\t\tSub ID: %s\n",
						sub->subscription, sub->sub_id);
				sub = sub->next;
			}
			break;
		case MIO_PACKET_AFFILIATIONS:
			aff = (mio_affiliation_t*) packet->payload;
			fprintf(stdout, "MIO Affiliations Packet:\n");
			if (aff == NULL ) {
				fprintf(stdout, "\t No affiliations\n");
				break;
			}
			while (aff != NULL ) {
				fprintf(stdout, "\tAffiliation: %s\n", aff->affiliation);

				switch (aff->type) {
				case MIO_AFFILIATION_NONE:
					fprintf(stdout, "\t\tType: None\n");
					break;
				case MIO_AFFILIATION_OWNER:
					fprintf(stdout, "\t\tType: Owner\n");
					break;
				case MIO_AFFILIATION_MEMBER:
					fprintf(stdout, "\t\tType: Member\n");
					break;
				case MIO_AFFILIATION_PUBLISHER:
					fprintf(stdout, "\t\tType: Publisher\n");
					break;
				case MIO_AFFILIATION_PUBLISH_ONLY:
					fprintf(stdout, "\t\tType: Publish-only\n");
					break;
				case MIO_AFFILIATION_OUTCAST:
					fprintf(stdout, "\t\tType: Outcast\n");
					break;
				default:
					mio_error("Unknown or missing affiliation type");
					return MIO_ERROR_UNKNOWN_AFFILIATION_TYPE;
					break;
				}
				aff = aff->next;
			}
			break;
		case MIO_PACKET_COLLECTIONS:
			coll = (mio_collection_t*) packet->payload;
			fprintf(stdout, "MIO Collections Packet:\n");
			if (coll == NULL ) {
				fprintf(stdout, "\t No associated collection\n");
				break;
			}
			while (coll != NULL ) {
				fprintf(stdout, "\tName: %s\n", coll->name);
				fprintf(stdout, "\t\tNode: %s\n", coll->node);
				coll = coll->next;
			}
			break;
		case MIO_PACKET_META:
			meta = (mio_meta_t *) packet->payload;
			fprintf(stdout, "MIO Meta Packet:\n");
			switch (meta->meta_type) {
			case MIO_META_TYPE_DEVICE:
				fprintf(stdout, "\tDevice:\n");
				break;
			case MIO_META_TYPE_LOCATION:
				fprintf(stdout, "\tLocation:\n");
				break;
			default:
				fprintf(stdout, "\tNo associated meta data\n");
				break;
			}
			if (meta->name != NULL )
				fprintf(stdout, "\t\tName: %s\n", meta->name);
			if (meta->info != NULL )
				fprintf(stdout, "\t\tInfo: %s\n", meta->info);
			if (meta->timestamp != NULL )
				fprintf(stdout, "\t\tTimestamp: %s\n", meta->timestamp);
			if (meta->geoloc != NULL )
				_mio_geoloc_print(meta->geoloc, "\t\t");

			t_meta = meta->transducers;
			while (t_meta != NULL ) {
				fprintf(stdout, "\t\tTransducer:\n");
				if (t_meta->name != NULL )
					fprintf(stdout, "\t\t\tName: %s\n", t_meta->name);
				if (t_meta->type != NULL )
					fprintf(stdout, "\t\t\tType: %s\n", t_meta->type);
				if (t_meta->interface != NULL )
					fprintf(stdout, "\t\t\tInterface: %s\n", t_meta->interface);
				if (t_meta->unit != NULL )
					fprintf(stdout, "\t\t\tUnit: %s\n", t_meta->unit);
				if (t_meta->interface != NULL )
					fprintf(stdout, "\t\t\tInterface: %s\n", t_meta->interface);
				if (t_meta->enumeration != NULL ) {
					fprintf(stdout, "\t\t\tEnum Map:\n");
					for (e_meta = t_meta->enumeration; e_meta != NULL ; e_meta =
							e_meta->next) {
						fprintf(stdout, "\t\t\t\tName: %s\n", e_meta->name);
						fprintf(stdout, "\t\t\t\tValue: %s\n", e_meta->value);
					}
				}
				if (t_meta->properties != NULL ) {
					fprintf(stdout, "\t\t\tProperty:\n");
					for (p_meta = t_meta->properties; p_meta != NULL ; p_meta =
							p_meta->next) {
						fprintf(stdout, "\t\t\t\tName: %s\n", p_meta->name);
						fprintf(stdout, "\t\t\t\tValue: %s\n", p_meta->value);
					}
				}
				if (t_meta->manufacturer != NULL )
					fprintf(stdout, "\t\t\tManufacturer: %s\n",
							t_meta->manufacturer);
				if (t_meta->serial != NULL )
					fprintf(stdout, "\t\t\tSerial: %s\n", t_meta->serial);
				if (t_meta->resolution != NULL )
					fprintf(stdout, "\t\t\tResolution: %s\n",
							t_meta->resolution);
				if (t_meta->accuracy != NULL )
					fprintf(stdout, "\t\t\tAccuracy: %s\n", t_meta->accuracy);
				if (t_meta->precision != NULL )
					fprintf(stdout, "\t\t\tPrecision: %s\n", t_meta->precision);
				if (t_meta->min_value != NULL )
					fprintf(stdout, "\t\t\tMinimum Value: %s\n",
							t_meta->min_value);
				if (t_meta->max_value != NULL )
					fprintf(stdout, "\t\t\tMaximum Value: %s\n",
							t_meta->max_value);
				if (t_meta->info != NULL )
					fprintf(stdout, "\t\t\tInfo: %s\n", t_meta->info);
				if (t_meta->geoloc != NULL )
					_mio_geoloc_print(t_meta->geoloc, "\t\t\t");
				t_meta = t_meta->next;
			}
			p_meta = meta->properties;
			while (p_meta != NULL ) {
				fprintf(stdout, "\t\tProperty:\n");
				if (p_meta->name != NULL )
					fprintf(stdout, "\t\t\tName: %s\n", p_meta->name);
				if (p_meta->value != NULL )
					fprintf(stdout, "\t\t\tValue: %s\n", p_meta->value);
				p_meta = p_meta->next;
			}
			break;
		case MIO_PACKET_SCHEDULE:
			event = (mio_event_t*) packet->payload;
			fprintf(stdout, "MIO Schedule Packet:");
			if (event == NULL ) {
				fprintf(stdout, "\n\tNo associated schedule\n");
				break;
			}
			while (event != NULL ) {
				fprintf(stdout, "\n\tID: %u\n", event->id);
				if (event->time != NULL )
					fprintf(stdout, "\tTime: %s\n", event->time);
				if (event->tranducer_name != NULL )
					fprintf(stdout, "\tTransducer Name: %s\n",
							event->tranducer_name);
				if (event->transducer_value != NULL )
					fprintf(stdout, "\tTransducer Value: %s\n",
							event->transducer_value);
				if (event->info != NULL )
					fprintf(stdout, "\tInfo: %s\n", event->info);
				if (event->recurrence != NULL ) {
					fprintf(stdout, "\t\tRecurrence:\n");
					if (event->recurrence->freq != NULL )
						fprintf(stdout, "\t\t\tFreqency: %s\n",
								event->recurrence->freq);
					if (event->recurrence->interval != NULL )
						fprintf(stdout, "\t\t\tInterval: %s\n",
								event->recurrence->interval);
					if (event->recurrence->count != NULL )
						fprintf(stdout, "\t\t\tCount: %s\n",
								event->recurrence->count);
					if (event->recurrence->until != NULL )
						fprintf(stdout, "\t\t\tUntil: %s\n",
								event->recurrence->until);
					if (event->recurrence->bymonth != NULL )
						fprintf(stdout, "\t\t\tBy Month: %s\n",
								event->recurrence->bymonth);
					if (event->recurrence->byday != NULL )
						fprintf(stdout, "\t\t\tBy Day: %s\n",
								event->recurrence->byday);
					if (event->recurrence->exdate != NULL )
						fprintf(stdout, "\t\t\tEx Date: %s\n",
								event->recurrence->exdate);
				}
				event = event->next;
			}
			break;
		case MIO_PACKET_NODE_TYPE:
			type = (mio_node_type_t*) packet->payload;
			fprintf(stdout, "MIO Node Type Packet:\n");
			if (*type == MIO_NODE_TYPE_COLLECTION)
				fprintf(stdout, "\tNode Type: Collection\n");
			else if (*type == MIO_NODE_TYPE_LEAF)
				fprintf(stdout, "\tNode Type: Leaf\n");
			else
				fprintf(stdout, "\tNode Type: Unknown\n");
			break;
		case MIO_PACKET_REFERENCES:
			ref = (mio_reference_t*) packet->payload;
			fprintf(stdout, "MIO References Packet:\n");
			if (ref == NULL ) {
				fprintf(stdout, "\tNo associated references\n");
				break;
			}
			while (ref != NULL ) {
				fprintf(stdout, "\tNode: %s\n", ref->node);
				fprintf(stdout, "\tName: %s\n", ref->name);
				switch (ref->type) {
				case MIO_REFERENCE_CHILD:
					fprintf(stdout, "\tType: Child\n");
					break;
				case MIO_REFERENCE_PARENT:
					fprintf(stdout, "\tType: Parent\n");
					break;
				default:
					fprintf(stdout, "\n\tType: Unknown\n");
					break;
				}
				switch (ref->meta_type) {
				case MIO_META_TYPE_DEVICE:
					fprintf(stdout, "\tMeta Type: Device\n\n");
					break;
				case MIO_META_TYPE_LOCATION:
					fprintf(stdout, "\tMeta Type: Location\n\n");
					break;
				default:
					fprintf(stdout, "\tMeta Type: Unknown\n");
					break;
				}
				ref = ref->next;
			}
			break;

		default:
			mio_error("Unknown packet type");
			return MIO_ERROR_UNKNOWN_PACKET_TYPE;
			break;
		}
		break;
	default:
		mio_error("Unknown response type");
		return MIO_ERROR_UNKNOWN_RESPONSE_TYPE;
		break;
	}

	return MIO_OK;
}

/**
 * @ingroup Core
 * Adds a payload to a mio packet.
 *
 * @param packet The mio packet to which the payload should be added.
 * @param payload A pointer to the payload to be added to the mio packet.
 * @param type The mio packet type tof the payload being added.
 */
void mio_packet_payload_add(mio_packet_t *packet, void *payload,
		mio_packet_type_t type) {
	packet->type = type;
	packet->payload = (void*) payload;
}

/**
 * @ingroup Meta
 * Adds a mio transducer meta struct to a mio meta struct. If the mio meta struct already contains transducer meta structs, the mio transducer meta struct being added will be appended to the end of the linked list of mio transducer meta structs.
 *
 * @param meta A pointer to the mio meta struct to which the mio transducer meta struct should be added.
 * @param t_meta A pointer to the mio transducer meta struct to be added.
 */
void mio_transducer_meta_add(mio_meta_t *meta, mio_transducer_meta_t *t_meta) {
	mio_transducer_meta_t *curr = mio_transducer_meta_tail_get(
			meta->transducers);
	if (curr == NULL )
		meta->transducers = t_meta;
	else
		curr->next = t_meta;
}

/**
 * @ingroup Meta
 * Adds a mio enum map meta struct to a mio meta struct. If the mio meta struct already contains enum map meta structs, the mio enum map meta struct being added will be appended to the end of the linked list of mio enum map meta structs.
 *
 * @param meta A pointer to the mio meta struct to which the mio transducer meta struct should be added.
 * @param t_meta A pointer to the mio transducer meta struct to be added.
 */
void mio_enum_map_meta_add(mio_transducer_meta_t *t_meta,
		mio_enum_map_meta_t *e_meta) {
	mio_enum_map_meta_t *curr = mio_enum_map_meta_tail_get(t_meta->enumeration);
	if (curr == NULL )
		t_meta->enumeration = e_meta;
	else
		curr->next = e_meta;
}

/**
 * @ingroup Meta
 * Merge a mio meta struct with an existing one. Any members contained in meta will be used to overwrite the corresponding members in meta_to_update.
 *
 * @param meta_to_update A pointer to the mio meta struct to be updated
 * @param event A pointer to the mio meta struct containing the up to date data.
 */
void mio_meta_merge(mio_meta_t *meta_to_update, mio_meta_t *meta) {
	if (meta->meta_type != MIO_META_TYPE_UKNOWN)
		meta_to_update->meta_type = meta->meta_type;
	if (meta->name != NULL ) {
		if (meta_to_update->name != NULL )
			free(meta_to_update->name);
		meta_to_update->name = strdup(meta->name);
	}
	if (meta->info != NULL ) {
		if (meta_to_update->info != NULL )
			free(meta_to_update->info);
		meta_to_update->info = strdup(meta->info);
	}
	if (meta->geoloc != NULL ) {
		if (meta_to_update->geoloc == NULL )
			meta_to_update->geoloc = mio_geoloc_new();
		mio_geoloc_merge(meta_to_update->geoloc, meta->geoloc);
	}
	if (meta->properties != NULL ) {
		if (meta_to_update->properties == NULL )
			meta_to_update->properties = mio_property_meta_new();
		mio_meta_property_merge(meta_to_update->properties, meta->properties);
	}
	if (meta_to_update->timestamp != NULL )
		free(meta_to_update->timestamp);
	meta_to_update->timestamp = strdup(meta->timestamp);
}

/**
 * @ingroup Meta
 * Merge a mio transducer meta struct with an existing one. Any members contained in transducer will be used to overwrite the corresponding members in transducer_to_update. Any mio properties contained in the mio transducer meta structs will be merged by name.
 *
 * @param transducer_to_update A pointer to the mio transducer meta struct to be updated
 * @param transdcuer A pointer to the mio transducer meta struct containing the up to date data.
 */
void mio_meta_transducer_merge(mio_transducer_meta_t *transducer_to_update,
		mio_transducer_meta_t *transducer) {
	mio_property_meta_t *property, *property_to_update;
	int updated_flag = 0;

	if (transducer->accuracy != NULL ) {
		if (transducer_to_update->accuracy != NULL )
			free(transducer_to_update->accuracy);
		transducer_to_update->accuracy = strdup(transducer->accuracy);
	}
	if (transducer->geoloc != NULL ) {
		if (transducer_to_update->geoloc == NULL )
			transducer_to_update->geoloc = mio_geoloc_new();
		mio_geoloc_merge(transducer_to_update->geoloc, transducer->geoloc);
	}
	if (transducer->interface != NULL ) {
		if (transducer_to_update->interface != NULL )
			free(transducer_to_update->interface);
		transducer_to_update->interface = strdup(transducer->interface);
	}
	if (transducer->manufacturer != NULL ) {
		if (transducer_to_update->manufacturer != NULL )
			free(transducer_to_update->manufacturer);
		transducer_to_update->manufacturer = strdup(transducer->manufacturer);
	}
	if (transducer->max_value != NULL ) {
		if (transducer_to_update->max_value != NULL )
			free(transducer_to_update->max_value);
		transducer_to_update->max_value = strdup(transducer->max_value);
	}
	if (transducer->min_value != NULL ) {
		if (transducer_to_update->min_value != NULL )
			free(transducer_to_update->min_value);
		transducer_to_update->min_value = strdup(transducer->min_value);
	}
	if (transducer->precision != NULL ) {
		if (transducer_to_update->precision != NULL )
			free(transducer_to_update->precision);
		transducer_to_update->precision = strdup(transducer->precision);
	}
	if (transducer->resolution != NULL ) {
		if (transducer_to_update->resolution != NULL )
			free(transducer_to_update->resolution);
		transducer_to_update->resolution = strdup(transducer->resolution);
	}
	if (transducer->serial != NULL ) {
		if (transducer_to_update->serial != NULL )
			free(transducer_to_update->serial);
		transducer_to_update->serial = strdup(transducer->serial);
	}
	if (transducer->type != NULL ) {
		if (transducer_to_update->type != NULL )
			free(transducer_to_update->type);
		transducer_to_update->type = strdup(transducer->type);
	}
	if (transducer->unit != NULL ) {
		if (transducer_to_update->unit != NULL )
			free(transducer_to_update->unit);
		transducer_to_update->unit = strdup(transducer->unit);
	}
	if (transducer->name != NULL ) {
		if (transducer_to_update->name != NULL )
			free(transducer_to_update->name);
		transducer_to_update->name = strdup(transducer->name);
	}
	if (transducer->info != NULL ) {
		if (transducer_to_update->info != NULL )
			free(transducer_to_update->info);
		transducer_to_update->info = strdup(transducer->info);
	}
	if (transducer->accuracy != NULL ) {
		if (transducer_to_update->accuracy != NULL )
			free(transducer_to_update->accuracy);
		transducer_to_update->accuracy = strdup(transducer->accuracy);
	}
//Overwrite (don't merge) enumeration map if it exists
	if (transducer->enumeration != NULL ) {
		if (transducer_to_update->enumeration != NULL )
			mio_enum_map_meta_free(transducer_to_update->enumeration);
		transducer_to_update->enumeration = mio_enum_map_meta_clone(
				transducer->enumeration);
	}
	// Merge properties of transducers
	if (transducer->properties != NULL ) {
		for (property = transducer->properties; property != NULL ; property =
				property->next) {
			for (property_to_update = transducer_to_update->properties;
					property_to_update != NULL ; property_to_update =
							property_to_update->next) {
				if (strcmp(property_to_update->name, property->name) == 0) {
					mio_meta_property_merge(property_to_update, property);
					updated_flag = 1;
				}
			}
			// If we didn't find a property with the same name to update, add it to the end of the linked list of properties
			if (!updated_flag) {
				property_to_update = mio_property_meta_tail_get(
						transducer_to_update->properties);
				if (property_to_update == NULL ) {
					transducer_to_update->properties = mio_property_meta_new();
					property_to_update = transducer_to_update->properties;
				} else {
					property_to_update->next = mio_property_meta_new();
					property_to_update = property_to_update->next;
				}
				mio_meta_property_merge(property_to_update, property);
			} else
				updated_flag = 0;
		}

	}
}

/**
 * @ingroup Meta
 * Merge a mio property meta struct with an existing one.
 *
 * @param property_to_update A pointer to the mio property meta struct to be updated
 * @param property A pointer to the mio property meta struct containing the up to date data.
 */
void mio_meta_property_merge(mio_property_meta_t *property_to_update,
		mio_property_meta_t *property) {
	if (property->name != NULL ) {
		if (property_to_update->name != NULL )
			free(property_to_update->name);
		property_to_update->name = strdup(property->name);
	}
	if (property->value != NULL ) {
		if (property_to_update->value != NULL )
			free(property_to_update->value);
		property_to_update->value = strdup(property->value);
	}
}

/**
 * @ingroup Meta
 * Overwrite an existing reference at a node.
 *
 * @param conn A pointer to an active mio connection.
 * @param node A string containing the JID of the node to update.
 * @param ref_node A string containing the JID of the node contained in the reference to overwrite.
 * @param ref_type The new mio meta type of the node contained within the reference to update.
 * @param response  A pointer to an allocated mio response struct which will be populated with the server's response.
 * @returns MIO_OK on success, otherwise an error.
 */
static int _mio_reference_meta_type_overwrite_publish(mio_conn_t *conn,
		char *node, char *ref_node, mio_reference_type_t ref_type,
		mio_meta_type_t ref_meta_type, mio_response_t *response) {
	mio_response_t *query_response = mio_response_new();
	mio_packet_t *packet;
	mio_reference_t *query_ref;
	mio_stanza_t *item;

	// Query node for its references
	mio_references_query(conn, node, query_response);
	if (query_response->response_type != MIO_RESPONSE_PACKET) {
		mio_response_free(query_response);
		return MIO_ERROR_UNEXPECTED_RESPONSE;
	}
	packet = (mio_packet_t*) query_response->response;
	if (packet->type != MIO_PACKET_REFERENCES) {
		mio_response_free(query_response);
		return MIO_ERROR_UNEXPECTED_PAYLOAD;
	}
	// If query was successful, remove reference that we want to overwrite
	query_ref = (mio_reference_t*) packet->payload;
	query_ref = mio_reference_remove(conn, query_ref, ref_type, ref_node);
	// If reference to remove was not present, don't publish anything
	if (query_ref != NULL ) {
		// Add new reference to list
		mio_reference_add(conn, query_ref, ref_type, ref_meta_type, ref_node);
		// Publish modified list of references
		item = mio_references_to_item(conn, query_ref);
		mio_item_publish(conn, item, node, response);
	}
// Cleanup
	mio_response_free(query_response);
	//mio_stanza_free(item);
	return MIO_OK;
}

/**
 * @ingroup Meta
 * Merge the meta data currently contained in a node with data from mio meta, mio transducer meta and/or mio property meta structs.
 *
 * @param conn A pointer to an active mio connection.
 * @param node A string containing the JID of the node to update.
 * @param meta A pointer to the mio meta struct with new data to be merged.
 * @param transducers A pointer to the mio transducer meta struct with new data to be merged.
 * @param properties A pointer to the mio property meta with new data to be merged.
 * @param response  A pointer to an allocated mio response struct which will be populated with the server's response.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_meta_merge_publish(mio_conn_t *conn, char *node, mio_meta_t *meta,
		mio_transducer_meta_t *transducers, mio_property_meta_t *properties,
		mio_response_t *response) {
	mio_meta_t *query_meta;
	mio_packet_t *packet;
	mio_transducer_meta_t *transducer, *query_transducer;
	mio_property_meta_t *property, *query_property;
	mio_stanza_t *item;
	int err;
	mio_response_t *query_response = mio_response_new();
	mio_response_t *query_refs, *pub_refs;
	mio_reference_t *ref, *curr_ref;

// Get the current meta information from the node
	mio_meta_query(conn, node, query_response);

	if (query_response->response_type != MIO_RESPONSE_PACKET)
		return MIO_ERROR_UNEXPECTED_RESPONSE;
	packet = (mio_packet_t*) query_response->response;
	if (packet->type != MIO_PACKET_META)
		return MIO_ERROR_UNEXPECTED_RESPONSE;
	query_meta = (mio_meta_t*) packet->payload;

// If we are given a meta struct, overwrite the one inside the node
	if (meta != NULL ) {
		if (query_meta == NULL )
			query_meta = mio_meta_new();

		// If the new meta type is not the same as the current one, we have to update the references of those nodes
		if (meta->meta_type != query_meta->meta_type) {
			query_refs = mio_response_new();
			mio_references_query(conn, node, query_refs);
			if (query_refs->response_type == MIO_RESPONSE_PACKET) {
				packet = (mio_packet_t*) query_refs->response;
				if (packet->type == MIO_PACKET_REFERENCES) {
					ref = (mio_reference_t*) packet->payload;
					// Loop over all references of node, updating each reference meta type
					for (curr_ref = ref; curr_ref != NULL ;
							curr_ref = curr_ref->next) {
						pub_refs = mio_response_new();
						if (curr_ref->type == MIO_REFERENCE_CHILD)
							err = _mio_reference_meta_type_overwrite_publish(
									conn, curr_ref->node, node,
									MIO_REFERENCE_PARENT, meta->meta_type,
									pub_refs);
						else if (curr_ref->type == MIO_REFERENCE_PARENT)
							err = _mio_reference_meta_type_overwrite_publish(
									conn, curr_ref->node, node,
									MIO_REFERENCE_CHILD, meta->meta_type,
									pub_refs);
						if (err != MIO_OK
								&& pub_refs->response_type != MIO_RESPONSE_OK)
							mio_error(
									"Unable to update meta_type reference of referenced node %s\n",
									curr_ref->node);
						mio_response_free(pub_refs);
					}
				} else
					mio_warn(
							"Query references of node %s failed, check permissions",
							node);
			} else
				mio_warn(
						"Query references of node %s failed, check if node exists permissions",
						node);
			mio_response_free(query_refs);
		}
		mio_meta_merge(query_meta, meta);
	}

//If we are given transducer(s), add them to the node and update attributes of already existing ones
	transducer = transducers;
	if (query_meta != NULL ) {
		if (transducer != NULL && query_meta->transducers == NULL )
			query_meta->transducers = mio_transducer_meta_new();
		while (transducer != NULL ) {
			query_transducer = query_meta->transducers;
			// See if the node already includes a transducer we are trying to add and update
			while (query_transducer != NULL ) {
				if (query_transducer->name != NULL ) {
					if (strcmp(query_transducer->name, transducer->name) == 0)
						break;
				}
				query_transducer = query_transducer->next;
			}
			// If a transducer with the same name exists, update its attributes, otherwise add it to the device
			if (query_transducer == NULL ) {
				query_transducer = mio_transducer_meta_tail_get(
						query_meta->transducers);
				if (query_transducer->name != NULL ) {
					query_transducer->next = mio_transducer_meta_new();
					query_transducer = query_transducer->next;
				}
			}
			mio_meta_transducer_merge(query_transducer, transducer);
			transducer = transducer->next;
		}
	}

//If we are given properties(s), add them to the node and update attributes of already existing ones
	property = properties;
	if (query_meta != NULL ) {
		if (property != NULL && query_meta->properties == NULL )
			query_meta->properties = mio_property_meta_new();
		while (property != NULL ) {
			query_property = query_meta->properties;
			// See if the node already includes a transducer we are trying to add and update
			while (query_property != NULL ) {
				if (query_property->name != NULL ) {
					if (strcmp(query_property->name, property->name) == 0)
						break;
				}
				query_property = query_property->next;
			}
			// If a property with the same name exists, update its attributes, otherwise add it to the device
			if (query_property == NULL ) {
				query_property = mio_property_meta_tail_get(
						query_meta->properties);
				if (query_property->name != NULL ) {
					query_property->next = mio_property_meta_new();
					query_property = query_property->next;
				}
			}
			mio_meta_property_merge(query_property, property);
			property = property->next;
		}
	}
// Convert updated query_meta struct to a stanza and publish it
	item = mio_meta_to_item(conn, query_meta);
	err = mio_item_publish(conn, item, node, response);

// Free structs
	mio_response_free(query_response);

	return err;
}

mio_transducer_meta_t *mio_meta_transducer_remove(
		mio_transducer_meta_t *transducers, char *transducer_name) {
	mio_transducer_meta_t *prev = NULL, *curr;

	curr = transducers;
	while (curr != NULL ) {
		if (strcmp(curr->name, transducer_name) == 0) {
			if (prev == NULL ) {
				curr = curr->next;
				transducers->next = NULL;
				mio_transducer_meta_free(transducers);
				return curr;
			}
			prev->next = curr->next;
			curr->next = NULL;
			mio_transducer_meta_free(curr);
			return transducers;
		}
		prev = curr;
		curr = curr->next;
	}
	return transducers;
}

mio_property_meta_t *mio_meta_property_remove(mio_property_meta_t *properties,
		char *property_name) {
	mio_property_meta_t *prev = NULL, *curr;

	curr = properties;
	while (curr != NULL ) {
		if (strcmp(curr->name, property_name) == 0) {
			if (prev == NULL ) {
				curr = curr->next;
				properties->next = NULL;
				mio_property_meta_free(properties);
				return curr;
			}
			prev->next = curr->next;
			curr->next = NULL;
			mio_property_meta_free(curr);
			return properties;
		}
		prev = curr;
		curr = curr->next;
	}
	return properties;
}

void mio_meta_geoloc_remove(mio_meta_t *meta,
		mio_transducer_meta_t *transducers, char *transducer_name) {
	mio_transducer_meta_t *curr;

	if (meta != NULL ) {
		mio_geoloc_free(meta->geoloc);
		meta->geoloc = NULL;
	}
	if (transducers != NULL && transducer_name != NULL ) {
		curr = transducers;
		while (curr != NULL ) {
			if (strcmp(curr->name, transducer_name) == 0) {
				mio_geoloc_free(curr->geoloc);
				curr->geoloc = NULL;
				break;
			}
			curr = curr->next;
		}
	}
}

int mio_meta_geoloc_remove_publish(mio_conn_t *conn, char *node,
		char *transducer_name, mio_response_t *response) {
	mio_packet_t *query_packet;
	mio_meta_t *query_meta;
	mio_response_t *query_response = mio_response_new();
	mio_stanza_t *item;
	int err;

	mio_meta_query(conn, node, query_response);
	if (query_response->response_type != MIO_RESPONSE_PACKET)
		return MIO_ERROR_UNEXPECTED_RESPONSE;

	query_packet = (mio_packet_t*) query_response->response;
	if (query_packet->type != MIO_PACKET_META)
		return MIO_ERROR_UNEXPECTED_PAYLOAD;

	query_meta = (mio_meta_t*) query_packet->payload;

// If no transducer is specified, remove the geoloc from the device registered at the node
	if (transducer_name == NULL )
		mio_meta_geoloc_remove(query_meta, NULL, NULL );
// Otherwise remove the geoloc from the specified transducer
	else
		mio_meta_geoloc_remove(NULL, query_meta->transducers, transducer_name);

//Publish the updated meta information
	item = mio_meta_to_item(conn, query_meta);
	err = mio_item_publish(conn, item, node, response);

//Cleanup
	mio_response_free(query_response);

	return err;
}

int mio_meta_remove_publish(mio_conn_t *conn, char *node, char *meta_name,
		char **transducer_names, int num_transducer_names,
		char **property_names, int num_property_names, mio_response_t *response) {
	mio_packet_t *query_packet;
	mio_meta_t *query_meta;
	mio_response_t *query_response = mio_response_new();
	mio_response_t *query_refs, *pub_refs;
	mio_packet_t *packet;
	mio_reference_t *ref, *curr_ref;
	mio_stanza_t *item, *iq;
	xmpp_stanza_t *remove_item = NULL, *retract = NULL;
	int err, i;

	mio_meta_query(conn, node, query_response);
	if (query_response->response_type != MIO_RESPONSE_PACKET)
		return MIO_ERROR_UNEXPECTED_RESPONSE;

	query_packet = (mio_packet_t*) query_response->response;
	if (query_packet->type != MIO_PACKET_META)
		return MIO_ERROR_UNEXPECTED_PAYLOAD;

	query_meta = (mio_meta_t*) query_packet->payload;

// If we want to remove the entire device, remove the entire meta item
	if (meta_name != NULL ) {
		iq = _mio_pubsub_set_stanza_new(conn, node);
		retract = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(retract, "retract");
		xmpp_stanza_set_attribute(retract, "node", node);
		remove_item = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(remove_item, "item");
		xmpp_stanza_set_attribute(remove_item, "id", "meta");
		xmpp_stanza_add_child(retract, remove_item);
		xmpp_stanza_add_child(iq->xmpp_stanza->children, retract);
		err = _mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
				response);
		mio_response_free(query_response);
		mio_stanza_free(iq);

		// Update the references of referenced nodes to meta type unknown
		query_refs = mio_response_new();
		mio_references_query(conn, node, query_refs);
		if (query_refs->response_type == MIO_RESPONSE_PACKET) {
			packet = (mio_packet_t*) query_refs->response;
			if (packet->type == MIO_PACKET_REFERENCES) {
				ref = (mio_reference_t*) packet->payload;
				// Loop over all references of node, updating each reference meta type
				for (curr_ref = ref; curr_ref != NULL ;
						curr_ref = curr_ref->next) {
					pub_refs = mio_response_new();
					if (curr_ref->type == MIO_REFERENCE_CHILD)
						err = _mio_reference_meta_type_overwrite_publish(conn,
								curr_ref->node, node, MIO_REFERENCE_PARENT,
								MIO_META_TYPE_UKNOWN, pub_refs);
					else if (curr_ref->type == MIO_REFERENCE_PARENT)
						err = _mio_reference_meta_type_overwrite_publish(conn,
								curr_ref->node, node, MIO_REFERENCE_CHILD,
								MIO_META_TYPE_UKNOWN, pub_refs);
					if (err != MIO_OK
							&& pub_refs->response_type != MIO_RESPONSE_OK)
						mio_error(
								"Unable to update meta_type reference of referenced node %s\n",
								curr_ref->node);
					mio_response_free(pub_refs);
				}
			} else
				mio_warn(
						"Query references of node %s failed, check permissions",
						node);
		} else
			mio_warn(
					"Query references of node %s failed, check if node exists permissions",
					node);
		mio_response_free(query_refs);
		return err;
	}

// Remove transducers
	if (transducer_names != NULL ) {
		for (i = 0; i < num_transducer_names; i++)
			query_meta->transducers = mio_meta_transducer_remove(
					query_meta->transducers, transducer_names[i]);
	}
// Remove properties
	if (property_names != NULL ) {
		for (i = 0; i < num_property_names; i++)
			query_meta->properties = mio_meta_property_remove(
					query_meta->properties, property_names[i]);
	}
// Convert meta data struct to stanza and publish
	item = mio_meta_to_item(conn, query_meta);
	err = mio_item_publish(conn, item, node, response);
// Cleanup
	mio_stanza_free(item);
	mio_response_free(query_response);
	return err;
}

/** Adds transducer value to data item. Can specify the value type
 *       of the data.
 *
 * @param d_meta
 * @param t_meta
 *  */
int mio_data_transducer_value_add(mio_data_t *data,
		mio_transducer_value_type_t type, int id, char *name, char *typed_value,
		char* raw_value, char *timestamp) {

	mio_transducer_value_t *t;

	if (data->transducers == NULL ) {
		data->transducers = mio_transducer_value_new();
		t = data->transducers;
	} else {
		t = _mio_transducer_value_tail_get(data->transducers);
		t->next = mio_transducer_value_new();
		t = t->next;
	}

	t->id = id;
	t->type = type;
	if (name != NULL ) {
		t->name = malloc(strlen(name) + 1);
		strcpy(t->name, name);
	}
	if (typed_value != NULL ) {
		t->typed_value = malloc(strlen(typed_value) + 1);
		strcpy(t->typed_value, typed_value);
	}
	if (raw_value != NULL ) {
		t->raw_value = malloc(strlen(raw_value) + 1);
		strcpy(t->raw_value, raw_value);
	}
	if (timestamp != NULL ) {
		t->timestamp = malloc(strlen(timestamp) + 1);
		strcpy(t->timestamp, timestamp);
	}

	data->num_transducers++;
	return MIO_OK;
}

static mio_stanza_t *_mio_pubsub_iq_get_stanza_new(mio_conn_t *conn,
		const char *node) {
	mio_stanza_t *iq = _mio_pubsub_iq_stanza_new(conn, node);
	xmpp_stanza_set_type(iq->xmpp_stanza, "get");
	return iq;
}
/*static mio_stanza_t *_mio_pubsub_iq_set_stanza_new(mio_conn_t *conn, const char *node) {
 mio_stanza_t *iq = _mio_pubsub_iq_stanza_new(conn, node);
 xmpp_stanza_set_type(iq->xmpp_stanza, "set");
 return iq;
 }*/

static mio_stanza_t *_mio_pubsub_iq_stanza_new(mio_conn_t *conn,
		const char *node) {
	char *pubsub_server, *pubsub_server_target = NULL;
	uuid_t uuid;
	mio_stanza_t *iq = mio_stanza_new(conn);

// Try to get server address from node, otherwise from JID
	if (node != NULL )
		pubsub_server_target = _mio_get_server(node);
	if (pubsub_server_target == NULL )
		pubsub_server_target = _mio_get_server(conn->xmpp_conn->jid);

	pubsub_server = malloc(sizeof(char) * (strlen(pubsub_server_target) + 8));

	sprintf(pubsub_server, "pubsub.%s", pubsub_server_target);

// Create id for iq stanza using new UUID
	uuid_generate(uuid);
	uuid_unparse(uuid, iq->id);

	xmpp_stanza_set_name(iq->xmpp_stanza, "iq");
	xmpp_stanza_set_attribute(iq->xmpp_stanza, "to", pubsub_server);
	xmpp_stanza_set_attribute(iq->xmpp_stanza, "id", iq->id);
	xmpp_stanza_set_attribute(iq->xmpp_stanza, "from", conn->xmpp_conn->jid);

	return iq;
}

static mio_stanza_t *_mio_pubsub_set_stanza_new(mio_conn_t *conn,
		const char *node) {
	mio_stanza_t *iq = _mio_pubsub_stanza_new(conn, node);
	xmpp_stanza_set_type(iq->xmpp_stanza, "set");
	return iq;
}

static mio_stanza_t *_mio_pubsub_get_stanza_new(mio_conn_t *conn,
		const char *node) {
	mio_stanza_t *iq = _mio_pubsub_stanza_new(conn, node);
	xmpp_stanza_set_type(iq->xmpp_stanza, "get");
	return iq;
}

static mio_stanza_t *_mio_pubsub_stanza_new(mio_conn_t *conn, const char *node) {
	xmpp_stanza_t *pubsub = NULL;
	uuid_t uuid;
	mio_stanza_t *iq = mio_stanza_new(conn);
	pubsub = xmpp_stanza_new(conn->xmpp_conn->ctx);
	char *pubsub_server, *pubsub_server_target = NULL;

// Try to get server address from node, otherwise from JID
	if (node != NULL )
		pubsub_server_target = _mio_get_server(node);
	if (pubsub_server_target == NULL )
		pubsub_server_target = _mio_get_server(conn->xmpp_conn->jid);

	pubsub_server = malloc(sizeof(char) * (strlen(pubsub_server_target) + 8));

	sprintf(pubsub_server, "pubsub.%s", pubsub_server_target);

// Create id for iq stanza using new UUID
	uuid_generate(uuid);
	uuid_unparse(uuid, iq->id);

// Create a new iq stanza
	xmpp_stanza_set_name(iq->xmpp_stanza, "iq");
	xmpp_stanza_set_attribute(iq->xmpp_stanza, "to", pubsub_server);
	xmpp_stanza_set_attribute(iq->xmpp_stanza, "id", iq->id);

	xmpp_stanza_set_name(pubsub, "pubsub");
	xmpp_stanza_set_ns(pubsub, "http://jabber.org/protocol/pubsub");

	xmpp_stanza_add_child(iq->xmpp_stanza, pubsub);

	xmpp_stanza_release(pubsub);
	free(pubsub_server);

	return iq;
}

mio_stanza_t *mio_pubsub_set_stanza_new(mio_conn_t *conn, const char *node) {
	mio_stanza_t *stanza = _mio_pubsub_stanza_new(conn, node);
	xmpp_stanza_set_type(stanza->xmpp_stanza, "set");
	return stanza;
}

mio_stanza_t *mio_pubsub_get_stanza_new(mio_conn_t *conn, const char *node) {
	mio_stanza_t *stanza = mio_pubsub_set_stanza_new(conn, node);
	xmpp_stanza_set_type(stanza->xmpp_stanza, "get");
	return stanza;
}

int mio_subscribe(mio_conn_t * conn, const char *node,
		mio_response_t * response) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *subscribe = NULL;
	int err;
	mio_packet_t *pkt;
	mio_subscription_t *sub;
	mio_response_t *subscriptions = mio_response_new();

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process subscribe request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	err = mio_subscriptions_query(conn, NULL, subscriptions);
	if (subscriptions->response_type != MIO_RESPONSE_ERROR) {
		pkt = (mio_packet_t*) subscriptions->response;
		sub = (mio_subscription_t*) pkt->payload;
		while (sub != NULL ) {
			if (strcmp(sub->subscription, node) == 0) {
				mio_error(
						"Already subscribed to node %s, aborting subscription request",
						node);
				mio_response_free(subscriptions);
				return MIO_ERROR_ALREADY_SUBSCRIBED;
			} else
				sub = sub->next;
		}
	} else if (subscriptions->response_type == MIO_RESPONSE_ERROR) {
		mio_response_print(subscriptions);
		mio_response_free(subscriptions);
		return err;
	} else {
		mio_response_free(subscriptions);
		return err;
	}

	iq = mio_pubsub_set_stanza_new(conn, node);

// Create a new subscribe stanza
	subscribe = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(subscribe, "subscribe");
	xmpp_stanza_set_attribute(subscribe, "node", node);
	xmpp_stanza_set_attribute(subscribe, "jid", conn->xmpp_conn->jid);

// Build xmpp message
	xmpp_stanza_add_child(iq->xmpp_stanza->children, subscribe);

// Send out the stanza
	err = _mio_send_blocking(conn, iq, (mio_handler) mio_handler_subscribe,
			response);

// Release the stanzas
	xmpp_stanza_release(subscribe);
	mio_stanza_free(iq);

	mio_response_free(subscriptions);
	return err;
}

/*
 int sox_acl_node_config(sox_conn_t *conn, const char *node, const char* title,
 const char* access, sox_userdata_t* const userdata) {
 return sox_acl_node_process(conn, node, title, access, userdata, 0);
 }

 int sox_acl_publisher_add(sox_conn_t *conn, const char *node,
 const char *publisher, sox_userdata_t * const userdata) {
 return sox_acl_publisher_process(conn, node, publisher, 1, userdata);
 }

 int sox_acl_publisher_remove(sox_conn_t *conn, const char *node,
 const char *publisher, sox_userdata_t * const userdata) {
 return sox_acl_publisher_process(conn, node, publisher, 0, userdata);
 }

 int sox_acl_publisher_process(sox_conn_t *conn, const char *node,
 const char *publisher, int add_remove, sox_userdata_t * const userdata) {

 xmpp_stanza_t *iq = NULL;
 xmpp_stanza_t *pubsub = NULL;
 xmpp_stanza_t *affiliations = NULL;
 xmpp_stanza_t *affiliation = NULL;

 char *pubsubserv;
 char *id;
 int err;

 // String length is length of server address + length of "pubsub." + terminating character
 pubsubserv = (char *) malloc(
 (strlen(sox_get_server(conn->xmpp_conn->jid)) + 8) * sizeof(char));
 if (pubsubserv == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }
 strcpy(pubsubserv, "pubsub.");
 strcat(pubsubserv, sox_get_server(conn->xmpp_conn->jid));

 // Create id for iq stanza consisting of "addpub_" and node name
 // String length is length of server address + length of "addpub_" + terminating character
 id = (char *) malloc((strlen(node) + 8) * sizeof(char));
 if (id == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }
 if (add_remove) {
 strcpy(id, "addpub_");
 strcat(id, node);
 // Add ID handler for error checking response
 sox_handler_id_add(conn, sox_handler_publisher_add, id, userdata);
 } else {
 strcpy(id, "rempub_");
 strcat(id, node);
 // Add ID handler for error checking response
 sox_handler_id_add(conn, sox_handler_publisher_remove, id, userdata);
 }

 // Create a new iq stanza
 iq = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(iq, "iq")) < 0)
 return err;
 if ((err = xmpp_stanza_set_type(iq, "set")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "to", pubsubserv)) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "id", id)) < 0)
 return err;

 // Create a new pubsub stanza
 pubsub = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(pubsub, "pubsub")) < 0)
 return err;
 if ((err = xmpp_stanza_set_ns(pubsub,
 "http://jabber.org/protocol/pubsub#owner")) < 0)
 return err;

 // Create a new affiliations stanza
 affiliations = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(affiliations, "affiliations")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(affiliations, "node", node)) < 0)
 return err;

 // Create a new affiliation stanza
 affiliation = xmpp_stanza_new(conn->xmpp_conn->ctx);
 // If add_remove == 1, add as publisher, else remove publisher
 if ((err = xmpp_stanza_set_name(affiliation, "affiliation")) < 0)
 return err;
 if (add_remove) {
 if ((err = xmpp_stanza_set_attribute(affiliation, "affiliation",
 "publisher")) < 0)
 return err;
 } else {
 if ((err = xmpp_stanza_set_attribute(affiliation, "affiliation", "none"))
 < 0)
 return err;
 }
 if ((err = xmpp_stanza_set_attribute(affiliation, "jid", publisher)) < 0)
 return err;

 // Build xmpp message
 if ((err = xmpp_stanza_add_child(affiliations, affiliation)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(pubsub, affiliations)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(iq, pubsub)) < 0)
 return err;

 // Release unneeded stanzas
 xmpp_stanza_release(pubsub);
 xmpp_stanza_release(affiliations);
 xmpp_stanza_release(affiliation);

 // Send out the stanza
 xmpp_send(conn->xmpp_conn, iq);

 // Release the stanza
 xmpp_stanza_release(iq);

 return 0;
 }

 int sox_check_jid_registered(sox_conn_t *conn, const char *checkjid,
 sox_userdata_t * const userdata) {

 xmpp_stanza_t *iq = NULL;
 xmpp_stanza_t *query = NULL;

 char *id;
 int err;

 // Create id for iq stanza consisting of "checkjid_" and jid to check
 // String length is length of checkjid + length of "checkjid_" + terminating character
 id = (char *) malloc((strlen(checkjid) + 10) * sizeof(char));
 if (id == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }

 // Add publisher name to userdata payload so that sox_handler_check_jid_registered() can call sox_acl_publisher_add()
 // It is assumed that userdata->payload1 contains the node name
 userdata->payload2 = checkjid;

 strcpy(id, "checkjid_");
 strcat(id, checkjid);

 // Create a new iq stanza
 iq = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(iq, "iq")) < 0)
 return err;
 if ((err = xmpp_stanza_set_type(iq, "get")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "to", checkjid)) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "id", id)) < 0)
 return err;

 // Create a new query stanza
 query = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(query, "query")) < 0)
 return err;
 if ((err = xmpp_stanza_set_ns(query,
 "http://jabber.org/protocol/disco#info")) < 0)
 return err;

 // Build xmpp message
 if ((err = xmpp_stanza_add_child(iq, query)) < 0)
 return err;

 // Release unneeded stanza
 xmpp_stanza_release(query);

 // Add ID handler for error checking response
 sox_handler_id_add(conn, sox_handler_check_jid_registered, id, userdata);

 // Send out the stanza
 xmpp_send(conn->xmpp_conn, iq);

 // Release the stanza
 xmpp_stanza_release(iq);

 return 0;
 }

 int sox_unsubscribe(sox_conn_t *conn, const char *node, const char *subid,
 sox_userdata_t * const userdata) {

 xmpp_stanza_t *iq = NULL;
 xmpp_stanza_t *pubsub = NULL;
 xmpp_stanza_t *unsubscribe = NULL;

 char *pubsubserv;
 char *id;
 int err;

 // String length is length of server address + length of "pubsub." + terminating character
 pubsubserv = (char *) malloc(
 (strlen(sox_get_server(conn->xmpp_conn->jid)) + 8) * sizeof(char));
 if (pubsubserv == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }
 strcpy(pubsubserv, "pubsub.");
 strcat(pubsubserv, sox_get_server(conn->xmpp_conn->jid));

 // Create id for iq stanza consisting of "unsubscribe_" and node name
 // String length is length of server address + length of "unsub_" + terminating character
 id = (char *) malloc((strlen(node) + 6) * sizeof(char));
 if (id == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }
 strcpy(id, "unsub_");
 strcat(id, node);

 // Create a new iq stanza
 iq = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(iq, "iq")) < 0)
 return err;
 if ((err = xmpp_stanza_set_type(iq, "set")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "to", pubsubserv)) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "id", id)) < 0)
 return err;

 // Create a new pubsub stanza
 pubsub = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(pubsub, "pubsub")) < 0)
 return err;
 if ((err = xmpp_stanza_set_ns(pubsub, "http://jabber.org/protocol/pubsub"))
 < 0)
 return err;

 unsubscribe = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(unsubscribe, "unsubscribe")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(unsubscribe, "node", node)) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(unsubscribe, "jid",
 conn->xmpp_conn->jid)) < 0)
 return err;
 // Add subid if we got one
 if (subid != NULL ) {
 if ((err = xmpp_stanza_set_attribute(unsubscribe, "subid", subid)) < 0)
 return err;
 }

 // Build xmpp message
 if ((err = xmpp_stanza_add_child(pubsub, unsubscribe)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(iq, pubsub)) < 0)
 return err;

 // Release unneeded stanzas
 xmpp_stanza_release(pubsub);
 xmpp_stanza_release(unsubscribe);

 // Add ID handler for error checking response
 sox_handler_id_add(conn, sox_handler_unsubscribe, id, userdata);

 // Send out the stanza
 xmpp_send(conn->xmpp_conn, iq);

 // Release the stanza
 xmpp_stanza_release(iq);

 return 0;
 }

 int sox_node_create(sox_conn_t *conn, const char *node, const char* title,
 const char* access, sox_userdata_t* const userdata) {
 return sox_acl_node_process(conn, node, title, access, userdata, 1);
 }

 int sox_acl_node_process(sox_conn_t *conn, const char *node, const char* title,
 const char* access, sox_userdata_t* const userdata,
 int create_configure) {

 xmpp_stanza_t *iq = NULL;
 xmpp_stanza_t *pubsub = NULL;
 xmpp_stanza_t *create = NULL;
 xmpp_stanza_t *title_field = NULL;
 xmpp_stanza_t *title_value = NULL;
 xmpp_stanza_t *title_val = NULL;
 xmpp_stanza_t *access_field = NULL;
 xmpp_stanza_t *access_value = NULL;
 xmpp_stanza_t *access_val = NULL;
 xmpp_stanza_t *configure = NULL;
 xmpp_stanza_t *x = NULL;

 char *pubsubserv;
 char *id;
 int err;

 // String length is length of server address + length of "pubsub." + terminating character
 pubsubserv = (char *) malloc(
 (strlen(sox_get_server(conn->xmpp_conn->jid)) + 8) * sizeof(char));
 if (pubsubserv == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }
 strcpy(pubsubserv, "pubsub.");
 strcat(pubsubserv, sox_get_server(conn->xmpp_conn->jid));

 // Create id for iq stanza consisting of "aclnode_" and node name
 // String length is length of server address + length of "create_" + terminating character
 id = (char *) malloc((strlen(node) + 9) * sizeof(char));
 if (id == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }
 strcpy(id, "aclnode_");
 strcat(id, node);

 // Create configure stanza
 configure = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(configure, "configure")) < 0)
 return err;

 // Create a new pubsub stanza
 pubsub = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(pubsub, "pubsub")) < 0)
 return err;

 if (create_configure) {
 // Create create node stanza
 create = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(create, "create")) < 0)
 return err;
 if ((xmpp_stanza_set_attribute(create, "node", node)) < 0)
 return err;
 if ((err = xmpp_stanza_set_ns(pubsub,
 "http://jabber.org/protocol/pubsub")) < 0)
 return err;
 } else {
 if ((xmpp_stanza_set_attribute(configure, "node", node)) < 0)
 return err;
 if ((err = xmpp_stanza_set_ns(pubsub,
 "http://jabber.org/protocol/pubsub#owner")) < 0)
 return err;
 }

 // Create a new iq stanza
 iq = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(iq, "iq")) < 0)
 return err;
 if ((err = xmpp_stanza_set_type(iq, "set")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "to", pubsubserv)) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "id", id)) < 0)
 return err;

 // Create x stanza
 x = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(x, "x")) < 0)
 return err;
 if ((err = xmpp_stanza_set_ns(x, "jabber:x:data")) < 0)
 return err;
 if ((err = xmpp_stanza_set_type(x, "submit")) < 0)
 return err;

 if (title != NULL ) {
 // Create title field and value stanza
 title_field = xmpp_stanza_new(conn->xmpp_conn->ctx);
 title_value = xmpp_stanza_new(conn->xmpp_conn->ctx);
 title_val = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(title_field, "field")) < 0)
 return err;
 if ((xmpp_stanza_set_attribute(title_field, "var", "pubsub#title")) < 0)
 return err;
 if ((err = xmpp_stanza_set_name(title_value, "value")) < 0)
 return err;
 if ((err = xmpp_stanza_set_text(title_val, title)) < 0)
 return err;

 // Build title field
 if ((err = xmpp_stanza_add_child(title_value, title_val)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(title_field, title_value)) < 0)
 return err;

 // Release unneeded stanzas
 xmpp_stanza_release(title_val);
 xmpp_stanza_release(title_value);
 }

 if (access != NULL ) {
 // Create access field and value stanza
 access_field = xmpp_stanza_new(conn->xmpp_conn->ctx);
 access_value = xmpp_stanza_new(conn->xmpp_conn->ctx);
 access_val = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(access_field, "field")) < 0)
 return err;
 if ((xmpp_stanza_set_attribute(access_field, "var",
 "pubsub#access_model")) < 0)
 return err;
 if ((err = xmpp_stanza_set_name(access_value, "value")) < 0)
 return err;
 if ((err = xmpp_stanza_set_text(access_val, access)) < 0)
 return err;

 // Build access field
 if ((err = xmpp_stanza_add_child(access_value, access_val)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(access_field, access_value)) < 0)
 return err;

 // Release unneeded stanzas
 xmpp_stanza_release(access_val);
 xmpp_stanza_release(access_value);
 }

 // Build xmpp message
 if (access != NULL || title != NULL ) {
 if (title != NULL ) {

 if ((err = xmpp_stanza_add_child(x, title_field)) < 0)
 return err;
 // Release unneeded stanza
 xmpp_stanza_release(title_field);
 }

 if (access != NULL ) {

 if ((err = xmpp_stanza_add_child(x, access_field)) < 0)
 return err;
 // Release unneeded stanza
 xmpp_stanza_release(access_field);
 }

 if ((err = xmpp_stanza_add_child(configure, x)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(pubsub, configure)) < 0)
 return err;
 // Release unneeded stanzas
 xmpp_stanza_release(x);
 xmpp_stanza_release(configure);
 }

 if (create) {
 if ((err = xmpp_stanza_add_child(pubsub, create)) < 0)
 return err;
 xmpp_stanza_release(create);
 }
 if ((err = xmpp_stanza_add_child(iq, pubsub)) < 0)
 return err;

 // Release unneeded stanzas
 xmpp_stanza_release(pubsub);

 // Add ID handler for error checking response
 sox_handler_id_add(conn, sox_handler_acl_node, id, userdata);

 // Send out the stanza
 xmpp_send(conn->xmpp_conn, iq);

 // Release the stanza
 xmpp_stanza_release(iq);

 return 0;
 }

 int sox_node_delete(sox_conn_t *conn, const char *node,
 sox_userdata_t * const userdata) {

 xmpp_stanza_t *iq = NULL;
 xmpp_stanza_t *pubsub = NULL;
 xmpp_stanza_t *delete = NULL;

 char *pubsubserv;
 char *id;
 int err;

 // String length is length of server address + length of "pubsub." + terminating character
 pubsubserv = (char *) malloc(
 (strlen(sox_get_server(conn->xmpp_conn->jid)) + 8) * sizeof(char));
 if (pubsubserv == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }
 strcpy(pubsubserv, "pubsub.");
 strcat(pubsubserv, sox_get_server(conn->xmpp_conn->jid));

 // Create id for iq stanza consisting of "delete_" and node name
 // String length is length of server address + length of "del_" + terminating character
 id = (char *) malloc((strlen(node) + 8) * sizeof(char));
 if (id == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }
 strcpy(id, "delete_");
 strcat(id, node);

 // Create a new iq stanza
 iq = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(iq, "iq")) < 0)
 return err;
 if ((err = xmpp_stanza_set_type(iq, "set")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "to", pubsubserv)) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "id", id)) < 0)
 return err;

 // Create a new pubsub stanza
 pubsub = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(pubsub, "pubsub")) < 0)
 return err;
 if ((err = xmpp_stanza_set_ns(pubsub,
 "http://jabber.org/protocol/pubsub#owner")) < 0)
 return err;

 // Create delete stanza
 delete = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(delete, "delete")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(delete, "node", node)) < 0)
 return err;

 // Build xmpp message
 if ((err = xmpp_stanza_add_child(pubsub, delete)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(iq, pubsub)) < 0)
 return err;

 // Release unneeded stanzas
 xmpp_stanza_release(pubsub);
 xmpp_stanza_release(delete);

 // Add ID handler for error checking response
 sox_handler_id_add(conn, sox_handler_node_delete, id, userdata);

 // Send out the stanza
 xmpp_send(conn->xmpp_conn, iq);

 // Release the stanza
 xmpp_stanza_release(iq);

 return 0;
 }
 */

/** Publishes the data item to the given connection. Allows node specification
 * and response.
 *
 * @param conn Active connection associated with publishing jid.
 * @param t_meta
 *  */
int mio_item_publish_data(mio_conn_t *conn, mio_stanza_t *item,
		const char *node, mio_response_t * response) {
	int err;

	err = mio_item_publish(conn, item, node, response);
	return err;
}
/*
 int sox_item_publish_meta(sox_conn_t *conn, sox_stanza_t *item,
 const char *node, sox_userdata_t * const userdata) {
 char *new_event;
 char *suffix;

 // Append _meta suffix
 new_event = (char *) malloc((strlen(node) + 6) * sizeof(char));
 if ((suffix = strrchr(node, '_')) != NULL ) {
 if (strcmp(suffix, "_meta") == 0)
 snprintf(new_event, strlen(node) - strlen(suffix) + 1, "%s", node);
 } else
 strcpy(new_event, node);
 strcat(new_event, "_meta");

 return sox_item_publish(conn, item, new_event, userdata);
 }
 */
/** Publishes mio_stanza to event node over a mio connection.
 *
 * @param conn
 * @param item
 * @param node
 * @param response
 * */
int mio_item_publish(mio_conn_t *conn, mio_stanza_t *item, const char *node,
		mio_response_t * response) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *publish = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process publish request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_set_stanza_new(conn, node);

	publish = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(publish, "publish");
	xmpp_stanza_set_attribute(publish, "node", node);

// Build xmpp message
	xmpp_stanza_add_child(publish, item->xmpp_stanza);
	xmpp_stanza_add_child(iq->xmpp_stanza->children, publish);

// Send out the stanza
	err = _mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
			response);

// Release the stanza
	xmpp_stanza_release(publish);
	mio_stanza_free(iq);

	return err;
}

/** Publishes an item to an event node without waiting for a response
 *
 * @param conn Active mio connection.
 * @param item Mio stanza that contains the item to send.
 * @param node The target event node's uuid.
 * */
int mio_item_publish_nonblocking(mio_conn_t *conn, mio_stanza_t *item,
		const char *node) {
	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *publish = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process publish request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_set_stanza_new(conn, node);

	publish = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(publish, "publish");
	xmpp_stanza_set_attribute(publish, "node", node);

// Build xmpp message
	xmpp_stanza_add_child(publish, item->xmpp_stanza);
	xmpp_stanza_add_child(iq->xmpp_stanza->children, publish);

// Send out the stanza
	err = _mio_send_nonblocking(conn, iq);

// Release the stanza
	xmpp_stanza_release(publish);
	mio_stanza_free(iq);

	return err;
}

int mio_transducer_value_merge_publish(mio_conn_t* conn,
		mio_transducer_value_t *transducers, const char *node,
		mio_response_t *response) {
	mio_response_t *query_response = mio_response_new();
	mio_packet_t *packet;
	mio_data_t *data;
	mio_stanza_t *item;
	mio_transducer_value_t *curr_rec, *curr;

// Get data items from node so that we can merge them with the given transducer values
	mio_item_recent_get(conn, node, query_response, 1, "data",
			(mio_handler) mio_handler_item_recent_get);
	if (query_response->response_type != MIO_RESPONSE_PACKET) {
		mio_error("Unable to query node %s for current data", node);
		mio_response_free(query_response);
		return MIO_ERROR_UNEXPECTED_RESPONSE;
	}
	packet = query_response->response;
	if (packet->type != MIO_PACKET_DATA) {
		mio_error("Unable to query node %s for current data", node);
		mio_response_free(query_response);
		return MIO_ERROR_UNEXPECTED_PAYLOAD;
	}
	data = packet->payload;

	// Iterate over current data values from node and merge with given transducer values if their names match
	for (curr_rec = data->transducers; curr_rec != NULL ;
			curr_rec = curr_rec->next) {
		for (curr = transducers; curr != NULL ; curr = curr->next) {
			if (curr->name != NULL && curr_rec->name != NULL ) {
				if (strcmp(curr->name, curr_rec->name) == 0) {
					if (curr_rec->raw_value != NULL )
						free(curr_rec->raw_value);
					curr_rec->raw_value = strdup(curr->raw_value);
					if (curr_rec->typed_value != NULL )
						free(curr_rec->typed_value);
					curr_rec->typed_value = strdup(curr->typed_value);
					if (curr_rec->timestamp != NULL )
						free(curr_rec->timestamp);
					curr_rec->timestamp = strdup(curr->timestamp);
					curr_rec->type = curr->type;
				}
			}
		}
	}

	// Publish merged data
	item = mio_pubsub_item_data_new(conn);
	for (curr = data->transducers; curr != NULL ; curr = curr->next)
		mio_item_transducer_value_add(item, NULL, curr->name, curr->typed_value,
				curr->raw_value, curr->timestamp);

	mio_item_publish_data(conn, item, node, response);
	mio_response_free(query_response);
	mio_stanza_free(item);

	return MIO_OK;
}

/**
 *
 * @param item
 * @param id
 * @param type
 * @param description
 * @param serial_number
 * */
int mio_item_device_add(mio_stanza_t *item, const char *id, const char *type,
		const char *description, const char *serial, const char *timestamp) {

	xmpp_stanza_t *Device = NULL;
	xmpp_stanza_t *device = NULL;
	int err, write_ns = 1;

// Check if input item stanza is NULL and return error if true
	if (item == NULL )
		return -1;

//Check if device node with same id already exists
	if (xmpp_stanza_get_children(item->xmpp_stanza) != NULL ) {
		device = xmpp_stanza_get_children(item->xmpp_stanza);
		while (device != NULL ) {
			if (strcmp(xmpp_stanza_get_name(device), "device") == 0) {
				// Check if MIO ns has already been added and do not add it again
				if (strcmp(xmpp_stanza_get_ns(device),
						"http://jabber.org/protocol/mio") == 0)
					write_ns = 0;
				if (strcmp(xmpp_stanza_get_attribute(device, "id"), id) == 0)
					return -1;
			}

			device = xmpp_stanza_get_next(device);
		}
	}

// Create a new stanza for a device installation and set its attributes
	Device = xmpp_stanza_new(item->xmpp_stanza->ctx);
	if ((err = xmpp_stanza_set_name(Device, "device")) < 0)
		return err;
	if (write_ns) {
		if ((err = xmpp_stanza_set_ns(Device, "http://jabber.org/protocol/mio"))
				< 0)
			return err;
	}
	if ((err = xmpp_stanza_set_attribute(Device, "id", id)) < 0)
		return err;
	if (type != NULL ) {
		if ((err = xmpp_stanza_set_type(Device, type)) < 0)
			return err;
	}
	if (description != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Device, "description", description))
				< 0)
			return err;
	}
	if (serial != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Device, "serial", serial)) < 0)
			return err;
	}
	if ((err = xmpp_stanza_set_attribute(Device, "timestamp", timestamp)) < 0)
		return err;

// Attach device installation stanza as child of inputted stanza
	xmpp_stanza_add_child(item->xmpp_stanza, Device);

// Release created device installation stanza
	xmpp_stanza_release(Device);

	return 0;
}

/** Adds transducer meta information to the meta item stanza.
 *
 * @param item
 * @param device_id
 * @param name
 * @param id
 * @param canActuate
 * @param units
 * @param unit_scalar
 * @param has_own_event_node
 * @param transducer_type_name
 * @param manufacturer
 * @param part_number
 * @param serial_number
 * @param min_value
 * @param max_value
 * @param resolution
 * @param precision
 * @param accuracy
 *
 * @returns MIO_OK upon success
 * */
int mio_item_transducer_add(mio_stanza_t *item, const char *device_id,
		const char *name, const char *id, const char *canActuate,
		const char *units, const char *unit_scalar,
		const char *has_own_event_node, const char *transducer_type_name,
		const char *manufacturer, const char *part_number, const char *serial,
		const char *min_value, const char *max_value, const char *resolution,
		const char *precision, const char *accuracy) {

	xmpp_stanza_t *device = NULL;
	xmpp_stanza_t *trans = NULL;
	xmpp_stanza_t *Transducer = NULL;
	int err;

// Check if message is empty and return error if true
	if (item == NULL )
		return -1;

// Check if message has no children and return error if true
	if (xmpp_stanza_get_children(item->xmpp_stanza) == NULL )
		return -1;

	device = xmpp_stanza_get_children(item->xmpp_stanza);
	while (device != NULL ) {
		if (strcmp(xmpp_stanza_get_name(device), "device") == 0) {
			if (strcmp(xmpp_stanza_get_attribute(device, "id"), device_id) == 0)
				break;
		}

		device = xmpp_stanza_get_next(device);
	}

	if (device == NULL )
		return -1;

	if (xmpp_stanza_get_children(device) != NULL ) {
		trans = xmpp_stanza_get_children(device);
		while (trans != NULL ) {
			if (strcmp(xmpp_stanza_get_name(trans), "transducer") == 0) {
				if (strcmp(xmpp_stanza_get_attribute(trans, "id"), id) == 0)
					return -1;
			}

			trans = xmpp_stanza_get_next(trans);
		}
	}

// Create a new stanza for a transducer installation and set its attributes
	Transducer = xmpp_stanza_new(item->xmpp_stanza->ctx);
	if ((err = xmpp_stanza_set_name(Transducer, "transducer")) < 0)
		return err;
	if ((err = xmpp_stanza_set_attribute(Transducer, "id", id)) < 0)
		return err;
	if (name != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "name", name)) < 0)
			return err;
	}
	if (canActuate != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "canActuate",
				canActuate)) < 0)
			return err;
	}
	if (units != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "units", units)) < 0)
			return err;
	}
	if (unit_scalar != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "unitScalar",
				unit_scalar)) < 0)
			return err;
	}
	if (has_own_event_node != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "hasOwnEventNode",
				has_own_event_node)) < 0)
			return err;
	}
	if (transducer_type_name != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "transducerTypeName",
				transducer_type_name)) < 0)
			return err;
	}
	if (manufacturer != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "manufacturer",
				manufacturer)) < 0)
			return err;
	}
	if (part_number != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "partNumber",
				part_number)) < 0)
			return err;
	}
	if (part_number != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "serial", serial)) < 0)
			return err;
	}
	if (min_value != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "minValue", min_value))
				< 0)
			return err;
	}
	if (max_value != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "maxValue", max_value))
				< 0)
			return err;
	}
	if (resolution != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "resolution",
				resolution)) < 0)
			return err;
	}
	if (precision != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "precision", precision))
				< 0)
			return err;
	}
	if (accuracy != NULL ) {
		if ((err = xmpp_stanza_set_attribute(Transducer, "accuracy", accuracy))
				< 0)
			return err;
	}

// Attach transducer installation stanza as child of inputed stanza
	xmpp_stanza_add_child(device, Transducer);

// Release created transducer installation stanza
	xmpp_stanza_release(Transducer);

	return 0;
}

int mio_item_transducer_value_actuate_add(mio_stanza_t *item,
		const char *deviceID, const char *id, const char *typed_val,
		const char *raw_val, const char *timestamp) {
	return _mio_item_transducer_value_add(item, deviceID, id, typed_val,
			raw_val, timestamp, 1);
}

int mio_item_transducer_value_add(mio_stanza_t *item, const char *deviceID,
		const char *id, const char *typed_val, const char *raw_val,
		const char *timestamp) {
	return _mio_item_transducer_value_add(item, deviceID, id, typed_val,
			raw_val, timestamp, 0);
}

static int _mio_item_transducer_value_add(mio_stanza_t *item,
		const char *deviceID, const char *id, const char *typed_val,
		const char *raw_val, const char *timestamp, int actuate) {

	xmpp_stanza_t *child = NULL;
	xmpp_stanza_t *trans = NULL;
	xmpp_stanza_t *TransducerValue = NULL;
	int err;

// Check if message is empty and return error if true
	if (item == NULL )
		return -1;

	if (xmpp_stanza_get_children(item->xmpp_stanza) == NULL )
		return -1;

	child = xmpp_stanza_get_children(item->xmpp_stanza);

// FIXME: This will crash if id == NULL
	if (strcmp(xmpp_stanza_get_name(child), "data") != 0) {
		while (child != NULL ) {
			if (strcmp(xmpp_stanza_get_name(child), "device") == 0) {
				if (strcmp(xmpp_stanza_get_attribute(child, "id"), deviceID)
						== 0)
					break;
			}

			child = xmpp_stanza_get_next(child);
		}

		if (child == NULL )
			return -1;

		if (xmpp_stanza_get_children(child) != NULL ) {
			trans = xmpp_stanza_get_children(child);
			while (trans != NULL ) {
				if (strcmp(xmpp_stanza_get_name(trans), "transducer") == 0) {
					if (strcmp(xmpp_stanza_get_attribute(trans, "id"), id) == 0)
						break;
				}

				trans = xmpp_stanza_get_next(trans);
			}
		}

		if (trans == NULL )
			return -1;
	}

// If we are using a data stanza to publish transducerValues without device or transducer information
// Check if a transducerValue with the same id has already been added
	else if (id != NULL ) {
		if (xmpp_stanza_get_children(child) != NULL ) {
			TransducerValue = xmpp_stanza_get_children(child);
			while (TransducerValue != NULL ) {
				if (strcmp(xmpp_stanza_get_name(TransducerValue),
						"transducerValue") == 0
						|| strcmp(xmpp_stanza_get_name(TransducerValue),
								"transducerSetValue") == 0) {
					if (xmpp_stanza_get_attribute(TransducerValue,
							"id") != NULL) {
						if (strcmp(
								xmpp_stanza_get_attribute(TransducerValue,
										"id"), id) == 0)
							break;
					}
				}
				TransducerValue = xmpp_stanza_get_next(TransducerValue);
			}
		}
	}

	if (TransducerValue == NULL ) {
// Create a new stanza for a transducer value and set its name if it doesn't already exist
		TransducerValue = xmpp_stanza_new(item->xmpp_stanza->ctx);
		if (actuate) {
			if ((err = xmpp_stanza_set_name(TransducerValue,
					"transducerSetValue")) < 0)
				return err;

		} else {
			if ((err = xmpp_stanza_set_name(TransducerValue, "transducerValue"))
					< 0)
				return err;
		}
// Attach transducer value stanza as child of inputed stanza
		if (trans == NULL )
			xmpp_stanza_add_child(child, TransducerValue);
		else
			xmpp_stanza_add_child(trans, TransducerValue);
// Release created transducer installation stanza
		xmpp_stanza_release(TransducerValue);
	}
	if (id != NULL )
		if ((err = xmpp_stanza_set_attribute(TransducerValue, "id", id)) < 0)
			return err;

// Overwrite or add attributes of the transducerValue stanza
	if ((err = xmpp_stanza_set_attribute(TransducerValue, "typedValue",
			typed_val)) < 0)
		return err;
	if (raw_val != NULL )
		if ((err = xmpp_stanza_set_attribute(TransducerValue, "rawValue",
				raw_val)) < 0)
			return err;
	if (timestamp != NULL )
		if ((err = xmpp_stanza_set_attribute(TransducerValue, "timestamp",
				timestamp)) < 0)
			return err;

	return MIO_OK;
}

/** Sends XMPP presence stanza to the XMPP server.
 *
 *  @param conn
 *
 *  @returns MIO_OK on success and MIO_ERROR_DISCONNECTED if no connection to server.
 *  */
int mio_listen_start(mio_conn_t *conn) {
	xmpp_stanza_t *pres;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error("Cannot start listening since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}
//Send initial <presence/> so that we appear online to contacts
	pres = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(pres, "presence");
	mio_debug("Sending presence stanza");
	xmpp_send(conn->xmpp_conn, pres);
	xmpp_stanza_release(pres);
	conn->presence_status = MIO_PRESENCE_PRESENT;
	return MIO_OK;
}

/** Sends XMPP presence unavailable stanza to the XMPP server.
 *
 *  @param conn Active MIO connection.
 *
 *  @returns MIO_OK on success and MIO_ERROR_DISCONNECTED if no connection to server.
 *  */
int mio_listen_stop(mio_conn_t *conn) {
	xmpp_stanza_t *pres;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error("Cannot stop listening since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}
//Send <presence type="unavailable"/> so that we appear unavailable to contacts
	pres = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(pres, "presence");
	xmpp_stanza_set_type(pres, "unavailable");
	mio_debug("Sending presence unavailable stanza");
	xmpp_send(conn->xmpp_conn, pres);
	xmpp_stanza_release(pres);
	conn->presence_status = MIO_PRESENCE_UNAVAILABLE;
	return MIO_OK;
}

/** Indicates to XMPP server that conn should receive pubsub data.
 *
 *  @param conn Active MIO connection.
 *
 *  @returns SOX_OK on success and MIO_ERROR_DISCONNECTED if no connection to server.
 *  */
int mio_pubsub_data_listen_start(mio_conn_t *conn) {
	mio_request_t *request = NULL;
	int err;
// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot start listening for data since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}
// Tell the sever that we can receive publications if we haven't already done so
	if (conn->presence_status != MIO_PRESENCE_PRESENT)
		mio_listen_start(conn);

	request = _mio_request_new();
	strcpy(request->id, "pubsub_data_rx");

// If we have too many open requests, wait
	sem_wait(conn->mio_open_requests);

	err = mio_handler_add(conn, (mio_handler) mio_handler_pubsub_data_receive,
			"http://jabber.org/protocol/pubsub#event", NULL, NULL, request,
			NULL, NULL );
	if (err == MIO_OK)
		conn->pubsub_rx_listening = 1;

	return err;
}

/** Indicates to XMPP server that client will stop listining for pubsub data.
 *
 *  @param conn Active MIO connection.
 *
 *  @returns SOX_OK on success.
 *  */
int mio_pubsub_data_listen_stop(mio_conn_t *conn) {
	mio_request_t *request;

// Signal mio_pubsub_data_receive if it is waiting and delete pubsub_data_rx request if we were listening
	request = _mio_request_get(conn, "pubsub_data_rx");
	if (request != NULL ) {
// FIXME: Potential deadlock on mio_pubsub_data_receive() function when condition signaled without function waiting
		conn->pubsub_rx_listening = 0;
		_mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
		_mio_request_delete(conn, "pubsub_data_rx");
	}
	return MIO_OK;
}

/** Clears the packet receive queue for pubsub packets.
 *
 * @param conn Active MIO connection.
 *  */
void mio_pubsub_data_rx_queue_clear(mio_conn_t *conn) {
	mio_response_t *response = _mio_pubsub_rx_queue_dequeue(conn);
	while (response != NULL ) {
		response = _mio_pubsub_rx_queue_dequeue(conn);
		mio_response_free(response);
	}
}

/** Clears the packet receive queue for pubsub packets.
 *
 * @param conn Active MIO connection.
 *
 * @returns MIO_OK on sccuess, and MIO_ERROR_DISCONNECTED on failure.
 *  */
int mio_pubsub_data_receive(mio_conn_t *conn, mio_response_t *response) {
	int err;
	mio_request_t *request;
	mio_response_t *rx_response = NULL;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process data_receive request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

// If there is a pubsub response waiting on the pubsub_rx_queue, return it. Else block until we receive one.
	rx_response = _mio_pubsub_rx_queue_dequeue(conn);
	if (rx_response != NULL ) {
		strcpy(response->id, rx_response->id);
		response->name = rx_response->name;
		response->ns = rx_response->ns;
		response->response_type = rx_response->response_type;
		response->type = rx_response->type;
		response->response = rx_response->response;
		response->stanza = rx_response->stanza;
		free(rx_response);
		return MIO_OK;
	}

// Call mio_pubsub_data_listen_start() if we aren't listening yet
	request = _mio_request_get(conn, "pubsub_data_rx");
	if (request == NULL ) {
		err = mio_pubsub_data_listen_start(conn);
		if (err != MIO_OK)
			return err;
		request = _mio_request_get(conn, "pubsub_data_rx");
	}
	while (rx_response == NULL && conn->pubsub_rx_queue_len <= 0
			&& conn->pubsub_rx_listening) {
		while (!request->predicate) {
			err = pthread_mutex_lock(&request->mutex);
			if (err != 0) {
				mio_error("Unable to lock mutex for request %s", request->id);
				return MIO_ERROR_MUTEX;
			}
// Wait for response from server
			err = pthread_cond_wait(&request->cond, &request->mutex);
			if (err != 0) {
				mio_error("Conditional wait for request with id %s failed",
						request->id);
				pthread_mutex_unlock(&request->mutex);
				return MIO_ERROR_COND_WAIT;
			} else {
				mio_debug("Got response for request with id %s", request->id);
				pthread_mutex_unlock(&request->mutex);
			}
		}
	}
	request->predicate = 0;
	rx_response = _mio_pubsub_rx_queue_dequeue(conn);
	if (rx_response == NULL )
		return MIO_ERROR_NO_RESPONSE;

	strcpy(response->id, rx_response->id);
	response->name = rx_response->name;
	response->ns = rx_response->ns;
	response->response_type = rx_response->response_type;
	response->type = rx_response->type;
	response->response = rx_response->response;
	response->stanza = rx_response->stanza;
	free(rx_response);
	return MIO_OK;
}

/** Queries for the subscribers of the event node. Subject to the access
 *      rights of the logged in user.
 *
 * @param conn Active MIO connection.
 * @param node The event node uuid to query for subscription information.
 * @param response The response from the XMPP server. Contains subscription
 *      information
 * */
int mio_subscriptions_query(mio_conn_t *conn, const char *node,
		mio_response_t * response) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *subscriptions = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process subscriptions query since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_get_stanza_new(conn, node);

// Create a new subscriptions stanza
	subscriptions = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(subscriptions, "subscriptions");

// If we are querying the affiliations of a node, add the node attribute to the subscriptions stanza and change the namespace of the pubsub stanza
	if (node != NULL ) {
		xmpp_stanza_set_attribute(subscriptions, "node", node);
		xmpp_stanza_set_ns(iq->xmpp_stanza->children,
				"http://jabber.org/protocol/pubsub#owner");
	}

// Build xmpp message
	xmpp_stanza_add_child(iq->xmpp_stanza->children, subscriptions);

// Send out the stanza
	err = _mio_send_blocking(conn, iq,
			(mio_handler) mio_handler_subscriptions_query, response);

// Release unneeded stanzas
	xmpp_stanza_release(subscriptions);
	mio_stanza_free(iq);

	return err;
}

int mio_acl_affiliations_query(mio_conn_t *conn, const char *node,
		mio_response_t * response) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *affiliations = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process affiliations query since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_get_stanza_new(conn, node);
	xmpp_stanza_set_ns(iq->xmpp_stanza->children,
			"http://jabber.org/protocol/pubsub");

// Create a new affiliations stanza
	affiliations = xmpp_stanza_new(conn->xmpp_conn->ctx);
	if ((err = xmpp_stanza_set_name(affiliations, "affiliations")) < 0)
		return err;

// If node input is NULL, query affiliations of the jid, else query affiliations of the node
	if (node != NULL ) {
		if ((err = xmpp_stanza_set_attribute(affiliations, "node", node)) < 0)
			return err;
		xmpp_stanza_set_ns(iq->xmpp_stanza->children,
				"http://jabber.org/protocol/pubsub#owner");
	}

// Build xmpp message
	xmpp_stanza_add_child(iq->xmpp_stanza->children, affiliations);

// Send out the stanza
	err = _mio_send_blocking(conn, iq,
			(mio_handler) mio_handler_acl_affiliations_query, response);

// Release unneeded stanzas
	xmpp_stanza_release(affiliations);
	mio_stanza_free(iq);

	return err;
}

/** Sets an event node's affiliation with a specific jid.
 *
 * @param conn Active MIO connection.
 * @param node Event node's UUID which will have its affiliation modified.
 * @param jid The user's jid whose affiliation with node is to change.
 * @param type The type of affiliation the jid should have with the event node
 * @param response [out] Where the response to the affiliation set request is stored.
 *
 * @returns SOX_OK on success, MIO_ERROR_DISCONNECTED on disconnection.
 * */
int mio_acl_affiliation_set(mio_conn_t *conn, const char *node, const char *jid,
		mio_affiliation_type_t type, mio_response_t * response) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *affiliations = NULL, *affiliation = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process affiliation set request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_set_stanza_new(conn, node);
	xmpp_stanza_set_ns(iq->xmpp_stanza->children,
			"http://jabber.org/protocol/pubsub#owner");

// Create a new affiliations stanza
	affiliations = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(affiliations, "affiliations");
	xmpp_stanza_set_attribute(affiliations, "node", node);

// Create a new affiliation stanza
	affiliation = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(affiliation, "affiliation");
	xmpp_stanza_set_attribute(affiliation, "jid", jid);

	switch (type) {
	case MIO_AFFILIATION_NONE:
		xmpp_stanza_set_attribute(affiliation, "affiliation", "none");
		break;
	case MIO_AFFILIATION_OWNER:
		xmpp_stanza_set_attribute(affiliation, "affiliation", "owner");
		break;
	case MIO_AFFILIATION_MEMBER:
		xmpp_stanza_set_attribute(affiliation, "affiliation", "member");
		break;
	case MIO_AFFILIATION_PUBLISHER:
		xmpp_stanza_set_attribute(affiliation, "affiliation", "publisher");
		break;
	case MIO_AFFILIATION_PUBLISH_ONLY:
		xmpp_stanza_set_attribute(affiliation, "affiliation", "publish-only");
		break;
	case MIO_AFFILIATION_OUTCAST:
		xmpp_stanza_set_attribute(affiliation, "affiliation", "outcast");
		break;
	default:
		mio_error("Unknown or missing affiliation type");
		return MIO_ERROR_UNKNOWN_AFFILIATION_TYPE;
		break;
	}

// Build xmpp message
	xmpp_stanza_add_child(affiliations, affiliation);
	xmpp_stanza_add_child(iq->xmpp_stanza->children, affiliations);

// Send out the stanza
	err = _mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
			response);

// Release unneeded stanzas
	xmpp_stanza_release(affiliations);
	xmpp_stanza_release(affiliation);
	mio_stanza_free(iq);

	return err;
}

/** Creates a new event node.
 *
 * @param conn Active MIO connection
 * @param node The node id of the event node. Should be uuid.
 * @param title The title of the event node.
 * @param access The access model of the event node
 * @param [out] response The response to the create node request
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on failed connection.
 * */
int mio_node_create(mio_conn_t * conn, const char *node, const char* title,
		const char* access, mio_response_t * response) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *create = NULL;
	xmpp_stanza_t *title_field = NULL;
	xmpp_stanza_t *title_value = NULL;
	xmpp_stanza_t *title_val = NULL;
	xmpp_stanza_t *access_field = NULL;
	xmpp_stanza_t *access_value = NULL;
	xmpp_stanza_t *access_val = NULL;
	xmpp_stanza_t *configure = NULL;
	xmpp_stanza_t *x = NULL;
	xmpp_stanza_t *form_field = NULL;
	xmpp_stanza_t *form_field_value = NULL;
	xmpp_stanza_t *form_field_value_text = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process node creation request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_set_stanza_new(conn, node);

// Create configure stanza
	configure = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(configure, "configure");

// Create create node stanza
	create = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(create, "create");
	xmpp_stanza_set_attribute(create, "node", node);

// Create x stanza
	x = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(x, "x");
	xmpp_stanza_set_ns(x, "jabber:x:data");
	xmpp_stanza_set_type(x, "submit");

	if (title != NULL ) {
// Create title field and value stanza
		form_field = xmpp_stanza_new(conn->xmpp_conn->ctx);
		form_field_value = xmpp_stanza_new(conn->xmpp_conn->ctx);
		form_field_value_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
		title_field = xmpp_stanza_new(conn->xmpp_conn->ctx);
		title_value = xmpp_stanza_new(conn->xmpp_conn->ctx);
		title_val = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(form_field_value, "value");
		xmpp_stanza_set_text(form_field_value_text,
				"http://jabber.org/protocol/pubsub#node_config");
		xmpp_stanza_set_name(form_field, "field");
		xmpp_stanza_set_attribute(form_field, "var", "FORM_TYPE");
		xmpp_stanza_set_type(form_field, "hidden");
		xmpp_stanza_set_name(title_field, "field");
		xmpp_stanza_set_attribute(title_field, "var", "pubsub#title");
		xmpp_stanza_set_name(title_value, "value");
		err = xmpp_stanza_set_text(title_val, title);

// Build title field
		xmpp_stanza_add_child(form_field_value, form_field_value_text);
		xmpp_stanza_add_child(form_field, form_field_value);
		xmpp_stanza_add_child(title_value, title_val);
		xmpp_stanza_add_child(title_field, title_value);

// Release unneeded stanzas
		xmpp_stanza_release(title_val);
		xmpp_stanza_release(title_value);
	}

	if (access != NULL ) {
// Create access field and value stanza
		access_field = xmpp_stanza_new(conn->xmpp_conn->ctx);
		access_value = xmpp_stanza_new(conn->xmpp_conn->ctx);
		access_val = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(access_field, "field");
		xmpp_stanza_set_attribute(access_field, "var", "pubsub#access_model");
		xmpp_stanza_set_name(access_value, "value");
		xmpp_stanza_set_text(access_val, access);

// Build access field
		xmpp_stanza_add_child(access_value, access_val);
		xmpp_stanza_add_child(access_field, access_value);

// Release unneeded stanzas
		xmpp_stanza_release(access_val);
		xmpp_stanza_release(access_value);
	}

// Build xmpp message
	if (access != NULL || title != NULL ) {
		if (title != NULL ) {
			xmpp_stanza_add_child(x, form_field);
			xmpp_stanza_add_child(x, title_field);
			// Release unneeded stanza
			xmpp_stanza_release(title_field);
		}

		if (access != NULL ) {
			err = xmpp_stanza_add_child(x, access_field);
			// Release unneeded stanza
			xmpp_stanza_release(access_field);
		}

		xmpp_stanza_add_child(configure, x);
	}
	if (create) {
		xmpp_stanza_add_child(iq->xmpp_stanza->children, create);
		xmpp_stanza_release(create);
	}

//TODO: BAD WAY OF DOING THIS
	if (access != NULL || title != NULL ) {
		xmpp_stanza_add_child(iq->xmpp_stanza->children, configure);

// Release unneeded stanzas
		xmpp_stanza_release(x);
		xmpp_stanza_release(configure);
	}

// Send out the stanza
	err = _mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
			response);

	if (form_field != NULL )
		xmpp_stanza_release(form_field);
	if (form_field_value != NULL )
		xmpp_stanza_release(form_field_value);
	if (form_field_value_text != NULL )
		xmpp_stanza_release(form_field_value_text);
// Release the stanzas
	mio_stanza_free(iq);

	return err;
}

/** Creates an collection event node.
 *
 * @param conn Active MIO connection.
 * @param node The unique node id of the collection node. Should be UUID.
 * @param name The name of the collection node. Need not be unique.
 *
 * @returns MIO_OK upon success, MIO_ERROR_DISCONNECTED on connection failure.
 * */
int mio_collection_node_create(mio_conn_t * conn, const char *node,
		const char *title, mio_response_t * response) {

	int err;
	xmpp_stanza_t *create = NULL, *curr = NULL, *field_title = NULL, *value =
			NULL, *value_text = NULL;
	mio_stanza_t *collection_stanza = mio_node_config_new(conn, node,
			MIO_NODE_TYPE_COLLECTION);
// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process collection node creation request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	create = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(create, "create");
	xmpp_stanza_set_attribute(create, "node", node);

	if (title != NULL ) {
		field_title = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(field_title, "field");
		xmpp_stanza_set_attribute(field_title, "var", "pubsub#title");
		value = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(value, "value");
		value_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_text(value_text, title);
		xmpp_stanza_add_child(value, value_text);
		xmpp_stanza_add_child(field_title, value);
		xmpp_stanza_add_child(
				collection_stanza->xmpp_stanza->children->children->children,
				field_title);
	}

// Put create stanza ahead of config stanza in collection_stanza
	curr = collection_stanza->xmpp_stanza->children->children;
	curr->prev = create;
	create->next = curr;
	collection_stanza->xmpp_stanza->children->children = create;

// Send out the stanza
	err = _mio_send_blocking(conn, collection_stanza,
			(mio_handler) mio_handler_error, response);

	mio_stanza_free(collection_stanza);

	return err;
}

/** Allows user to configure existing collection node.
 *
 * @param conn Active MIO connection.
 * @param node The node id of the collection node.
 * @param collection_stanza Stanza describing collection node.
 * @param response Pointer to allocated mio response.
 *      Holds response to configure request.
 *
 * @returns MIO_OK on success. MIO_ERROR_DISCONNECTED on connection error.
 * */
int mio_collection_node_configure(mio_conn_t * conn, const char *node,
		mio_stanza_t *collection_stanza, mio_response_t * response) {

	xmpp_stanza_t *configure = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process collection node configuration request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

//Update pubsub ns to pubsub#owner
	xmpp_stanza_set_ns(collection_stanza->xmpp_stanza->children,
			"http://jabber.org/protocol/pubsub#owner");

	configure = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(configure, "configure");
	xmpp_stanza_set_attribute(configure, "node", node);

	xmpp_stanza_add_child(collection_stanza->xmpp_stanza->children, configure);

// Send out the stanza
	err = _mio_send_blocking(conn, collection_stanza,
			(mio_handler) mio_handler_error, response);

	mio_stanza_free(collection_stanza);

	return err;
}

/** Queries collection node for children.
 *
 * @param conn Active MIO connection.
 * @param node Node id of the collection node to query.
 * @param response The response packet containing the children of the collection node
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on MIO disconnection.
 * */
int mio_collection_children_query(mio_conn_t * conn, const char *node,
		mio_response_t * response) {

	int err;
	xmpp_stanza_t *query = NULL;

// Create stanzas
	mio_stanza_t *iq = _mio_pubsub_iq_get_stanza_new(conn, node);
	query = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#items");
	xmpp_stanza_set_attribute(query, "node", node);
	xmpp_stanza_add_child(iq->xmpp_stanza, query);

// Send out the stanza
	err = _mio_send_blocking(conn, iq,
			(mio_handler) mio_handler_collection_children_query, response);

	mio_stanza_free(iq);

	return err;
}

/** Queries collection node for parents.
 *
 * @param conn Active MIO connection.
 * @param node Node id of the collection node to query.
 * @param response The response packet containing the parents of the collection node
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on MIO disconnection.
 * */
int mio_collection_parents_query(mio_conn_t * conn, const char *node,
		mio_response_t * response) {

	int err;
	xmpp_stanza_t *configure = NULL;

// Create stanzas
	mio_stanza_t *iq = _mio_pubsub_get_stanza_new(conn, node);
	xmpp_stanza_set_ns(iq->xmpp_stanza->children,
			"http://jabber.org/protocol/pubsub#owner");
// Create configure stanza
	configure = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(configure, "configure");
	xmpp_stanza_set_attribute(configure, "node", node);

// Build message
	xmpp_stanza_add_child(iq->xmpp_stanza->children, configure);

// Send out the stanza
	err = _mio_send_blocking(conn, iq,
			(mio_handler) mio_handler_collection_parents_query, response);

	mio_stanza_free(iq);

	return err;
}

mio_stanza_t *mio_node_config_new(mio_conn_t * conn, const char *node,
		mio_node_type_t type) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *configure = NULL, *x = NULL, *field_form = NULL, *value_ns =
			NULL, *field_pubsub = NULL, *value_collection = NULL,
			*value_ns_text = NULL, *value_collection_text = NULL;

// Create stanzas
	iq = _mio_pubsub_set_stanza_new(conn, node);

	configure = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(configure, "configure");
	xmpp_stanza_set_attribute(configure, "node", node);

	x = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(x, "x");
	xmpp_stanza_set_ns(x, "jabber:x:data");
	xmpp_stanza_set_attribute(x, "type", "submit");

	field_form = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(field_form, "field");
	xmpp_stanza_set_attribute(field_form, "var", "FORM_TYPE");
	xmpp_stanza_set_attribute(field_form, "type", "hidden");

	value_ns = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(value_ns, "value");

	value_ns_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_text(value_ns_text,
			"http://jabber.org/protocol/pubsub#node_config");
	xmpp_stanza_add_child(x, field_form);

	if (type != MIO_NODE_TYPE_UNKNOWN) {
		field_pubsub = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(field_pubsub, "field");
		xmpp_stanza_set_attribute(field_pubsub, "var", "pubsub#node_type");

		value_collection = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(value_collection, "value");

		value_collection_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
		if (type == MIO_NODE_TYPE_COLLECTION)
			xmpp_stanza_set_text(value_collection_text, "collection");
		else
			xmpp_stanza_set_text(value_collection_text, "leaf");

		xmpp_stanza_add_child(value_collection, value_collection_text);
		xmpp_stanza_add_child(field_pubsub, value_collection);
		xmpp_stanza_add_child(x, field_pubsub);
	}
// Build xmpp message
	xmpp_stanza_add_child(value_ns, value_ns_text);
	xmpp_stanza_add_child(field_form, value_ns);
	xmpp_stanza_add_child(configure, x);
	xmpp_stanza_add_child(iq->xmpp_stanza->children, configure);

	return iq;
}

/** Adds a child node to a collection nodes stanza
 *
 * @param conn Active MIO connection.
 * @param child The node id of the child to add to the collection node's stanza
 * @param collection_stanza The collection stanza to add the child node id to.
 *
 * @returns MIO_OK upon success, MIO_ERROR_DISCONNECTED if disconnected.
 * */
int mio_collection_config_child_add(mio_conn_t * conn, const char *child,
		mio_stanza_t *collection_stanza) {

	xmpp_stanza_t *value_child = NULL, *value_child_text = NULL, *field = NULL,
			*curr = NULL, *curr_value;
	char *curr_attribute = NULL;

	xmpp_stanza_set_ns(collection_stanza->xmpp_stanza->children,
			"http://jabber.org/protocol/pubsub#owner");

// Create value stanza for child
	value_child = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(value_child, "value");
	value_child_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_text(value_child_text, child);
	xmpp_stanza_add_child(value_child, value_child_text);

// Get the field stanzas and find the pubsub#children one
	curr = xmpp_stanza_get_children(
			collection_stanza->xmpp_stanza->children->children->children);
	while (curr != NULL ) {
		curr_attribute = xmpp_stanza_get_attribute(curr, "var");
		if (curr_attribute != NULL ) {
			if (strcmp(curr_attribute, "pubsub#children") == 0)
				break;
		}
		curr = xmpp_stanza_get_next(curr);
	}

// If there is no pubsub#children value stanza, create it and add the child collection node
	if (curr == NULL ) {
		field = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(field, "field");
		xmpp_stanza_set_attribute(field, "var", "pubsub#children");
		xmpp_stanza_add_child(field, value_child);
		xmpp_stanza_add_child(
				collection_stanza->xmpp_stanza->children->children->children,
				field);
	} else {
		curr_value = xmpp_stanza_get_children(curr);
		while (curr_value != NULL ) {
			if (strcmp(xmpp_stanza_get_text(curr_value), child) == 0)
				return MIO_ERROR_DUPLICATE_ENTRY;
			curr_value = xmpp_stanza_get_next(curr_value);
		}
// If no duplicate is found, add parent to list of parent nodes
		xmpp_stanza_add_child(curr, value_child);
	}

	return MIO_OK;
}

/** Adds collection node as parent to the child node.
 *
 * @param conn Active MIO connection.
 * @param parent Event node id of the collection node parent.
 * @param collection_stanza The child's collection stanza.
 *
 * @returns SOX_OK on success, SOX_ERROR_DISCONNECTED when disconected.
 * */
int mio_collection_config_parent_add(mio_conn_t * conn, const char *parent,
		mio_stanza_t *collection_stanza) {

	xmpp_stanza_t *value_parent = NULL, *value_parent_text = NULL,
			*field = NULL, *curr = NULL, *curr_value = NULL;

	char *curr_attribute = NULL;

	xmpp_stanza_set_ns(collection_stanza->xmpp_stanza->children,
			"http://jabber.org/protocol/pubsub#owner");
// Create value stanza for parent
	value_parent = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(value_parent, "value");
	value_parent_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_text(value_parent_text, parent);
	xmpp_stanza_add_child(value_parent, value_parent_text);

// Get the field stanzas and find the pubsub#collection one
	curr = xmpp_stanza_get_children(
			collection_stanza->xmpp_stanza->children->children->children);
	while (curr != NULL ) {
		curr_attribute = xmpp_stanza_get_attribute(curr, "var");
		if (curr_attribute != NULL ) {
			if (strcmp(curr_attribute, "pubsub#collection") == 0)
				break;
		}
		curr = xmpp_stanza_get_next(curr);
	}

// If there is no pubsub#collection value stanza, create it and add the child collection node
	if (curr == NULL ) {
		field = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(field, "field");
		xmpp_stanza_set_attribute(field, "var", "pubsub#collection");
		xmpp_stanza_add_child(field, value_parent);
		xmpp_stanza_add_child(
				collection_stanza->xmpp_stanza->children->children->children,
				field);
// Check if we are trying to add a duplicate parent
	} else {
		curr_value = xmpp_stanza_get_children(curr);
		while (curr_value != NULL ) {
			if (strcmp(xmpp_stanza_get_text(curr_value), parent) == 0)
				return MIO_ERROR_DUPLICATE_ENTRY;
			curr_value = xmpp_stanza_get_next(curr_value);
		}
// If no duplicate is found, add parent to list of parent nodes
		xmpp_stanza_add_child(curr, value_parent);
	}

	return MIO_OK;
}

int mio_collection_child_add(mio_conn_t * conn, const char *child,
		const char *parent, mio_response_t *response) {

	mio_response_t *temp_response = mio_response_new();
	mio_stanza_t *collection_stanza = mio_node_config_new(conn, parent,
			MIO_NODE_TYPE_UNKNOWN);
	mio_collection_children_query(conn, parent, temp_response);
	if (temp_response->response_type == MIO_RESPONSE_PACKET) {
		mio_packet_t *packet = (mio_packet_t*) temp_response->response;
		mio_collection_t *coll = (mio_collection_t*) packet->payload;
		while (coll != NULL ) {
			mio_collection_config_child_add(conn, coll->node,
					collection_stanza);
			coll = coll->next;
		}
	}
	mio_collection_config_child_add(conn, child, collection_stanza);
	mio_response_free(temp_response);
	temp_response = mio_response_new();
	_mio_send_blocking(conn, collection_stanza, (mio_handler) mio_handler_error,
			temp_response);

	mio_response_free(temp_response);
	temp_response = mio_response_new();
	mio_collection_parents_query(conn, child, temp_response);
	mio_stanza_free(collection_stanza);
	collection_stanza = mio_node_config_new(conn, child, MIO_NODE_TYPE_UNKNOWN);
	if (temp_response->response_type == MIO_RESPONSE_PACKET) {
		mio_packet_t *packet = (mio_packet_t*) temp_response->response;
		mio_collection_t *coll = (mio_collection_t*) packet->payload;
		while (coll != NULL ) {
			mio_collection_config_parent_add(conn, coll->node,
					collection_stanza);
			coll = coll->next;
		}
	}
	mio_response_free(temp_response);
	mio_collection_config_parent_add(conn, parent, collection_stanza);
	_mio_send_blocking(conn, collection_stanza, (mio_handler) mio_handler_error,
			response);
	mio_stanza_free(collection_stanza);

	return MIO_OK;
}

int mio_collection_child_remove(mio_conn_t * conn, const char *child,
		const char *parent, mio_response_t *response) {

	int remove_flag = 0;
	mio_response_t *response_parent = NULL, *response_child = NULL;
	mio_stanza_t *collection_stanza_parent, *collection_stanza_child;
	mio_packet_t *packet = NULL;
	mio_collection_t *coll = NULL;
	mio_response_error_t rerr;

// Query parent and child nodes for children and parents respectively
	collection_stanza_parent = mio_stanza_new(conn);
	collection_stanza_child = mio_stanza_new(conn);
	mio_collection_children_query(conn, parent, response_parent);
	mio_collection_parents_query(conn, child, response_child);

// Parse children affiliated with parent and add to new configuration stanza
	collection_stanza_parent = mio_node_config_new(conn, parent,
			MIO_NODE_TYPE_UNKNOWN);
	if (response_parent->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t*) response_parent->response;
		coll = (mio_collection_t*) packet->payload;
		while (coll != NULL ) {
			if (strcmp(coll->node, child) != 0)
				mio_collection_config_child_add(conn, coll->node,
						collection_stanza_parent);
			else
				remove_flag++;
			coll = coll->next;
		}
	}
	if (remove_flag == 0)
		mio_error(
				"Child %s not affiliated with parent %s, check if node IDs are correct.",
				child, parent);
	else {
// If only one child was affiliated with the parent, add a blank value stanza to indicate that the parent has no children
		if (packet->num_payloads == 1)
			mio_collection_config_child_add(conn, "", collection_stanza_parent);
	}

	mio_response_free(response_parent);
	response_child = mio_response_new();
	mio_collection_parents_query(conn, child, response_child);
	collection_stanza_child = mio_node_config_new(conn, child,
			MIO_NODE_TYPE_UNKNOWN);
	if (response_child->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t*) response_child->response;
		coll = (mio_collection_t*) packet->payload;
		while (coll != NULL ) {
			if (strcmp(coll->node, parent) != 0)
				mio_collection_config_parent_add(conn, coll->node,
						collection_stanza_child);
			else
				remove_flag++;
			coll = coll->next;
		}
	}
	if (remove_flag < 2)
		mio_error(
				"Parent %s not affiliated with child %s, check if node IDs are correct",
				parent, child);
	else {
// If only one parent was affiliated with the child, add a blank value stanza to indicate that the parent has no children
		if (packet->num_payloads == 1)
			mio_collection_config_parent_add(conn, "", collection_stanza_child);

// If the child was found at the parent and vice versa, send out the new config stanzas to update the nodes
		_mio_send_blocking(conn, collection_stanza_parent,
				(mio_handler) mio_handler_error, response);
		if (response->response_type == MIO_RESPONSE_OK) {
			mio_response_free(response);
			_mio_send_blocking(conn, collection_stanza_child,
					(mio_handler) mio_handler_error, response);
		} else {
			mio_error("Error removing child from parent node:");
			mio_response_print(response);
		}
	}
	mio_response_free(response_child);
	mio_stanza_free(collection_stanza_parent);
	mio_stanza_free(collection_stanza_child);
	if (remove_flag != 2)
		return MIO_ERROR_NOT_AFFILIATED;

	return MIO_OK;
}

mio_reference_t *mio_reference_new() {
	mio_reference_t *ref = malloc(sizeof(mio_reference_t));
	memset(ref, 0, sizeof(mio_reference_t));
	return ref;
}

void mio_reference_free(mio_reference_t *ref) {
	if (ref->node != NULL )
		free(ref->node);
	if (ref->name != NULL )
		free(ref->name);
	free(ref);
}

mio_reference_t *mio_reference_tail_get(mio_reference_t *ref) {
	mio_reference_t *tail = NULL;

	while (ref != NULL ) {
		tail = ref;
		ref = ref->next;
	}
	return tail;
}

int mio_references_query(mio_conn_t *conn, char *node, mio_response_t *response) {
	return mio_item_recent_get(conn, node, response, 1, "references",
			mio_handler_references_query);
}

mio_stanza_t *mio_references_to_item(mio_conn_t* conn, mio_reference_t *ref) {
	mio_reference_t *curr;
	xmpp_stanza_t *ref_stanza, *reference_stanza;
	mio_stanza_t *item = mio_pubsub_item_new(conn, "references");

	reference_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(reference_stanza, "references");
	for (curr = ref; curr != NULL ; curr = curr->next) {
		ref_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(ref_stanza, "reference");

// Populate stanza with attributes from reference
		switch (curr->type) {
		case MIO_REFERENCE_CHILD:
			xmpp_stanza_set_attribute(ref_stanza, "type", "child");
			break;
		case MIO_REFERENCE_PARENT:
			xmpp_stanza_set_attribute(ref_stanza, "type", "parent");
			break;
		default:
			mio_stanza_free(item);
			return NULL ;
			break;
		}

		switch (curr->meta_type) {
		case MIO_META_TYPE_DEVICE:
			xmpp_stanza_set_attribute(ref_stanza, "metaType", "device");
			break;
		case MIO_META_TYPE_LOCATION:
			xmpp_stanza_set_attribute(ref_stanza, "metaType", "location");
			break;
		case MIO_META_TYPE_UKNOWN:
			xmpp_stanza_set_attribute(ref_stanza, "metaType", "unknown");
			break;
		default:
			mio_stanza_free(item);
			return NULL ;
			break;
		}

		if (curr->node != NULL )
			xmpp_stanza_set_attribute(ref_stanza, "node", curr->node);
		if (curr->name != NULL )
			xmpp_stanza_set_attribute(ref_stanza, "name", curr->name);

		xmpp_stanza_add_child(reference_stanza, ref_stanza);
		xmpp_stanza_release(ref_stanza);
	}
	xmpp_stanza_add_child(item->xmpp_stanza, reference_stanza);
	xmpp_stanza_release(reference_stanza);
	return item;
}

void mio_reference_add(mio_conn_t* conn, mio_reference_t *refs,
		mio_reference_type_t type, mio_meta_type_t meta_type, char *node) {
	mio_reference_t *curr;
	mio_response_t *query_response;
	mio_packet_t *packet;
	mio_meta_t *meta;

	if (refs->type == MIO_REFERENCE_UNKOWN)
		curr = refs;
	else {
		curr = mio_reference_tail_get(refs);
		curr->next = mio_reference_new();
		curr = curr->next;
	}
	curr->type = type;
	curr->meta_type = meta_type;
	if (node != NULL ) {
		curr->node = strdup(node);

		// FIXME: Move this code to tool
		query_response = mio_response_new();
		mio_meta_query(conn, node, query_response);
		if (query_response->response_type != MIO_RESPONSE_PACKET) {
			mio_response_free(query_response);
			return;
		}
		packet = query_response->response;
		if (packet->type != MIO_PACKET_META) {
			mio_response_free(query_response);
			return;
		}
		meta = packet->payload;
		if (meta->name != NULL )
			curr->name = strdup(meta->name);
		mio_response_free(query_response);
	}
}

mio_reference_t *mio_reference_remove(mio_conn_t* conn, mio_reference_t *refs,
		mio_reference_type_t type, char *node) {
	mio_reference_t *prev = NULL, *curr;
	int found_flag = 0;

	curr = refs;
	while (curr != NULL ) {
		if (curr->type == type && strcmp(curr->node, node) == 0) {
			found_flag = 1;
			if (prev == NULL ) {
				curr = curr->next;
				return curr;
			}
			prev->next = curr->next;
			curr->next = NULL;
			mio_reference_free(curr);
			return refs;
		}
		prev = curr;
		curr = curr->next;
	}
	if (found_flag)
		return refs;
	else
		return NULL ;
}

int mio_reference_child_remove(mio_conn_t *conn, char* parent, char *child,
		mio_response_t *response) {
	mio_response_t *query_parent = mio_response_new();
	mio_response_t *query_child, *pub_child = NULL, *pub_parent = NULL;
	mio_packet_t *packet;
	mio_reference_t *parent_refs, *child_refs;
	mio_stanza_t *child_item = NULL, *parent_item = NULL;
	mio_response_error_t *rerr;
	int err;

// Query child for current references
	query_child = mio_response_new();
	mio_references_query(conn, child, query_child);

	if (query_child->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t*) query_child->response;

		if (packet->type == MIO_PACKET_REFERENCES) {
			child_refs = packet->payload;

// Convert references to items and send them out

			if (child_refs != NULL ) {
				child_refs = mio_reference_remove(conn, child_refs,
						MIO_REFERENCE_PARENT, parent);
// If we removed all references, update the packet so that the payload isn't freed twice
				if (child_refs == NULL )
					packet->payload = NULL;
				child_item = mio_references_to_item(conn, child_refs);
				pub_child = mio_response_new();
				mio_item_publish(conn, child_item, child, pub_child);
				if (pub_child->response_type == MIO_RESPONSE_ERROR) {
					mio_error("Error publishing to child node\n");
					mio_response_print(pub_child);
				}
			}
		}
	}
// Query parent for current references
	mio_references_query(conn, parent, query_parent);

	if (query_parent->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t*) query_parent->response;

		if (packet->type == MIO_PACKET_REFERENCES) {
			parent_refs = packet->payload;

			if (parent_refs != NULL ) {
				parent_refs = mio_reference_remove(conn, parent_refs,
						MIO_REFERENCE_CHILD, child);
// If we removed all references, update the packet so that the payload isn't freed twice
				if (parent_refs == NULL )
					packet->payload = NULL;
				parent_item = mio_references_to_item(conn, parent_refs);
				pub_parent = mio_response_new();
				err = mio_item_publish(conn, parent_item, parent, pub_parent);
				if (response->response_type == MIO_RESPONSE_ERROR)
					mio_error("Error publishing to parent node\n");
			}
		}
	}

	if (pub_parent == NULL && pub_child == NULL ) {
		response->response_type = MIO_RESPONSE_ERROR;
		rerr = _mio_response_error_new();
		rerr->description = strdup("Reference not found at child or parent");
		// FIXME: Add proper error number
		rerr->err_num = 0;
		response->response = rerr;
	} else
		response->response_type = MIO_RESPONSE_OK;

// Cleanup
	mio_response_free(query_child);
	mio_response_free(query_parent);
	if (pub_parent != NULL )
		mio_response_free(pub_parent);
	if (pub_child != NULL )
		mio_response_free(pub_child);
	if (parent_item != NULL )
		mio_stanza_free(parent_item);
	if (child_item != NULL )
		mio_stanza_free(child_item);
	return err;
}

int mio_reference_child_add(mio_conn_t *conn, char* parent, char *child,
		int add_reference_at_child, mio_response_t *response) {
	mio_reference_t *curr, *parent_refs = NULL, *child_refs = NULL;
	mio_response_t *query_parent = mio_response_new();
	mio_response_t *query_child, *pub_child, *query_child_type,
			*query_parent_type;
	mio_packet_t *packet;
	mio_stanza_t *child_item, *parent_item;
// Default meta types to unknown types
	mio_meta_type_t parent_type = MIO_META_TYPE_UKNOWN;
	mio_meta_type_t child_type = MIO_META_TYPE_UKNOWN;
	mio_meta_t *meta;
	int err;

// Query node for current references
	mio_references_query(conn, parent, query_parent);

	if (query_parent->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t*) query_parent->response;

		if (packet->type == MIO_PACKET_REFERENCES && packet->num_payloads > 0) {
			parent_refs = packet->payload;
// Check if child reference already exists or if loop would be created
			for (curr = parent_refs; curr != NULL ; curr = curr->next) {
				if (strcmp(curr->node, child) == 0) {
					if (curr->type == MIO_REFERENCE_CHILD) {
						mio_response_free(query_parent);
						mio_error(
								"Child node %s already referenced from node %s\n",
								child, parent);
						return MIO_ERROR_DUPLICATE_ENTRY;
					} else if (curr->type == MIO_REFERENCE_PARENT) {
						mio_response_free(query_parent);
						mio_error(
								"Child node %s already referenced as parent from node %s\n",
								child, parent);
						return MIO_ERROR_REFERENCE_LOOP;
					}
				}
			}
		} else
			parent_refs = mio_reference_new();
	} else
		parent_refs = mio_reference_new();

// If we want to add a reference link at the chikd, first check the parent type
	if (add_reference_at_child == MIO_ADD_REFERENCE_AT_CHILD) {
		query_parent_type = mio_response_new();
		mio_meta_query(conn, parent, query_parent_type);
// If we can't check the parent type (permissions issues, etc. leave it as unknown)
		if (query_parent_type->response_type == MIO_RESPONSE_PACKET) {
			packet = (mio_packet_t*) query_parent_type->response;
			if (packet->type == MIO_PACKET_META) {
				meta = (mio_meta_t*) packet->payload;
				if (meta != NULL )
					parent_type = meta->meta_type;
			}
		}

// Check if node already referenced as parent at node or if loop would be created
		query_child = mio_response_new();
		mio_references_query(conn, child, query_child);

		if (query_child->response_type == MIO_RESPONSE_PACKET) {
			packet = (mio_packet_t*) query_child->response;

			if (packet->type == MIO_PACKET_REFERENCES
					&& packet->num_payloads > 0) {
				child_refs = (mio_reference_t*) packet->payload;

				for (curr = child_refs; curr != NULL ; curr = curr->next) {
					if (strcmp(curr->node, parent) == 0) {
						if (curr->type == MIO_REFERENCE_PARENT) {
							mio_response_free(query_parent);
							mio_response_free(query_child);
							mio_error(
									"Parent node %s already referenced as parent from node %s\n",
									parent, child);
							return MIO_ERROR_DUPLICATE_ENTRY;
						} else if (curr->type == MIO_REFERENCE_CHILD) {
							mio_response_free(query_parent);
							mio_response_free(query_child);
							mio_error(
									"Node %s already referenced as child from node %s\n",
									child, parent);
							return MIO_ERROR_REFERENCE_LOOP;
						}
					}
				}
			} else
				child_refs = mio_reference_new();
		} else
			child_refs = mio_reference_new();

		// Add parent references
		mio_reference_add(conn, child_refs, MIO_REFERENCE_PARENT, parent_type,
				parent);
		// Convert references to items and send them out
		child_item = mio_references_to_item(conn, child_refs);
		// Publish items
		pub_child = mio_response_new();
		err = mio_item_publish(conn, child_item, child, pub_child);
		// If publication failed, return error response
		if (pub_child->response_type == MIO_RESPONSE_ERROR) {
			response->response_type = MIO_RESPONSE_ERROR;
			mio_error("Error adding reference to child node %s\n", child);
		}
		mio_response_free(pub_child);
	}
// Check child type
	query_child_type = mio_response_new();
	mio_meta_query(conn, child, query_child_type);
// If we can't check the child type (permissions issues, etc. leave it as unknown)
	if (query_child_type->response_type == MIO_RESPONSE_PACKET) {
		packet = (mio_packet_t*) query_child_type->response;
		if (packet->type == MIO_PACKET_META) {
			meta = (mio_meta_t*) packet->payload;
			if (meta != NULL )
				child_type = meta->meta_type;
		}
	}
// Add child references
	mio_reference_add(conn, parent_refs, MIO_REFERENCE_CHILD, child_type,
			child);

// Convert references to items and send them out
	parent_item = mio_references_to_item(conn, parent_refs);

// If publication was successful, publish to parent
	err = mio_item_publish(conn, parent_item, parent, response);
	if (response->response_type == MIO_RESPONSE_ERROR)
		mio_error("Error adding reference to parent node %s\n", parent);
	return err;
}

/*
 int mio_node_info_query(mio_conn_t * conn, const char *node,
 mio_response_t * response) {

 int err;
 xmpp_stanza_t *configure = NULL;

 // Create stanzas
 mio_stanza_t *iq = _mio_pubsub_get_stanza_new(conn, node);
 xmpp_stanza_set_ns(iq->xmpp_stanza->children,
 "http://jabber.org/protocol/pubsub#owner");
 // Create configure stanza
 configure = xmpp_stanza_new(conn->xmpp_conn->ctx);
 xmpp_stanza_set_name(configure, "configure");
 xmpp_stanza_set_attribute(configure, "node", node);

 // Build message
 xmpp_stanza_add_child(iq->xmpp_stanza->children, configure);

 // Send out the stanza
 err = _mio_send_blocking(conn, iq,
 (mio_handler) mio_handler_collection_parents_query, response);

 mio_stanza_free(iq);

 return err;
 }*/

int mio_node_type_query(mio_conn_t * conn, const char *node,
		mio_response_t * response) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *query = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process node type query request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = _mio_pubsub_iq_get_stanza_new(conn, node);

// Create query stanza
	query = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#info");
	xmpp_stanza_set_attribute(query, "node", node);

// Build xmpp message
	xmpp_stanza_add_child(iq->xmpp_stanza, query);

// Send out the stanza
	err = _mio_send_blocking(conn, iq,
			(mio_handler) mio_handler_node_type_query, response);

// Release the stanzas
	xmpp_stanza_release(query);
	mio_stanza_free(iq);

	return err;
}

/** Delete an event node by the event node's id.
 *
 * @param conn Active MIO connection
 * @param node Event node id of node to delete.
 * @param response Response to deletion request.
 *
 * @returns SOX_OK on success, SOX_ERROR_DISCONNECTED if connection disconnected.
 * */
int mio_node_delete(mio_conn_t * conn, const char *node,
		mio_response_t * response) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *delete = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process node deletion request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_set_stanza_new(conn, node);
	xmpp_stanza_set_ns(iq->xmpp_stanza->children,
			"http://jabber.org/protocol/pubsub#owner");

// Create delete stanza
	delete = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(delete, "delete");
	xmpp_stanza_set_attribute(delete, "node", node);

// Build xmpp message
	xmpp_stanza_add_child(iq->xmpp_stanza->children, delete);

// Send out the stanza
	err = _mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
			response);

// Release the stanzas
	xmpp_stanza_release(delete);
	mio_stanza_free(iq);

	return err;
}

/** Unsubscribes a user from a event node. Can usnusbscribe
 *
 * @param conn Active MIO connection.
 * @param node Event node id to unsubscribe from.
 * @param sub_id Optional subscription id.
 * @param response Response to unsubscribe request.
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED when conn disconnected.
 * */
int mio_unsubscribe(mio_conn_t * conn, const char *node, const char *sub_id,
		mio_response_t * response) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *unsubscribe = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process unsubscribe request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_set_stanza_new(conn, node);

// Create unsubscribe stanza
	unsubscribe = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(unsubscribe, "unsubscribe");
	xmpp_stanza_set_attribute(unsubscribe, "node", node);
	xmpp_stanza_set_attribute(unsubscribe, "jid", conn->xmpp_conn->jid);

// Add subid if we got one
	if (sub_id != NULL ) {
		if ((err = xmpp_stanza_set_attribute(unsubscribe, "subid", sub_id)) < 0)
			return err;
	}

// Build xmpp message
	xmpp_stanza_add_child(iq->xmpp_stanza->children, unsubscribe);

// Send out the stanza
	err = _mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
			response);

// Release the stanzas
	xmpp_stanza_release(unsubscribe);
	mio_stanza_free(iq);

	return err;
}

/** Allows user to change passowrd for connected jid.
 *
 * @param conn Active MIO connection.
 * @param new_pass The new password to connect via the jid logged in with.
 * @param response The response to the change password request.
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on disconnect.
 * */
int mio_password_change(mio_conn_t * conn, const char *new_pass,
		mio_response_t * response) {

	xmpp_stanza_t *iq = NULL, *query = NULL, *username = NULL, *username_tag =
			NULL, *password = NULL, *password_tag = NULL;
	char *server = NULL, *id = NULL, *user = NULL;
	int user_length = 0, err;
	mio_stanza_t *stanza = mio_stanza_new(conn);

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process subscribe request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

// Extract username from JID
	user_length = strlen(conn->xmpp_conn->jid) - strlen(server) - 1;
	user = malloc((user_length + 1) * sizeof(char));
	user = strncpy(user, conn->xmpp_conn->jid, user_length);

// Create a new iq stanza
	iq = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(iq, "iq");
	xmpp_stanza_set_type(iq, "set");
	xmpp_stanza_set_attribute(iq, "to", server);
	xmpp_stanza_set_attribute(iq, "id", id);

// Create query stanza
	query = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:register");

// Create username stanza
	username = xmpp_stanza_new(conn->xmpp_conn->ctx);
	username_tag = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(username, "username");
	xmpp_stanza_set_text(username_tag, user);

// Create password stanza
	password = xmpp_stanza_new(conn->xmpp_conn->ctx);
	password_tag = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(password, "password");
	xmpp_stanza_set_text(password_tag, new_pass);

// Build xmpp message
	xmpp_stanza_add_child(password, password_tag);
	xmpp_stanza_add_child(username, username_tag);
	xmpp_stanza_add_child(query, username);
	xmpp_stanza_add_child(query, password);
	xmpp_stanza_add_child(iq, query);
	stanza->xmpp_stanza = iq;

// Release unneeded stanzas and strings
	xmpp_stanza_release(query);
	xmpp_stanza_release(username);
	xmpp_stanza_release(password);
	xmpp_stanza_release(username_tag);
	xmpp_stanza_release(password_tag);
	free(user);

// Send out the stanza
	err = _mio_send_blocking(conn, stanza, (mio_handler) mio_handler_error,
			response);

// Release the stanzas
	mio_stanza_free(stanza);

	return err;
}

/** Registers an event node.
 *
 * @param conn An active MIO connection.
 * @param new_node The node id of the new event node. Should be a UUID.
 * @param creation_node The node id of the creation node listener.
 * @param response The response to the node registration request.
 * */
int mio_node_register(mio_conn_t *conn, const char *new_node,
		const char *creation_node, mio_response_t * response) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *publish = NULL;
	xmpp_stanza_t *item = NULL;
	xmpp_stanza_t *data = NULL;
	xmpp_stanza_t *node_register = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process node registration request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_set_stanza_new(conn, creation_node);

	publish = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(publish, "publish");
	xmpp_stanza_set_attribute(publish, "node", creation_node);

// Create a new item stanzas
	item = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(item, "item");
	xmpp_stanza_set_attribute(item, "id", "current");

// Create a new data stanza
	data = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(data, "data");
	xmpp_stanza_set_ns(data, "http://jabber.org/protocol/mio#service");

// Create a new node register
	node_register = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(node_register, "nodeRegister");
	xmpp_stanza_set_attribute(node_register, "id", new_node);

// Build xmpp message
	xmpp_stanza_add_child(data, node_register);
	xmpp_stanza_add_child(item, data);
	xmpp_stanza_add_child(publish, item);
	xmpp_stanza_add_child(iq->xmpp_stanza->children, publish);

// Send out the stanza
	err = _mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
			response);

// Release the stanzas
	xmpp_stanza_release(node_register);
	xmpp_stanza_release(data);
	xmpp_stanza_release(item);
	xmpp_stanza_release(publish);
	mio_stanza_free(iq);

	return err;
}

static void _mio_geoloc_element_add_string(mio_conn_t *conn,
		xmpp_stanza_t *geoloc_stanza, char* element_name, char* element_text) {
	xmpp_stanza_t *element, *element_text_stanza;
	element = xmpp_stanza_new(conn->xmpp_conn->ctx);
	element_text_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(element, element_name);
	xmpp_stanza_set_text(element_text_stanza, element_text);
	xmpp_stanza_add_child(element, element_text_stanza);
	xmpp_stanza_add_child(geoloc_stanza, element);
	xmpp_stanza_release(element_text_stanza);
	xmpp_stanza_release(element);
}

static void _mio_geoloc_element_add_double(mio_conn_t *conn,
		xmpp_stanza_t *geoloc_stanza, char* element_name, double* element_value) {
	xmpp_stanza_t *element, *element_text_stanza;
	char text[128];
	element = xmpp_stanza_new(conn->xmpp_conn->ctx);
	element_text_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
	snprintf(text, 128, "%lf", *element_value);
	xmpp_stanza_set_name(element, element_name);
	xmpp_stanza_set_text(element_text_stanza, text);
	xmpp_stanza_add_child(element, element_text_stanza);
	xmpp_stanza_add_child(geoloc_stanza, element);
	xmpp_stanza_release(element_text_stanza);
	xmpp_stanza_release(element);
}

void mio_geoloc_merge(mio_geoloc_t *geoloc_to_update, mio_geoloc_t *geoloc) {

	if (geoloc->accuracy != NULL ) {
		if (geoloc_to_update->accuracy != NULL )
			free(geoloc_to_update->accuracy);
		geoloc_to_update->accuracy = malloc(sizeof(double));
		*geoloc_to_update->accuracy = *geoloc->accuracy;
	}
	if (geoloc->alt != NULL ) {
		if (geoloc_to_update->alt != NULL )
			free(geoloc_to_update->alt);
		geoloc_to_update->alt = malloc(sizeof(double));
		*geoloc_to_update->alt = *geoloc->alt;
	}
	if (geoloc->area != NULL ) {
		if (geoloc_to_update->area != NULL )
			free(geoloc_to_update->area);
		geoloc_to_update->area = strdup(geoloc->area);
	}
	if (geoloc->bearing != NULL ) {
		if (geoloc_to_update->bearing != NULL )
			free(geoloc_to_update->bearing);
		geoloc_to_update->bearing = malloc(sizeof(double));
		*geoloc_to_update->bearing = *geoloc->bearing;
	}
	if (geoloc->building != NULL ) {
		if (geoloc_to_update->building != NULL )
			free(geoloc_to_update->building);
		geoloc_to_update->building = strdup(geoloc->building);
	}
	if (geoloc->country != NULL ) {
		if (geoloc_to_update->country != NULL )
			free(geoloc_to_update->country);
		geoloc_to_update->country = strdup(geoloc->country);
	}
	if (geoloc->country_code != NULL ) {
		if (geoloc_to_update->country_code != NULL )
			free(geoloc_to_update->country_code);
		geoloc_to_update->country_code = strdup(geoloc->country_code);
	}
	if (geoloc->datum != NULL ) {
		if (geoloc_to_update->datum != NULL )
			free(geoloc_to_update->datum);
		geoloc_to_update->datum = strdup(geoloc->datum);
	}
	if (geoloc->description != NULL ) {
		if (geoloc_to_update->description != NULL )
			free(geoloc_to_update->description);
		geoloc_to_update->description = strdup(geoloc->description);
	}
	if (geoloc->floor != NULL ) {
		if (geoloc_to_update->floor != NULL )
			free(geoloc_to_update->floor);
		geoloc_to_update->floor = strdup(geoloc->floor);
	}
	if (geoloc->lat != NULL ) {
		if (geoloc_to_update->lat != NULL )
			free(geoloc_to_update->lat);
		geoloc_to_update->lat = malloc(sizeof(double));
		*geoloc_to_update->lat = *geoloc->lat;
	}
	if (geoloc->locality != NULL ) {
		if (geoloc_to_update->locality != NULL )
			free(geoloc_to_update->locality);
		geoloc_to_update->locality = strdup(geoloc->locality);
	}
	if (geoloc->lon != NULL ) {
		if (geoloc_to_update->lon != NULL )
			free(geoloc_to_update->lon);
		geoloc_to_update->lon = malloc(sizeof(double));
		*geoloc_to_update->lon = *geoloc->lon;
	}
	if (geoloc->postal_code != NULL ) {
		if (geoloc_to_update->postal_code != NULL )
			free(geoloc_to_update->postal_code);
		geoloc_to_update->postal_code = strdup(geoloc->postal_code);
	}
	if (geoloc->region != NULL ) {
		if (geoloc_to_update->region != NULL )
			free(geoloc_to_update->region);
		geoloc_to_update->region = strdup(geoloc->region);
	}
	if (geoloc->room != NULL ) {
		if (geoloc_to_update->room != NULL )
			free(geoloc_to_update->room);
		geoloc_to_update->room = strdup(geoloc->room);
	}
	if (geoloc->speed != NULL ) {
		if (geoloc_to_update->speed != NULL )
			free(geoloc_to_update->speed);
		geoloc_to_update->speed = malloc(sizeof(double));
		*geoloc_to_update->speed = *geoloc->speed;
	}
	if (geoloc->street != NULL ) {
		if (geoloc_to_update->street != NULL )
			free(geoloc_to_update->street);
		geoloc_to_update->street = strdup(geoloc->street);
	}
	if (geoloc->text != NULL ) {
		if (geoloc_to_update->text != NULL )
			free(geoloc_to_update->text);
		geoloc_to_update->text = strdup(geoloc->text);
	}
	if (geoloc->tzo != NULL ) {
		if (geoloc_to_update->tzo != NULL )
			free(geoloc_to_update->tzo);
		geoloc_to_update->tzo = strdup(geoloc->tzo);
	}
	if (geoloc->timestamp != NULL ) {
		if (geoloc_to_update->timestamp != NULL )
			free(geoloc_to_update->timestamp);
		geoloc_to_update->timestamp = strdup(geoloc->timestamp);
	}
	if (geoloc->uri != NULL ) {
		if (geoloc_to_update->uri != NULL )
			free(geoloc_to_update->uri);
		geoloc_to_update->uri = strdup(geoloc->uri);
	}
}

/** Converts a mio_geloc_t struct into an XMPP stanza.
 *
 * @param conn Active MIO connection
 * @param geoloc Initialized geolocation structure.
 *
 * @returns Stanza with geolocation information.
 * */
mio_stanza_t *mio_geoloc_to_stanza(mio_conn_t* conn, mio_geoloc_t *geoloc) {
	xmpp_stanza_t *geoloc_stanza;
	mio_stanza_t *ret = mio_stanza_new(conn);

	geoloc_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(geoloc_stanza, "geoloc");
	xmpp_stanza_set_ns(geoloc_stanza, "http://jabber.org/protocol/geoloc");
	xmpp_stanza_set_attribute(geoloc_stanza, "xml:lang", "en");

	if (geoloc->accuracy != NULL )
		_mio_geoloc_element_add_double(conn, geoloc_stanza, "accuracy",
				geoloc->accuracy);
	if (geoloc->alt != NULL )
		_mio_geoloc_element_add_double(conn, geoloc_stanza, "alt",
				geoloc->accuracy);
	if (geoloc->area != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "area",
				geoloc->area);
	if (geoloc->bearing != NULL )
		_mio_geoloc_element_add_double(conn, geoloc_stanza, "bearing",
				geoloc->bearing);
	if (geoloc->building != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "building",
				geoloc->building);
	if (geoloc->country != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "country",
				geoloc->country);
	if (geoloc->country_code != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "countrycode",
				geoloc->country_code);
	if (geoloc->datum != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "datum",
				geoloc->datum);
	if (geoloc->description != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "description",
				geoloc->description);
	if (geoloc->floor != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "floor",
				geoloc->floor);
	if (geoloc->lat != NULL )
		_mio_geoloc_element_add_double(conn, geoloc_stanza, "lat", geoloc->lat);
	if (geoloc->locality != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "locality",
				geoloc->locality);
	if (geoloc->lon != NULL )
		_mio_geoloc_element_add_double(conn, geoloc_stanza, "lon", geoloc->lon);
	if (geoloc->postal_code != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "postalcode",
				geoloc->postal_code);
	if (geoloc->region != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "region",
				geoloc->region);
	if (geoloc->room != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "room",
				geoloc->room);
	if (geoloc->speed != NULL )
		_mio_geoloc_element_add_double(conn, geoloc_stanza, "speed",
				geoloc->speed);
	if (geoloc->street != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "street",
				geoloc->street);
	if (geoloc->text != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "text",
				geoloc->text);
	if (geoloc->timestamp != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "timestamp",
				geoloc->timestamp);
	if (geoloc->tzo != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "tzo", geoloc->tzo);
	if (geoloc->uri != NULL )
		_mio_geoloc_element_add_string(conn, geoloc_stanza, "uri", geoloc->uri);

	ret->xmpp_stanza = geoloc_stanza;
	return ret;
}

/** Converts a mio_meta_t struct into a mio stanza.
 *
 * @param conn Active MIO connection.
 * @param meta An initialized meta struct to be parsed into a stanza.
 *
 * @returns Stanza with meta information contained within.
 * */
mio_stanza_t *mio_meta_to_item(mio_conn_t* conn, mio_meta_t *meta) {
	xmpp_stanza_t *meta_stanza = NULL, *transducer_stanza = NULL,
			*property_stanza = NULL, *t_property_stanza = NULL, *enum_stanza =
					NULL;
	mio_transducer_meta_t *transducer;
	mio_enum_map_meta_t *enum_map;
	mio_property_meta_t *property, *t_property;
	mio_stanza_t *geoloc, *item;

	item = mio_pubsub_item_new(conn, "meta");

	meta_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(meta_stanza, "meta");
	xmpp_stanza_set_ns(meta_stanza, "http://jabber.org/protocol/mio");

	switch (meta->meta_type) {
	case MIO_META_TYPE_DEVICE:
		xmpp_stanza_set_attribute(meta_stanza, "type", "device");
		break;
	case MIO_META_TYPE_LOCATION:
		xmpp_stanza_set_attribute(meta_stanza, "type", "location");
		break;
	default:
		xmpp_stanza_release(meta_stanza);
		mio_error("Cannot generate item from unknown meta type\n");
		return NULL ;
	}
	if (meta->timestamp != NULL )
		xmpp_stanza_set_attribute(meta_stanza, "timestamp", meta->timestamp);
	if (meta->info != NULL )
		xmpp_stanza_set_attribute(meta_stanza, "info", meta->info);
	if (meta->geoloc != NULL ) {
		geoloc = mio_geoloc_to_stanza(conn, meta->geoloc);
		xmpp_stanza_add_child(meta_stanza, geoloc->xmpp_stanza);
		mio_stanza_free(geoloc);
	}
	if (meta->name != NULL )
		xmpp_stanza_set_attribute(meta_stanza, "name", meta->name);

	for (transducer = meta->transducers; transducer != NULL ; transducer =
			transducer->next) {
		transducer_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(transducer_stanza, "transducer");

		if (transducer->accuracy != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "accuracy",
					transducer->accuracy);
		if (transducer->geoloc != NULL ) {
			geoloc = mio_geoloc_to_stanza(conn, transducer->geoloc);
			xmpp_stanza_add_child(transducer_stanza, geoloc->xmpp_stanza);
			mio_stanza_free(geoloc);
		}
		if (transducer->interface != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "interface",
					transducer->interface);
    	if (transducer->manufacturer != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "manufacturer",
					transducer->interface);
		if (transducer->manufacturer != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "manufacturer",
					transducer->manufacturer);
		if (transducer->max_value != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "maxValue",
					transducer->max_value);
		if (transducer->min_value != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "minValue",
					transducer->min_value);
		if (transducer->name != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "name",
					transducer->name);
		if (transducer->precision != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "precision",
					transducer->precision);
		if (transducer->resolution != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "resolution",
					transducer->resolution);
		if (transducer->serial != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "serial",
					transducer->serial);
		if (transducer->type != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "type",
					transducer->type);
		if (transducer->unit != NULL )
			xmpp_stanza_set_attribute(transducer_stanza, "unit",
					transducer->unit);
		if (transducer->enumeration != NULL ) {
			for (enum_map = transducer->enumeration; enum_map != NULL ;
					enum_map = enum_map->next) {
				enum_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
				xmpp_stanza_set_name(enum_stanza, "map");
				if (enum_map->name != NULL )
					xmpp_stanza_set_attribute(enum_stanza, "name",
							enum_map->name);
				if (enum_map->value != NULL )
					xmpp_stanza_set_attribute(enum_stanza, "value",
							enum_map->value);
				xmpp_stanza_add_child(transducer_stanza, enum_stanza);
				xmpp_stanza_release(enum_stanza);
			}
		}
		if (transducer->properties != NULL ) {
			for (t_property = transducer->properties; t_property != NULL ;
					t_property = t_property->next) {
				t_property_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
				xmpp_stanza_set_name(t_property_stanza, "property");
				if (t_property->name != NULL )
					xmpp_stanza_set_attribute(t_property_stanza, "name",
							t_property->name);
				if (t_property->value != NULL )
					xmpp_stanza_set_attribute(t_property_stanza, "value",
							t_property->value);
				xmpp_stanza_add_child(transducer_stanza, t_property_stanza);
				xmpp_stanza_release(t_property_stanza);
			}
		}

		xmpp_stanza_add_child(meta_stanza, transducer_stanza);
		xmpp_stanza_release(transducer_stanza);
	}

	for (property = meta->properties; property != NULL ;
			property = property->next) {

		if (property->name != NULL && property->value != NULL ) {
			property_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
			xmpp_stanza_set_name(property_stanza, "property");
			xmpp_stanza_set_attribute(property_stanza, "name", property->name);
			xmpp_stanza_set_attribute(property_stanza, "value",
					property->value);
			xmpp_stanza_add_child(meta_stanza, property_stanza);
			xmpp_stanza_release(property_stanza);
		}
	}
	xmpp_stanza_add_child(item->xmpp_stanza, meta_stanza);
	return item;
}

/** Gets the meta information of an event node.
 *
 * @param conn Active MIO connection.
 * @param node Node id of node to query for meta.
 * @param response Pointer to response struct to store response in.
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on disconnection.
 * */
int mio_meta_query(mio_conn_t* conn, const char *node, mio_response_t *response) {
	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *items = NULL, *item = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process meta query request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_get_stanza_new(conn, node);

// Create items stanza
	items = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(items, "items");
	xmpp_stanza_set_attribute(items, "node", node);

// Create item stanza
	item = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(item, "item");
	xmpp_stanza_set_attribute(item, "id", "meta");

// Build xmpp message
	xmpp_stanza_add_child(items, item);
	xmpp_stanza_add_child(iq->xmpp_stanza->children, items);

// Send out the stanza
	err = _mio_send_blocking(conn, iq, (mio_handler) mio_handler_meta_query,
			response);

// Release the stanzas
	xmpp_stanza_release(items);
	xmpp_stanza_release(item);
	mio_stanza_free(iq);

	return err;
}

/** Publishes meta information to an event node.
 *
 * @param conn Active MIO connection.
 * @param node Event node id to publish meta information to.
 * @param meta The meta information structure for the event node.
 * @param response Pointer to the response received in acknowledgement of the publish request.
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNEDTED on disconnection.
 * */
int mio_meta_publish(mio_conn_t* conn, const char *node, mio_meta_t *meta,
		mio_response_t *response) {

	mio_stanza_t *iq = NULL, *meta_stanza = NULL;
	xmpp_stanza_t *item = NULL;
	int err;

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process meta publish request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_get_stanza_new(conn, node);
	meta_stanza = mio_meta_to_item(conn, meta);

	item = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(item, "item");
	xmpp_stanza_set_id(item, "meta");

	xmpp_stanza_add_child(item, meta_stanza->xmpp_stanza);
	xmpp_stanza_add_child(iq->xmpp_stanza->children, item);

	err = _mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
			response);

	xmpp_stanza_release(item);
	mio_stanza_free(meta_stanza);
	mio_stanza_free(iq);

	return err;
}

/** Gets n of the most recent published items.
 *
 * @param conn
 * @param node
 * @param response
 * @param max_items
 * @param item_id
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on disconnect.
 * */
int mio_item_recent_get(mio_conn_t* conn, const char *node,
		mio_response_t * response, int max_items, const char *item_id,
		mio_handler *handler) {

	mio_stanza_t *iq = NULL;
	xmpp_stanza_t *items = NULL;
	xmpp_stanza_t *item = NULL;
	int err;
	char max_items_s[16];

// Check if connection is active
	if (!conn->xmpp_conn->authenticated) {
		mio_error(
				"Cannot process recent item get request since not connected to XMPP server");
		return MIO_ERROR_DISCONNECTED;
	}

	iq = mio_pubsub_get_stanza_new(conn, node);

// Create items stanza
	items = xmpp_stanza_new(conn->xmpp_conn->ctx);
	xmpp_stanza_set_name(items, "items");
	xmpp_stanza_set_attribute(items, "node", node);

// If an item id is specified, attach it to the stanza, otherwise retrieve up to max_items most recent items
	if (item_id != NULL ) {
		item = xmpp_stanza_new(conn->xmpp_conn->ctx);
		xmpp_stanza_set_name(item, "item");
		xmpp_stanza_set_id(item, item_id);
		xmpp_stanza_add_child(items, item);
	} else {
		if (max_items != 0) {
			snprintf(max_items_s, 16, "%u", max_items);
			xmpp_stanza_set_attribute(items, "max_items", max_items_s);
		}
	}

// Build xmpp message
	xmpp_stanza_add_child(iq->xmpp_stanza->children, items);

// If a handler is specified use it, otherwise use default handler
	if (handler == NULL )
// Send out the stanza
		err = _mio_send_blocking(conn, iq,
				(mio_handler) mio_handler_item_recent_get, response);
	else
		err = _mio_send_blocking(conn, iq, (mio_handler) handler, response);

// Release the stanzas
	xmpp_stanza_release(items);
	mio_stanza_free(iq);

	return err;
}

/*
 int sox_node_register(sox_conn_t *conn, const char *new_node,
 const char *creation_node, sox_userdata_t * const userdata) {

 xmpp_stanza_t *iq = NULL;
 xmpp_stanza_t *pubsub = NULL;
 xmpp_stanza_t *publish = NULL;
 xmpp_stanza_t *item = NULL;
 xmpp_stanza_t *data = NULL;
 xmpp_stanza_t *node_register = NULL;

 char *pubsubserv;
 char *id;
 int err;

 // String length is length of server address + length of "pubsub." + terminating character
 pubsubserv = (char *) malloc(
 (strlen(sox_get_server(conn->xmpp_conn->jid)) + 9) * sizeof(char));
 if (pubsubserv == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }
 strcpy(pubsubserv, "pubsub.");
 strcat(pubsubserv, sox_get_server(conn->xmpp_conn->jid));

 // Create id for iq stanza consisting of "nodereg_" and node name
 // String length is length of server address + length of "nodereg_" + terminating character
 id = (char *) malloc((strlen(new_node) + 9) * sizeof(char));
 if (id == NULL ) {
 fprintf(stderr, "ERROR: Memory allocation error!\n");
 return -1;
 }
 strcpy(id, "nodereg_");
 strcat(id, new_node);
 // Add ID handler for error checking response
 sox_handler_id_add(conn, sox_handler_node_register, id, userdata);

 // Create a new iq stanza
 iq = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(iq, "iq")) < 0)
 return err;
 if ((err = xmpp_stanza_set_type(iq, "set")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "to", pubsubserv)) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(iq, "id", id)) < 0)
 return err;

 // Create a new pubsub stanza
 pubsub = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(pubsub, "pubsub")) < 0)
 return err;
 if ((err = xmpp_stanza_set_ns(pubsub, "http://jabber.org/protocol/pubsub"))
 < 0)
 return err;

 publish = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(publish, "publish")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(publish, "node", creation_node)) < 0)
 return err;

 // Create a new item stanzas
 item = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(item, "item")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(item, "id", "current")) < 0)
 return err;

 // Create a new data stanza
 data = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(data, "data")) < 0)
 return err;
 if ((err = xmpp_stanza_set_ns(data,
 "http://jabber.org/protocol/sox#service")) < 0)
 return err;

 // Create a new node register
 node_register = xmpp_stanza_new(conn->xmpp_conn->ctx);
 if ((err = xmpp_stanza_set_name(node_register, "nodeRegister")) < 0)
 return err;
 if ((err = xmpp_stanza_set_attribute(node_register, "id", new_node)) < 0)
 return err;

 // Build xmpp message
 if ((err = xmpp_stanza_add_child(data, node_register)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(item, data)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(publish, item)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(pubsub, publish)) < 0)
 return err;
 if ((err = xmpp_stanza_add_child(iq, pubsub)) < 0)
 return err;

 // Release unneeded stanzas
 xmpp_stanza_release(node_register);
 xmpp_stanza_release(data);
 xmpp_stanza_release(item);
 xmpp_stanza_release(publish);
 xmpp_stanza_release(pubsub);

 // Send out the stanza
 xmpp_send(conn->xmpp_conn, iq);

 // Release the stanza
 xmpp_stanza_release(iq);

 return 0;
 }
 */
