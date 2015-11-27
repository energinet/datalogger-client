/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2012 LIAB ApS <info@liab.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <strophe.h>
#include <stdbool.h>
#include <signal.h>
#include <syslog.h>

#include "sdvp.h"
#include "sdvp_helper.h"
#include "sdvp_rpchelper.h"
#include "sdvp_register.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

//// DEFINES ////

#define PACKET_ID_LENGTH_INC_TERM 7
#define RESPONSE_TIME_TEXT_LEN 60
#define KEEP_THIS_HANDLER_ACTIVE 1
#define IDLE_COUNT_MAX 1000*60*2 //More than two minutes 
#define MAX_PINGS 2

//// VARIABLES ////

static const char* sdvpServerUsername = "ts1";
//static const char* sdvpServerGroup = "ServiceProvider";
static char* sdvpServerFullJid = NULL;

static xmpp_ctx_t *ctx;
static unsigned char sdvpIsInitialised = false;
static unsigned char sdvpIsRunning = false;

static unsigned int sdvpIdleCounter = 0;
static unsigned char sdvpPingsSent = 0;

//static sdvp_reply_params_t* (*callback)(sdvp_method_t, sdvp_from_t*,
//		sdvp_params_t*);

static void (*callbackOnConnectionChange)(sdvp_conn_event_t);

static bool debugIsEnabled = false;

//// CODE: PRIVATE ////

static void sdvpSendPing( xmpp_conn_t * const conn, xmpp_ctx_t * const ctx) {

	xmpp_stanza_t *iq,*ping;
	
	iq = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(iq, "iq");
	xmpp_stanza_set_type(iq, "get");
	xmpp_stanza_set_id(iq, "ping");

	ping = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(ping, "ping");
	xmpp_stanza_set_ns(ping, "urn:xmpp:ping");
	xmpp_stanza_add_child(iq,ping);
	xmpp_stanza_release(ping);

	xmpp_send(conn,iq);
	xmpp_stanza_release(iq);

	sdvpIdleCounter = 0;
	sdvpPingsSent++;

}

static int HandlePingResult(xmpp_conn_t *const conn, 
							xmpp_stanza_t *const stanza, 
							void *const userdata) {

	//All is ok
	sdvpIdleCounter = 0;
	sdvpPingsSent = 0;

	return KEEP_THIS_HANDLER_ACTIVE;
}

/**
 * Get the method type from the method string
 */
sdvp_method_t _GetDefinedMetodeFromMethod(const char* method) {
	sdvp_method_t methodType = SDVP_METHOD_UNDEFINED;
	if (method != NULL ) {
/*		if (strcmp("61850.read", method) == 0) {
			methodType = SDVP_METHOD_61850_READ;
		} else if (strcmp("61850.write", method) == 0) {
			methodType = SDVP_METHOD_61850_WRITE;
		} else if (strcmp("61850.list", method) == 0) {
			methodType = SDVP_METHOD_61850_LIST;
		} else if (strcmp("direct.read", method) == 0) {
			methodType = SDVP_METHOD_DIRECT_READ;
		} else if (strcmp("direct.write", method) == 0) {
			methodType = SDVP_METHOD_DIRECT_WRITE;
		} else if (strcmp("direct.list", method) == 0) {
			methodType = SDVP_METHOD_DIRECT_LIST;
		} else if (strcmp("liab.list", method) == 0) {
			methodType = SDVP_METHOD_LIAB_LIST;
        } else*/ 
		
		if (strcmp("Read", method) == 0) {
            methodType = IEC_READ;
        } else if (strcmp("Write", method) == 0) {
            methodType = IEC_WRITE;
		} else if (strcmp("GetNameList", method) == 0) {
            methodType = IEC_GET_NAME_LIST;
        } else if (strcmp("GetVariableAccessAttributes", method) == 0) {
            methodType = IEC_GET_VARIABLE_ACCESS_ATTRIBUTE;
        } else if (strcmp("Watchdog", method) == 0) {
			methodType = LIAB_WATCHDOG;
		}
	}
	return methodType;
}

/**
 * Handles the service
 */
