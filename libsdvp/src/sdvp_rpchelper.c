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
#include <syslog.h>

#include "sdvp_rpchelper.h"
#include "sdvp_helper.h"
#include "sdvptypes.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define PACKET_ID_LENGTH_INC_TERM 7

/**
 * Finds the total number of 'param' elements in the stanza
 * @return the number of elements found
 */
int _FindNumberOfParams (xmpp_stanza_t * params) {
    int rv = 0;
    xmpp_stanza_t *param;
    for (param = xmpp_stanza_get_children(params); param; param =
            xmpp_stanza_get_next(param)) {
        if (xmpp_stanza_get_name(param)
                && strcmp(xmpp_stanza_get_name(param), "param") == 0) {
            rv++;
        }
    }

    return rv;
}

/**
 * Finds the total number of 'data' elements in the 'array'-stanza
 * @return the number of elements found
 */
int _countDataElementsInArray (xmpp_stanza_t * array) {
    int rv = 0;
    xmpp_stanza_t *data;
    for (data = xmpp_stanza_get_children(array); data; data =
            xmpp_stanza_get_next(data)) {
        if (xmpp_stanza_get_name(data)
                && strcmp(xmpp_stanza_get_name(data), "data") == 0) {
            rv++;
        }
    }
    return rv;
}

/**
 * Get the parameter number
 */
char* _GetRpcParam (xmpp_stanza_t * const stanza, int num) {
    int i = 0;
    xmpp_stanza_t *query, *methodCall, *params, *param, *value, *string;

    if (!(query = xmpp_stanza_get_child_by_name(stanza, "query"))) {
        return NULL ;
    }

    if (!(methodCall = xmpp_stanza_get_child_by_name(query, "methodCall"))) {
        return NULL ;
    }

    if (!(params = xmpp_stanza_get_child_by_name(methodCall, "params"))) {
        return NULL ;
    }

    for (param = xmpp_stanza_get_children(params); param; param =
            xmpp_stanza_get_next(param)) {
        if (xmpp_stanza_get_name(param)
                && strcmp(xmpp_stanza_get_name(param), "param") == 0) {
            if (i == num)
                break;
            i++;
        }
    }

    if (!param) {
        return NULL ;
    }

    if (!(value = xmpp_stanza_get_child_by_name(param, "value"))) {
        return NULL ;
    }

    if (!(string = xmpp_stanza_get_child_by_name(value, "string"))) {
        string = xmpp_stanza_get_child_by_name(value, "i4");
    }

    if (!string) {
        return NULL ;
    }

    return xmpp_stanza_get_text(string);
}

xmpp_stanza_t* _GetRpcParamsElement (xmpp_stanza_t * const stanza) {
    xmpp_stanza_t *query, *methodCall, *params;

    if (!(query = xmpp_stanza_get_child_by_name(stanza, "query"))) {
        return NULL ;
    }

    if (!(methodCall = xmpp_stanza_get_child_by_name(query, "methodCall"))) {
        return NULL ;
    }

    if (!(params = xmpp_stanza_get_child_by_name(methodCall, "params"))) {
        return NULL ;
    }
    return params;
}

xmpp_stanza_t* _GetRpcStructureElementFromParams (xmpp_stanza_t * const params) {
    xmpp_stanza_t *param, *value, *structure;

    if (!(param = xmpp_stanza_get_child_by_name(params, "param"))) {
        return NULL ;
    }

    if (!(value = xmpp_stanza_get_child_by_name(param, "value"))) {
        return NULL ;
    }

    if (!(structure = xmpp_stanza_get_child_by_name(value, "struct"))) {
        return NULL ;
    }

    return structure;
}

/**
 * This handles a status response iq be printing it out
 * @return 0 Causing this handler to be destroyed (single-shot)
 */
