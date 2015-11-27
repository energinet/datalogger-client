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

#include "sdvp_register.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

//// DEFINES ////


//// VARIABLES ////

static sdvp_reply_params_t* (*callbackSdvp) (sdvp_method_t, sdvp_from_t*, sdvp_params_t*) = NULL;
static sdvp_ReadReply_t* (*callbackRead) (sdvp_ReadParam_t* params) = NULL;
static sdvp_WriteReply_t* (*callbackWrite) (sdvp_WriteParam_t* params) = NULL;
static sdvp_GetNameListReply_t* (*callbackGetNameList) (sdvp_GetNameListParam_t* params) = NULL;
static sdvp_GetVariableAccessAttributeReply_t* (*callbackGetVariableAccessAttribute)
		(sdvp_GetVariableAccessAttributeParam_t* params) = NULL;


//// CODE: PRIVATE ////

static bool parametersIsSet (void *param) {
    if (param == NULL) {
        return false;
    } else {
        return true;
    }
}


//// CODE: PUBLIC ////


/* Register functions  */

void sdvp_RegisterCallback (
        sdvp_reply_params_t* (*callback) (sdvp_method_t, sdvp_from_t*,
                sdvp_params_t*)) {
    callbackSdvp = callback;
}

void sdvp_RegisterRead (sdvp_ReadReply_t* (*cbRead) (sdvp_ReadParam_t* params)) {
    callbackRead = cbRead;
}

void sdvp_RegisterWrite (sdvp_WriteReply_t* (*cbWrite) (sdvp_WriteParam_t* params)) {
	callbackWrite = cbWrite;
}

void sdvp_RegisterGetNameList (sdvp_GetNameListReply_t* (*cbGetNameList) (sdvp_GetNameListParam_t* params)) {
	callbackGetNameList = cbGetNameList;
}

void sdvp_RegisterGetVariableAccessAttribute (
		sdvp_GetVariableAccessAttributeReply_t* (*cbGetVariableAccessAttribute)
		(sdvp_GetVariableAccessAttributeParam_t* params)) {
	callbackGetVariableAccessAttribute = cbGetVariableAccessAttribute;
}

/* Call callback functions  */

sdvp_reply_params_t* _CallSdvp (sdvp_method_t method, sdvp_params_t* params) {
    sdvp_reply_params_t* reply = NULL;
    if (!parametersIsSet(params)) {
        // TODO: Create service error telling that the parameter is wrong
    } else if (callbackSdvp != NULL) {
        reply = callbackSdvp(method, params->from, params);
    } else {
        // TODO: Create service error telling that the service in not impl
    }
    return reply;
}


sdvp_ReadReply_t* _CallRead (sdvp_ReadParam_t* params) {
	sdvp_ReadReply_t* reply = NULL;
    if (!parametersIsSet(params)) {
        // TODO: Create service error telling that the parameter is wrong
    } else if (callbackRead != NULL) {
		reply = callbackRead(params);
    } else {
        // TODO: Create service error telling that the service in not impl
	}
	return reply;
}

sdvp_WriteReply_t* _CallWrite (sdvp_WriteParam_t* params) {
	sdvp_WriteReply_t* reply = NULL;
    if (!parametersIsSet(params)) {
        // TODO: Create service error telling that the parameter is wrong
    } else if (callbackWrite != NULL) {
		reply = callbackWrite(params);
    } else {
        // TODO: Create service error telling that the service in not impl
	}
	return reply;
}

sdvp_GetNameListReply_t* _CallGetNameList (sdvp_GetNameListParam_t* params) {
	sdvp_GetNameListReply_t* reply = NULL;
	if (!parametersIsSet(params)) {
	    // TODO: Create service error telling that the parameter is wrong
	} else if (callbackGetNameList != NULL) {
		reply = callbackGetNameList(params);
    } else {
        // TODO: Create service error telling that the service in not impl
	}
	return reply;
}

sdvp_GetVariableAccessAttributeReply_t* _CallGetVariableAccessAttribute (
		sdvp_GetVariableAccessAttributeParam_t* params) {
	sdvp_GetVariableAccessAttributeReply_t* reply = NULL;
    if (!parametersIsSet(params)) {
        // TODO: Create service error telling that the parameter is wrong
    } else if (callbackGetVariableAccessAttribute != NULL) {
		reply = callbackGetVariableAccessAttribute(params);
    } else {
        // TODO: Create service error telling that the service in not impl
	}
	return reply;
}


/**
 * Checks that the callback is registered for the specified sdvp method type
 * @return 1 if a callback is registred else 0
 */
int _CallbackIsRegistered(sdvp_method_t method) {
	int rv = 0;
	switch (method) {
	case IEC_READ:
		rv = (callbackRead != NULL);
		break;
	case IEC_WRITE:
		rv = (callbackWrite != NULL);
		break;
	case IEC_GET_VARIABLE_ACCESS_ATTRIBUTE:
		rv = (callbackGetVariableAccessAttribute != NULL);
		break;
	case IEC_GET_NAME_LIST:
		rv = (callbackGetNameList != NULL);
		break;
	case SDVP_METHOD_61850_READ:
	case SDVP_METHOD_61850_WRITE:
	case SDVP_METHOD_61850_LIST:
	case SDVP_METHOD_DIRECT_READ:
	case SDVP_METHOD_DIRECT_WRITE:
	case SDVP_METHOD_DIRECT_LIST:
	case SDVP_METHOD_LIAB_LIST:
	case SDVP_METHOD_UNDEFINED:
		rv = (callbackSdvp != NULL);
		break;
	}
	return rv;
}