xmpp_stanza_t * _HandleServiceRequest(xmpp_conn_t * const conn,
		xmpp_stanza_t * const stanza, void * const userdata) {
	xmpp_stanza_t *reply;
	xmpp_stanza_t *wd, *nodetext;
	xmpp_ctx_t *ctx = (xmpp_ctx_t*) userdata;
	sdvp_method_t methodType = SDVP_METHOD_UNDEFINED;
	sdvp_from_t* from;
	char *method;
	sdvp_reply_params_t* sdvpReplyParams = NULL;
    void* replyParams;
	void* paramStructure = NULL;
	char strbuf[20];
	static unsigned int watchdog = 1;

	//HJP: Hvorfor oprette reply her, når den næsten altid bliver overskrevet?
	reply = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(reply, "iq");

	from = _CreateFrom(xmpp_stanza_get_attribute(stanza, "from"), "TODO",
			"TODO");
	syslog(LOG_DEBUG, "Received a RPC Method call from %s\n", from->name);

	method = _GetRpcMethode(stanza);
	methodType = _GetDefinedMetodeFromMethod(method);

	syslog(LOG_DEBUG, "XML-RPC method name %s\n", method);

	free(method);

	if (methodType == SDVP_METHOD_UNDEFINED) {
		syslog(LOG_DEBUG, "Undefined method type\n");

		sdvp_InitiateReplyParameters(&sdvpReplyParams, 1);
		sdvpReplyParams->params[0].strValue =
				strdup("ERROR: Method is undefined. Methods are CasE SentItive.\n");
		sdvpReplyParams->params[0].type = IEC_VISIBLE_STRING;
		//HJP: Jeg releaser reply her, det ville være smartere kun at oprette den når nødvendigt...
		xmpp_stanza_release(reply);
        reply = _CreateReply(ctx, methodType, sdvpReplyParams);
		xmpp_stanza_set_type(reply, "error");
        sdvp_FreeReplyParameters(sdvpReplyParams);
	} else if (methodType == LIAB_WATCHDOG) {
		xmpp_stanza_release(reply);
		reply = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(reply, "query");
		xmpp_stanza_set_ns(reply, "jabber:iq:rpc");

		wd = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(wd, "watchdog");
		xmpp_stanza_add_child(reply, wd);
		xmpp_stanza_release(wd);

		nodetext = xmpp_stanza_new(ctx);
		sprintf(strbuf, "%u", watchdog++);
		xmpp_stanza_set_text(nodetext, strbuf);
		xmpp_stanza_add_child(wd, nodetext);
		xmpp_stanza_release(nodetext);

	} else{
		if (_CallbackIsRegistered(methodType)) {
			paramStructure = _CreateParamStructureFromParameters(methodType, stanza);
			replyParams = _CallTheRightCallback(methodType, paramStructure);
			_FreeParamStructure(methodType,paramStructure);
			xmpp_stanza_release(reply);
            reply = _CreateReply(ctx, methodType, replyParams);

			sdvp_FreeCallbackReply(methodType, replyParams);
        	sdvp_FreeReplyParameters(sdvpReplyParams);
		} else {
			// TODO: Send a error with method not implemented or alike
		}
	}

	_FreeFrom(from);


	return reply;
}

/**
 * Checks content of stanza to be valid XML-RPC format
 */
int _CheckRpcFormat(xmpp_stanza_t * const stanza) {
	int rv = 0;
	// TODO: Implement check!
	return rv;
}

/**
 * This is called when someone sends a RPC method call
 * Checks for XML-RPC format validation and sends error if so otherwise it calls the
 * HandleServiceCall for finding the right service and creating the right structures
 * @return KEEP_THIS_HANDLER_ACTIVE
 */