int HandleRpcResponse (xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
        void * const userdata) {
    xmpp_stanza_t *query, *item;

    char *type;
    char *text;
    size_t buf_size;
    type = xmpp_stanza_get_type(stanza);
    if (strcmp(type, "error") == 0) {
        syslog(LOG_WARNING, "ERROR: query failed\n");
    } else {
        query = xmpp_stanza_get_child_by_name(stanza, "status-response");
        syslog(LOG_DEBUG, "Status response:\n");
        for (item = xmpp_stanza_get_children(query); item; item =
                xmpp_stanza_get_next(item)) {
            xmpp_stanza_to_text(item, &text, &buf_size);
            syslog(LOG_DEBUG, "%s\n", text);
			free(text);
        }
    }

    return 0;
}

/**
 * Sends an Rpc method call over XMPP to fullServerJid
 * FIXME: Implement as in HandleRpcCall!
 */
int _SendRpcCall (xmpp_conn_t * const conn, xmpp_ctx_t *ctx,
        char * const fullServerJid) {
    xmpp_stanza_t *sr_iq, *sr_qry; //, *metCall;
    char packetId[PACKET_ID_LENGTH_INC_TERM];
    _GeneratePacketId(packetId, PACKET_ID_LENGTH_INC_TERM);

// create iq stanza for request
    sr_iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(sr_iq, "iq");
    xmpp_stanza_set_type(sr_iq, "get");
    xmpp_stanza_set_id(sr_iq, packetId);
    xmpp_stanza_set_attribute(sr_iq, "to", fullServerJid);

// Setting from is not needed as the server does this
//  xmpp_stanza_set_attribute(sr_iq, "from", "?@sdvp");
    sr_qry = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(sr_qry, "query");
    xmpp_stanza_set_ns(sr_qry, "jabber:iq:rpc");
//    xmpp_stanza_set_text(sr_qry, testCmd);

    xmpp_stanza_add_child(sr_iq, sr_qry);
    xmpp_stanza_release(sr_qry);
    xmpp_id_handler_add(conn, HandleRpcResponse, packetId, ctx);
// send out the stanza
    xmpp_send(conn, sr_iq);
    xmpp_stanza_release(sr_iq);

    return 0;
}

char* _GetRpcMethode (xmpp_stanza_t * const stanza) {
    xmpp_stanza_t *query, *methodCall, *methodName;

    if (!(query = xmpp_stanza_get_child_by_name(stanza, "query"))) {
        return NULL ;
    }

    if (!(methodCall = xmpp_stanza_get_child_by_name(query, "methodCall"))) {
        return NULL ;
    }

    if (!(methodName = xmpp_stanza_get_child_by_name(methodCall, "methodName"))) {
        return NULL ;
    }

    return xmpp_stanza_get_text(methodName);

}

/**
 * Constructs the XML-RPC format for the reply according to
 * http://xmpp.org/extensions/xep-0009.html
 * @return XMPP stanza containing the XML-RPC method response
 */
static xmpp_stanza_t* HandleRpcCallRet (xmpp_ctx_t *ctx,
        sdvp_reply_params_t* replyParams) {
    xmpp_stanza_t *query, *metRes, *params, *param, *value, *type, *valueText;
    char* text = NULL;
    int paramLoop;
    query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, "query");
    xmpp_stanza_set_ns(query, "jabber:iq:rpc");

    metRes = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(metRes, "methodResponse");
    xmpp_stanza_add_child(query, metRes);
	xmpp_stanza_release(metRes);

    params = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(params, "params");
    xmpp_stanza_add_child(metRes, params);
	xmpp_stanza_release(params);

    for (paramLoop = 0;
            (replyParams) && (paramLoop < replyParams->numberOfParams);
            paramLoop++) {
        param = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(param, "param");
        xmpp_stanza_add_child(params, param);
		xmpp_stanza_release(param);

        value = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(value, "value");
        xmpp_stanza_add_child(param, value);
		xmpp_stanza_release(value);

        type = xmpp_stanza_new(ctx);
        valueText = xmpp_stanza_new(ctx);
        switch (replyParams->params[paramLoop].type) {
        case IEC_BOOLEAN:
            xmpp_stanza_set_name(type, "boolean");
            if (replyParams->params[paramLoop].boolValue) {
                text = strdup("1");
            } else {
                text = strdup("0");
            }
            break;
        case IEC_ENUMERATED:
            case IEC_CODED_ENUM:
            case IEC_OCTET_STRING:
            case IEC_VISIBLE_STRING:
            case IEC_UNICODE_STRING:
            xmpp_stanza_set_name(type, "string");
            text = strdup(replyParams->params[paramLoop].strValue);
            break;
        case IEC_INT8:
            case IEC_INT16:
            case IEC_INT32:
            case IEC_INT8U:
            case IEC_INT16U:
            xmpp_stanza_set_name(type, "i4"); // or 'int'
            text = malloc(sizeof("2147483647"));
            sprintf(text, "%f", replyParams->params[paramLoop].f32Value);
            break;
        case IEC_INT32U:
            // TODO: Implement
            xmpp_stanza_set_name(type, "base64");
            text = strdup("Not implemented!\0");
            break;
        case IEC_FLOAT32:
            case IEC_FLOAT64:
            xmpp_stanza_set_name(type, "double");
            text = malloc(30); // TODO: Why this number?
            sprintf(text, "%f", replyParams->params[paramLoop].f32Value);
            break;
        case IEC_TIME:
            case IEC_STRUCT:
            break;
        }
        xmpp_stanza_add_child(value, type);
		xmpp_stanza_release(type);

        xmpp_stanza_set_text(valueText, text);
        xmpp_stanza_add_child(type, valueText);
		xmpp_stanza_release(valueText);
        free(text);
    }

    return query;

}

static xmpp_stanza_t* _CreateServiceErrorReplyStanza (xmpp_ctx_t *ctx,
        iec_service_errors_t error) {
    xmpp_stanza_t *query, *metRes, *params, *param, *pValue;
    xmpp_stanza_t *structure, *member, *name, *nameText, *sValue, *type,
            *valueText;
    char enumVal[20];

    query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, "query");
    xmpp_stanza_set_ns(query, "jabber:iq:rpc");

    metRes = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(metRes, "methodResponse");
    xmpp_stanza_add_child(query, metRes);
    xmpp_stanza_release(metRes);

    params = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(params, "params");
    xmpp_stanza_add_child(metRes, params);
    xmpp_stanza_release(params);

    param = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(param, "param");
    xmpp_stanza_add_child(params, param);
    xmpp_stanza_release(param);

    pValue = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(pValue, "value");
    xmpp_stanza_add_child(param, pValue);
    xmpp_stanza_release(pValue);

    structure = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(structure, "struct");
    xmpp_stanza_add_child(pValue, structure);
    xmpp_stanza_release(structure);

    // ServiceError as Text
    member = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(member, "member");
    xmpp_stanza_add_child(structure, member);
    xmpp_stanza_release(member);

    name = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(name, "name");
    xmpp_stanza_add_child(member, name);
    xmpp_stanza_release(name);

    nameText = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(nameText, "ServiceError");
    xmpp_stanza_add_child(name, nameText);
    xmpp_stanza_release(nameText);

    sValue = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(sValue, "value");
    xmpp_stanza_add_child(member, sValue);
    xmpp_stanza_release(sValue);

    type = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(type, "string");
    xmpp_stanza_add_child(sValue, type);
    xmpp_stanza_release(type);

    valueText = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(valueText, _ServiceErrorToString(error));
    xmpp_stanza_add_child(type, valueText);
    xmpp_stanza_release(valueText);


    // ServiceError as Enum value
    member = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(member, "member");
    xmpp_stanza_add_child(structure, member);
    xmpp_stanza_release(member);

    name = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(name, "name");
    xmpp_stanza_add_child(member, name);
    xmpp_stanza_release(name);

    nameText = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(nameText, "ServiceErrorEnum");
    xmpp_stanza_add_child(name, nameText);
    xmpp_stanza_release(nameText);

    sValue = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(sValue, "value");
    xmpp_stanza_add_child(member, sValue);
    xmpp_stanza_release(sValue);

    type = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(type, "i4");
    xmpp_stanza_add_child(sValue, type);
    xmpp_stanza_release(type);

    valueText = xmpp_stanza_new(ctx);
    snprintf(enumVal, 20, "%d", error);
    xmpp_stanza_set_text(valueText, enumVal);
    xmpp_stanza_add_child(type, valueText);
    xmpp_stanza_release(valueText);

    return query;
}