int HandleRpcCall(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
		void * const userdata) {
	xmpp_stanza_t *reply;
	xmpp_stanza_t *xmlRpcReply;
	xmpp_ctx_t *ctx = (xmpp_ctx_t*) userdata;
	sdvp_from_t* from;
	sdvp_reply_params_t* replyParams = NULL;
	int formatInvalid;

	sdvpIdleCounter = 0;
	sdvpPingsSent = 0;

	reply = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(reply, "iq");

	// TODO: Get the Group and get the jid
	from = _CreateFrom(xmpp_stanza_get_attribute(stanza, "from"), "TODO",
			"TODO");

	syslog(LOG_DEBUG, "Received a RPC Method call from %s\n", from->name);

	formatInvalid = _CheckRpcFormat(stanza);
	if (formatInvalid) {
	    // FIXME: Something here fails!
		syslog(LOG_WARNING, "Error in XML-RPC format\n");
		sdvp_InitiateReplyParameters(&replyParams, 1);
		replyParams->params[0].strValue = strdup("Error in XML-RPC format\n");
		replyParams->params[0].type = IEC_VISIBLE_STRING;
		xmpp_stanza_set_type(reply, "error");
		// TODO: Create a type

		//HJP var her!
		xmlRpcReply = _CreateReply(ctx, SDVP_METHOD_UNDEFINED ,replyParams);
		xmpp_stanza_add_child(reply, xmlRpcReply);
		//HJP: stanza_add_child laver en kopi, så der skal releases
		xmpp_stanza_release(xmlRpcReply);
		sdvp_FreeReplyParameters(replyParams);
	} else {
		// The reply from this function should be ready to send
		xmlRpcReply = _HandleServiceRequest(conn, stanza, userdata);
		xmpp_stanza_add_child(reply, xmlRpcReply);
		//HJP: stanza_add_child laver en kopi, så der skal releases
		xmpp_stanza_release(xmlRpcReply);
	}

	xmpp_stanza_set_id(reply, xmpp_stanza_get_id(stanza));
	xmpp_stanza_set_attribute(reply, "to",
			xmpp_stanza_get_attribute(stanza, "from"));
	xmpp_stanza_set_type(reply, "result");

	xmpp_send(conn, reply);
	xmpp_stanza_release(reply); // Frees the stanza and all of its children

	_FreeFrom(from);

	return KEEP_THIS_HANDLER_ACTIVE;
}

/**
 * This handles the response from the XMPP server when a request for roster is
 * send.
 * @return 1 which keeps this handler in the handler list
 */
//TODO: Create structure for holding the roster
static int _HandleRosterReply(xmpp_conn_t * const conn,
		xmpp_stanza_t * const stanza, void * const userdata) {
	xmpp_stanza_t *query, *item, *group;
	char *type, *name, *jid, *sub, groups[200], *groupName;
	char *g;
	int r_count = 0;

	sdvpIdleCounter = 0;
	sdvpPingsSent = 0;

	type = xmpp_stanza_get_type(stanza);
	if (strcmp(type, "error") == 0) {
		syslog(LOG_WARNING, "ERROR: query failed\n");
	} else {
		query = xmpp_stanza_get_child_by_name(stanza, "query");
		syslog(LOG_INFO, "Roster (Only the first group is shown!):\n----------\n");
		for (item = xmpp_stanza_get_children(query); item; item =
				xmpp_stanza_get_next(item)) {
			name = xmpp_stanza_get_attribute(item, "name");
			jid = xmpp_stanza_get_attribute(item, "jid");
			sub = xmpp_stanza_get_attribute(item, "subscription");

			// Get groups
			//
			strcpy(groups, "-");
			for (group = xmpp_stanza_get_children(item); group; group =
					xmpp_stanza_get_next(group)) {
				groupName = xmpp_stanza_get_name(group);
				if (strcmp(groupName, "group") == 0) {
				    g = xmpp_stanza_get_text(group);
					strcpy(groups, g);
					free(g);
				}
			}

			syslog(LOG_INFO, "#%d\tName=%s\n\tJID=%s\n\tsubscription=%s\n\tGroups=%s\n\n",
					r_count, name, jid, sub, groups);
			r_count++;
		}
		syslog(LOG_INFO, "----------\n");
	}
	return 1;
}