static xmpp_stanza_t* _CreateGetNameListReplyStanza (xmpp_ctx_t *ctx,
        sdvp_GetNameListReply_t* reply) {
    xmpp_stanza_t *query, *metRes, *params, *param, *valueParam, *structure;
    xmpp_stanza_t *memberId, *memberMf, *nameId, *nameMf, *valueId;
    xmpp_stanza_t *array, *data, *type, *valueText;
    int paramLoop;

    if (reply->serviceError != SERV_ERROR_NO_ERROR) {
        query = _CreateServiceErrorReplyStanza(ctx, reply->serviceError);
    } else {
        query = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(query, "query");
        xmpp_stanza_set_ns(query, "jabber:iq:rpc");

        metRes = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(metRes, "methodResponse");
        xmpp_stanza_add_child(query, metRes);
		xmpp_stanza_release(metRes);

        params = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(params, "params");
        xmpp_stanza_add_child(metRes, params);
		xmpp_stanza_release(params);

        param = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(param, "param");
        xmpp_stanza_add_child(params, param);
		xmpp_stanza_release(param);

        valueParam = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(valueParam, "value");
        xmpp_stanza_add_child(param, valueParam);
		xmpp_stanza_release(valueParam);

        structure = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(structure, "struct");
        xmpp_stanza_add_child(valueParam, structure);
		xmpp_stanza_release(structure);

        memberId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(memberId, "member");
        xmpp_stanza_add_child(structure, memberId);
		xmpp_stanza_release(memberId);

        nameId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(nameId, "name");
        xmpp_stanza_add_child(memberId, nameId);
		xmpp_stanza_release(nameId);

        valueText = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(valueText, "Identifiers");
        xmpp_stanza_add_child(nameId, valueText);
		xmpp_stanza_release(valueText);

        valueId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(valueId, "value");
        xmpp_stanza_add_child(memberId, valueId);
		xmpp_stanza_release(valueId);

        array = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(array, "array");
        xmpp_stanza_add_child(valueId, array);
		xmpp_stanza_release(array);

        for (paramLoop = 0;
                (reply) && (paramLoop < reply->numberOfIdentifiers);
                paramLoop++) {
            data = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(data, "data");
            xmpp_stanza_add_child(array, data);
			xmpp_stanza_release(data);

            valueParam = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(valueParam, "value");
            xmpp_stanza_add_child(data, valueParam);
			xmpp_stanza_release(valueParam);

            type = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(type, "string");
            xmpp_stanza_add_child(valueParam, type);
			xmpp_stanza_release(type);

            valueText = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(valueText, reply->listOfIdentifier[paramLoop]);
            xmpp_stanza_add_child(type, valueText);
			xmpp_stanza_release(valueText);
        }

        memberMf = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(memberMf, "member");
        xmpp_stanza_add_child(structure, memberMf);
		xmpp_stanza_release(memberMf);

        nameMf = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(nameMf, "name");
        xmpp_stanza_add_child(memberMf, nameMf);
		xmpp_stanza_release(nameMf);

        valueText = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(valueText, "MoreFollows");
        xmpp_stanza_add_child(nameMf, valueText);
		xmpp_stanza_release(valueText);

        valueParam = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(valueParam, "value");
        xmpp_stanza_add_child(memberMf, valueParam);
		xmpp_stanza_release(valueParam);

        type = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(type, "boolean");
        xmpp_stanza_add_child(valueParam, type);
		xmpp_stanza_release(type);

        valueText = xmpp_stanza_new(ctx);
        if (reply->moreFollows) {
            xmpp_stanza_set_text(valueText, "1");
        } else {
            xmpp_stanza_set_text(valueText, "0");
        }
        xmpp_stanza_add_child(type, valueText);
		xmpp_stanza_release(valueText);
    }

    return query;
}