static int _SendRosterRequest(xmpp_conn_t * const conn, xmpp_ctx_t *ctx) {
	xmpp_stanza_t *roaster, *roaster_qry;
	char packetId[PACKET_ID_LENGTH_INC_TERM];
	_GeneratePacketId(packetId, PACKET_ID_LENGTH_INC_TERM);
	roaster = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(roaster, "iq");
	xmpp_stanza_set_type(roaster, "get");
	xmpp_stanza_set_id(roaster, packetId);
	roaster_qry = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(roaster_qry, "query");
	xmpp_stanza_set_ns(roaster_qry, XMPP_NS_ROSTER);
	xmpp_stanza_add_child(roaster, roaster_qry);
// we can release the stanza since it belongs to sr_iq now
	xmpp_stanza_release(roaster_qry);
// set up reply handler
	xmpp_handler_add(conn, _HandleRosterReply, "jabber:iq:roster", NULL, NULL,
			ctx);
	xmpp_send(conn, roaster);
	xmpp_stanza_release(roaster);

	return 0;
}

/**
 * This handles a status response iq be printing it out
 */
int PresenceHandler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
		void * const userdata) {
//    xmpp_stanza_t *reply;
	char *type;
//    char *id;
	char *sender;
	xmpp_ctx_t *ctx = (xmpp_ctx_t *) userdata;

	sdvpIdleCounter = 0;
	sdvpPingsSent = 0;

// type is NULL if initial presence is received
	type = xmpp_stanza_get_type(stanza);
	if (type != NULL ) {
		if (strcmp(type, "error") == 0) {
			syslog(LOG_WARNING, "ERROR: Received presence with type of error\n");
		} else {
//            id = xmpp_stanza_get_id(stanza);
			sender = xmpp_stanza_get_attribute(stanza, "from");

//            reply = xmpp_stanza_new(ctx);
//            xmpp_stanza_set_name(reply, "presence");
//            xmpp_stanza_set_type(reply, "result");
//            if (id != NULL ) {
//                xmpp_stanza_set_id(reply, id);
//            }
//            xmpp_stanza_set_attribute(reply, "to", sender);
//            xmpp_send(conn, reply);
//            xmpp_stanza_release(reply);
		}
	} else {
//        id = xmpp_stanza_get_id(stanza);
		sender = strdup(xmpp_stanza_get_attribute(stanza, "from"));

		// Check if the presence is from the server user
		char *senderUsername;
		senderUsername = GetUsernameFromJid(sender);
		if (strcmp(sdvpServerUsername, senderUsername) == 0) {
			sdvpServerFullJid = strdup(sender);
			// Send status-request
			_SendRpcCall(conn, ctx, sdvpServerFullJid);
		}
		free(sender);

		// Dont send reply as the server should do this automatically
		// to all connected users on the roster
//        reply = xmpp_stanza_new(ctx);
//        xmpp_stanza_set_name(reply, "presence");
//        xmpp_stanza_set_type(reply, "result");
//        if (id != NULL ) {
//            xmpp_stanza_set_id(reply, id);
//        }
//        xmpp_stanza_set_attribute(reply, "to", sender);
//        xmpp_send(conn, reply);
//        xmpp_stanza_release(reply);

	}
	return 1;
}

/**
 * Ping handler
 * Upon receiving a ping from the server this handler sends a "pong" back
 * The "pong" is an empty iq-result.
 */
int HandlePing(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
		void * const userdata) {
	xmpp_stanza_t *pong;
	char *id;
	char *sender;
	xmpp_ctx_t *ctx = (xmpp_ctx_t *) userdata;

	sdvpIdleCounter = 0;
	sdvpPingsSent = 0;

	id = xmpp_stanza_get_id(stanza);
	sender = xmpp_stanza_get_attribute(stanza, "from");

	pong = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(pong, "iq");
	xmpp_stanza_set_type(pong, "result");
	xmpp_stanza_set_id(pong, id);
	xmpp_stanza_set_attribute(pong, "to", sender);
//    xmpp_stanza_set_attribute(pong, "from", "foxg20@einsteinium");
	xmpp_send(conn, pong);
	xmpp_stanza_release(pong);

	return 1; // Returning 0 = Handler is deleted (Single shot), 1 = Handler is kept
}

/**
 * This connection handler is called after connection has been etablized.
 * All initial messages can therefore be put here.
 */