static xmpp_stanza_t* _CreateGetVariableAccessAttributeReplyStanza (
        xmpp_ctx_t *ctx, sdvp_GetVariableAccessAttributeReply_t* reply) {
    xmpp_stanza_t *query, *metRes, *params, *param, *valueParam, *structure;
    xmpp_stanza_t *memberId, *nameId, *valueId;
    xmpp_stanza_t *array, *data, *type, *valueText;
    int paramLoop;

    if (reply->serviceError != SERV_ERROR_NO_ERROR) {
        query = _CreateServiceErrorReplyStanza(ctx, reply->serviceError);
    } else {
        query = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(query, "query");
        xmpp_stanza_set_ns(query, "jabber:iq:rpc");

        metRes = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(metRes, "methodResponse");
        xmpp_stanza_add_child(query, metRes);
        xmpp_stanza_release(metRes);

        params = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(params, "params");
        xmpp_stanza_add_child(metRes, params);
        xmpp_stanza_release(params);

        param = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(param, "param");
        xmpp_stanza_add_child(params, param);
        xmpp_stanza_release(param);

        valueParam = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(valueParam, "value");
        xmpp_stanza_add_child(param, valueParam);
        xmpp_stanza_release(valueParam);

        structure = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(structure, "struct");
        xmpp_stanza_add_child(valueParam, structure);
        xmpp_stanza_release(structure);

        memberId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(memberId, "member");
        xmpp_stanza_add_child(structure, memberId);
        xmpp_stanza_release(memberId);

        nameId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(nameId, "name");
        xmpp_stanza_add_child(memberId, nameId);
        xmpp_stanza_release(nameId);

        valueText = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(valueText, "TypeDescription");
        xmpp_stanza_add_child(nameId, valueText);
        xmpp_stanza_release(valueText);

        valueId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(valueId, "value");
        xmpp_stanza_add_child(memberId, valueId);
		xmpp_stanza_release(valueId);

        array = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(array, "array");
        xmpp_stanza_add_child(valueId, array);
        xmpp_stanza_release(array);

        for (paramLoop = 0;
                (reply) && (paramLoop < reply->numberOfDataAttNames);
                paramLoop++) {
            data = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(data, "data");
            xmpp_stanza_add_child(array, data);
			xmpp_stanza_release(data);

            valueParam = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(valueParam, "value");
            xmpp_stanza_add_child(data, valueParam);
			xmpp_stanza_release(valueParam);

            type = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(type, "string");
            xmpp_stanza_add_child(valueParam, type);
			xmpp_stanza_release(type);

            valueText = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(valueText,
                    reply->dataAttributeNames[paramLoop]);
            xmpp_stanza_add_child(type, valueText);
        	xmpp_stanza_release(valueText);
        }

    }

    return query;
}