static void sdvp_HandleConnection(xmpp_conn_t * const conn,
		const xmpp_conn_event_t status, const int error,
		xmpp_stream_error_t * const stream_error, void * const userdata) {
	xmpp_ctx_t *ctx = (xmpp_ctx_t *) userdata;
	xmpp_stanza_t* pres;
	char packetId[PACKET_ID_LENGTH_INC_TERM];
	if (status == XMPP_CONN_CONNECT) {
		syslog(LOG_DEBUG, "Connected\n");
		xmpp_handler_add(conn, HandleRpcCall, "jabber:iq:rpc", "iq", "get",
				ctx);
		xmpp_handler_add(conn, PresenceHandler, NULL, "presence", NULL, ctx);
		xmpp_handler_add(conn, HandlePing, "urn:xmpp:ping", "iq", "get", ctx);
		xmpp_id_handler_add(conn, HandlePingResult, "ping", ctx);

		// Send initial <presence/> so that we appear online to contacts
		// This is very important as the server don't route messages/iq's if not online!
		//
		pres = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(pres, "presence");
		_GeneratePacketId(packetId, PACKET_ID_LENGTH_INC_TERM);
		xmpp_stanza_set_id(pres, packetId);
		xmpp_send(conn, pres);
		xmpp_stanza_release(pres);

		// Send request for roaster
		_SendRosterRequest(conn, ctx);
		if (callbackOnConnectionChange != NULL ) {
			callbackOnConnectionChange(SDVP_CONN_CONNECT);
		}
	} else {
		syslog(LOG_WARNING, "conn_handler disconnected\n");
		sdvpIsRunning = false;
		xmpp_stop(ctx);
	}
}

int sdvp_ConnectionCreate(sdvp_config_t *sdvpConfig) {
	xmpp_conn_t *conn;

#ifdef MEMWATCH
	/* memwatch setup */
    mwStatistics( 2 );
    TRACE("Watching libsdvp!\n");
#endif

	if (sdvpConfig->debug == SDVP_DEBUG_ENABLED) {
		debugIsEnabled = true;
	}
	if (!sdvpIsInitialised) {
		if (debugIsEnabled) {
			sdvp_InitialiseWithDebug();
		} else {
			sdvp_Initialise();
		}
	}

// create a connection
	conn = xmpp_conn_new(ctx);
	xmpp_conn_set_jid(conn, sdvpConfig->user);
	xmpp_conn_set_pass(conn, sdvpConfig->password);
	xmpp_connect_client(conn, sdvpConfig->host, 0, sdvp_HandleConnection, ctx);
//	callback = sdvpConfig->callback;
	callbackOnConnectionChange = sdvpConfig->callbackOnConnectionChange;

	//Event loop
	sdvpIsRunning = true;
	while(sdvpIsRunning) {
		xmpp_run_once(ctx,1);

		if (sdvpIdleCounter > IDLE_COUNT_MAX)
			sdvpSendPing(conn, ctx);

		if (sdvpPingsSent > MAX_PINGS)
			sdvp_ConnectionStop(sdvpConfig);

		sdvpIdleCounter++;
	}

	xmpp_conn_release(conn);

	return 0;
}

/**
 * Stop a running connection 
 * @param sdvpConfig The connection object 
 */
void sdvp_ConnectionStop(sdvp_config_t *sdvpConfig) {
	sdvpIsRunning = false;
	xmpp_stop(ctx);
}

void sdvp_InitialiseWithDebug() {
	xmpp_log_t *log = NULL;
	xmpp_initialize();
	log = xmpp_get_syslog_logger(NULL);
	ctx = xmpp_ctx_new(NULL, log);
	sdvpIsInitialised = true;
}

void sdvp_Initialise() {
	xmpp_initialize();
	ctx = xmpp_ctx_new(NULL, NULL );
	sdvpIsInitialised = true;
}

/**
 * Frees the resources allocated and shutdown the underlaying modules of XMPP
 */
void sdvp_Shutdown() {
	xmpp_ctx_free(ctx);
	xmpp_shutdown();
	sdvpIsInitialised = false;
}