static xmpp_stanza_t* _CreateReadReplyStanza (
    xmpp_ctx_t *ctx, sdvp_ReadReply_t* reply) {
    xmpp_stanza_t *query, *metRes, *params, *param, *valueParam, *structure;
    xmpp_stanza_t *memberId, *nameId, *valueId;
    xmpp_stanza_t *array, *data, *type, *valueText;
    int paramLoop;
    char* text = NULL;

    if (reply->serviceError != SERV_ERROR_NO_ERROR) {
        query = _CreateServiceErrorReplyStanza(ctx, reply->serviceError);
    } else {
        query = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(query, "query");
        xmpp_stanza_set_ns(query, "jabber:iq:rpc");

        metRes = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(metRes, "methodResponse");
        xmpp_stanza_add_child(query, metRes);
        xmpp_stanza_release(metRes);

        params = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(params, "params");
        xmpp_stanza_add_child(metRes, params);
        xmpp_stanza_release(params);

        param = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(param, "param");
        xmpp_stanza_add_child(params, param);
        xmpp_stanza_release(param);

        valueParam = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(valueParam, "value");
        xmpp_stanza_add_child(param, valueParam);
		xmpp_stanza_release(valueParam);

        structure = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(structure, "struct");
        xmpp_stanza_add_child(valueParam, structure);
        xmpp_stanza_release(structure);

        memberId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(memberId, "member");
        xmpp_stanza_add_child(structure, memberId);
        xmpp_stanza_release(memberId);

        nameId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(nameId, "name");
        xmpp_stanza_add_child(memberId, nameId);
        xmpp_stanza_release(nameId);

        valueText = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(valueText, "ListOfAccessResult");
        xmpp_stanza_add_child(nameId, valueText);
		xmpp_stanza_release(valueText);

        valueId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(valueId, "value");
        xmpp_stanza_add_child(memberId, valueId);
        xmpp_stanza_release(valueId);

        array = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(array, "array");
        xmpp_stanza_add_child(valueId, array);
        xmpp_stanza_release(array);

        for (paramLoop = 0;
                (reply) && (paramLoop < reply->numberOfAccessResults);
                paramLoop++) {
            data = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(data, "data");
            valueParam = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(valueParam, "value");
            type = xmpp_stanza_new(ctx);
            valueText = xmpp_stanza_new(ctx);

            switch (reply->listOfAccessResults[paramLoop]->type) {
            case IEC_BOOLEAN:
                xmpp_stanza_set_name(type, "boolean");
                if (reply->listOfAccessResults[paramLoop]->boolValue) {
                    text = strdup("1");
                } else {
                    text = strdup("0");
                }
                break;
            case IEC_ENUMERATED:
                case IEC_CODED_ENUM:
                case IEC_OCTET_STRING:
                case IEC_VISIBLE_STRING:
                case IEC_UNICODE_STRING:
                xmpp_stanza_set_name(type, "string");
                text = strdup(reply->listOfAccessResults[paramLoop]->strValue);
                break;
            case IEC_INT8:
                case IEC_INT16:
                case IEC_INT32:
                case IEC_INT8U:
                case IEC_INT16U:
                xmpp_stanza_set_name(type, "i4"); // or 'int'
                text = malloc(sizeof("2147483647"));
                sprintf(text, "%d",
                        reply->listOfAccessResults[paramLoop]->intValue);
                break;
            case IEC_INT32U:
                // TODO: Implement
                xmpp_stanza_set_name(type, "base64");
                text = strdup("Not implemented!\0");
                break;
            case IEC_FLOAT32:
                case IEC_FLOAT64:
                xmpp_stanza_set_name(type, "double");
                text = malloc(30);
                sprintf(text, "%f",
                        reply->listOfAccessResults[paramLoop]->f32Value);
                break;
            case IEC_TIME:
                xmpp_stanza_set_name(type, "dateTime.iso8601");
                char buf[sizeof "2014-04-01T00:00:00Z"];
                time_t t = reply->listOfAccessResults[paramLoop]->intValue;
                strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
                text = strdup(buf);
                break;

            case IEC_STRUCT:
                break;
            }

            xmpp_stanza_set_text(valueText, text);
            xmpp_stanza_add_child(type, valueText);
            xmpp_stanza_add_child(valueParam, type);
            xmpp_stanza_add_child(data, valueParam);
            xmpp_stanza_add_child(array, data);

            free(text);
            xmpp_stanza_release(valueText);
            xmpp_stanza_release(type);
            xmpp_stanza_release(valueParam);
            xmpp_stanza_release(data);
        }

    }

    return query;
}

static xmpp_stanza_t* _CreateWriteReplyStanza (
        xmpp_ctx_t *ctx, sdvp_WriteReply_t* reply) {
    xmpp_stanza_t *query, *metRes, *params, *param, *valueParam, *structure;
    xmpp_stanza_t *memberId, *nameId, *valueId, *type, *valueText;

    if (reply->serviceError != SERV_ERROR_NO_ERROR) {
        query = _CreateServiceErrorReplyStanza(ctx, reply->serviceError);
    } else {
        query = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(query, "query");
        xmpp_stanza_set_ns(query, "jabber:iq:rpc");

        metRes = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(metRes, "methodResponse");
        xmpp_stanza_add_child(query, metRes);

        params = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(params, "params");
        xmpp_stanza_add_child(metRes, params);

        param = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(param, "param");
        xmpp_stanza_add_child(params, param);

        valueParam = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(valueParam, "value");
        xmpp_stanza_add_child(param, valueParam);

        structure = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(structure, "struct");
        xmpp_stanza_add_child(valueParam, structure);

        memberId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(memberId, "member");
        xmpp_stanza_add_child(structure, memberId);

        nameId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(nameId, "name");
        xmpp_stanza_add_child(memberId, nameId);

        valueText = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(valueText, "WriteResult");
        xmpp_stanza_add_child(nameId, valueText);
        xmpp_stanza_release(valueText);

        valueId = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(valueId, "value");
        xmpp_stanza_add_child(memberId, valueId);

        type = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(type, "boolean");
        xmpp_stanza_add_child(valueId, type);

        valueText = xmpp_stanza_new(ctx);
        if (reply->success) {
            xmpp_stanza_set_text(valueText, "1");
        } else {
            xmpp_stanza_set_text(valueText, "0");
        }
        xmpp_stanza_add_child(type, valueText);

        xmpp_stanza_release(metRes);
        xmpp_stanza_release(params);
        xmpp_stanza_release(param);
        xmpp_stanza_release(valueParam);
        xmpp_stanza_release(structure);
        xmpp_stanza_release(memberId);
        xmpp_stanza_release(nameId);
        xmpp_stanza_release(valueId);
        xmpp_stanza_release(type);
        xmpp_stanza_release(valueText);
    }

    return query;
}

/**
 * Constructs the XML-RPC format for the reply according to
 * http://xmpp.org/extensions/xep-0009.html
 * @return XMPP stanza containing the XML-RPC method response
 */
xmpp_stanza_t* _CreateReply (xmpp_ctx_t *ctx, sdvp_method_t method, void* reply) {
    xmpp_stanza_t *rv = NULL;

    switch (method) {
    case IEC_READ:
        rv = _CreateReadReplyStanza(ctx, (sdvp_ReadReply_t*) reply);
        break;
    case IEC_WRITE:
        rv = _CreateWriteReplyStanza(ctx, (sdvp_WriteReply_t*) reply);
        break;
    case IEC_GET_VARIABLE_ACCESS_ATTRIBUTE:
        rv = _CreateGetVariableAccessAttributeReplyStanza(
                ctx, (sdvp_GetVariableAccessAttributeReply_t*) reply);
        break;
    case IEC_GET_NAME_LIST:
        rv = _CreateGetNameListReplyStanza(ctx,
                (sdvp_GetNameListReply_t*) reply);
        break;
    case SDVP_METHOD_61850_READ:
        case SDVP_METHOD_61850_WRITE:
        case SDVP_METHOD_61850_LIST:
        case SDVP_METHOD_DIRECT_READ:
        case SDVP_METHOD_DIRECT_WRITE:
        case SDVP_METHOD_DIRECT_LIST:
        case SDVP_METHOD_LIAB_LIST:
        case SDVP_METHOD_UNDEFINED:
        rv = HandleRpcCallRet(ctx, (sdvp_reply_params_t*) reply);
        break;
    }

    return rv;
}

