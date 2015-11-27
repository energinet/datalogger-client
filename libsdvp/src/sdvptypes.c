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
#

#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <time.h>

#define ISO8601_FORMAT_Z "%04d-%02d-%02dT%02d:%02d:%lfZ"
#define ISO8601_FORMAT_M "%04d-%02d-%02dT%02d:%02d:%lf-%02d:%02d"
#define ISO8601_FORMAT_P "%04d-%02d-%02dT%02d:%02d:%lf+%02d:%02d"

#if defined(HAVE_STRUCT_TM_TM_GMTOFF)
#define GMTOFF(t) ((t).tm_gmtoff)
#elif defined(HAVE_STRUCT_TM___TM_GMTOFF)
#define GMTOFF(t) ((t).__tm_gmtoff)
#elif defined(WIN32)
#define GMTOFF(t) (gmt_to_local_win32())
#elif defined(HAVE_TIMEZONE)
/* FIXME: the following assumes fixed dst offset of 1 hour */
#define GMTOFF(t) (-timezone + ((t).tm_isdst > 0 ? 3600 : 0))
#else
/* FIXME: work out the offset anyway. */
#define GMTOFF(t) (0)
#endif


#include "sdvptypes.h"
#include "sdvp_rpchelper.h"
#include "sdvp_helper.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

//// DEFINES ////

//// VARIABLES ////

//// CODE: PRIVATE ////

/** Initiates **/

sdvp_GetNameListParam_t* _InitiateGetNameListParameters () {
    sdvp_GetNameListParam_t* rv = calloc(1, sizeof(sdvp_GetNameListParam_t));
    return rv;
}

sdvp_GetNameListReply_t* sdvp_InitiateGetNameListReply (int numberOfIdentifiers) {
    sdvp_GetNameListReply_t* rv = calloc(1, sizeof(sdvp_GetNameListReply_t));
    rv->numberOfIdentifiers = numberOfIdentifiers;
    rv->moreFollows = false;
    rv->listOfIdentifier = calloc(numberOfIdentifiers, sizeof(char*));
    rv->serviceError = SERV_ERROR_NO_ERROR;
    return rv;
}

sdvp_GetVariableAccessAttributeParam_t*
_InitiateGetVariableAccessAttributesParameters () {
    sdvp_GetVariableAccessAttributeParam_t* rv = malloc(
            sizeof(sdvp_GetVariableAccessAttributeParam_t));
    return rv;
}

sdvp_GetVariableAccessAttributeReply_t*
sdvp_InitiateGetVariableAccessAttributesReply (int numberOfDataAttNames) {
    sdvp_GetVariableAccessAttributeReply_t* rv = calloc(1,
            sizeof(sdvp_GetVariableAccessAttributeReply_t));
    rv->numberOfDataAttNames = numberOfDataAttNames;
    rv->dataAttributeNames = calloc(numberOfDataAttNames, sizeof(char*));
    rv->serviceError = SERV_ERROR_NO_ERROR;
    return rv;
}

sdvp_ReadParam_t* _InitiateReadParameters () {
    sdvp_ReadParam_t* rv = calloc(1, sizeof(sdvp_ReadParam_t));
    return rv;
}

sdvp_ReadReply_t* sdvp_InitiateReadReply (int numberOfAccessResults) {
    int i = 0;
    sdvp_ReadReply_t* rv = calloc(1, sizeof(sdvp_ReadReply_t));
    rv->numberOfAccessResults = numberOfAccessResults;
    rv->serviceError = SERV_ERROR_NO_ERROR;
    rv->listOfAccessResults = calloc(
            numberOfAccessResults, sizeof(sdvp_access_results_t*));
    for (; i < numberOfAccessResults; i++) {
        rv->listOfAccessResults[i] = calloc(1, sizeof(sdvp_access_results_t));
    }
    return rv;
}

sdvp_WriteParam_t* _InitiateWriteParameters (int numberOfDataAttValues) {
    sdvp_WriteParam_t* rv = malloc(sizeof(sdvp_WriteParam_t));
    rv->params = calloc(1, sizeof(sdvp_write_params_t));
    return rv;
}

sdvp_WriteParam_t* _InitiateWriteDataParameters (sdvp_WriteParam_t* wp,
        int numberOfDataAttValues) {
    int i = 0;
    wp->params->numberOfDataAttValues = numberOfDataAttValues;
    wp->params->dataAttributeValues = calloc(numberOfDataAttValues,
            sizeof(sdvp_data_attribute_value_t*));
    for (; i < numberOfDataAttValues; i++) {
        wp->params->dataAttributeValues[i] = calloc(1,
                sizeof(sdvp_data_attribute_value_t));
    }
    return wp;
}

sdvp_WriteReply_t* sdvp_InitiateWriteReply (bool success) {
    sdvp_WriteReply_t* rv = calloc(1, sizeof(sdvp_WriteReply_t));
    rv->success = success;
    rv->serviceError = SERV_ERROR_NO_ERROR;
    return rv;
}

sdvp_params_t* _InitiateSdvpParameters (int numberOfParams) {
    sdvp_params_t* rv = calloc(1, sizeof(sdvp_params_t));
    rv->numberOfParams = numberOfParams;
    // Allocate memory for the paramteres
    rv->params = calloc(numberOfParams, sizeof(sdvp_param_t));
    return rv;
}

/** Frees **/

void sdvp_FreeGetNameListReply ( sdvp_GetNameListReply_t *reply ) {
	int i;
	for (i=0; i < reply->numberOfIdentifiers; i++) {
		free(reply->listOfIdentifier[i]);
	}
	free(reply->listOfIdentifier);
	free(reply); 
}

void sdvp_FreeGetVariableAccessAttributesReply ( 
		sdvp_GetVariableAccessAttributeReply_t *reply ) {
	int i;
	for (i=0; i < reply->numberOfDataAttNames; i++) {
		free(reply->dataAttributeNames[i]);
	}
	free(reply->dataAttributeNames);
	free(reply); 
}

void sdvp_FreeReadReply ( sdvp_ReadReply_t *reply ) {
	int i;
	for (i=0; i < reply->numberOfAccessResults; i++) {
		if (reply->listOfAccessResults[i]->strValue)
			free(reply->listOfAccessResults[i]->strValue);
		free(reply->listOfAccessResults[i]);
	}
	free(reply->listOfAccessResults);
	free(reply);
}

void sdvp_FreeWriteReply( sdvp_WriteReply_t *reply ) {
	free(reply);
}

static void _FreeListOfIdentifiers (sdvp_GetNameListReply_t* reply) {
    int i;
    if (reply->listOfIdentifier) {
        for (i = 0; i < reply->numberOfIdentifiers; i++) {
            free(reply->listOfIdentifier[i]);
        }
        free(reply->listOfIdentifier);
    }
}

void _FreeGetNameListParameters (sdvp_GetNameListParam_t* param) {
    _FreeFrom(param->sender);
    free(param->params.continueAfter);
    free(param->params.domainName);
    free(param->params.objectClass);
    free(param->params.objectScope);
    free(param);
}

void _FreeGetNameListReply (sdvp_GetNameListReply_t* reply) {
    free(reply->listOfIdentifier);
    free(reply);
}

void _FreeGetVariableAccessAttributesParameters (
        sdvp_GetVariableAccessAttributeParam_t* param) {
    _FreeFrom(param->sender);
    free(param->dataReference);
    free(param);
}

void _FreeReadParameters (sdvp_ReadParam_t* param) {
    _FreeFrom(param->sender);
    free(param->reference);
    free(param);
}

void _FreeWriteParameters (sdvp_WriteParam_t * param) {
    int i = 0;
    _FreeFrom(param->sender);
    for (; i < param->params->numberOfDataAttValues; i++) {
        if (param->params->dataAttributeValues[i]->strValue) {
            free(param->params->dataAttributeValues[i]->strValue);
        }
        free(param->params->dataAttributeValues[i]);
    }
    free(param->params->dataAttributeValues);
    free(param->params->reference);
    free(param->params);
    free(param);
}

void _FreeSdvpParameters (sdvp_params_t* callParameters) {
    int i = 0;
    if (callParameters->params) {
        for (; i < callParameters->numberOfParams; i++) {
            free(callParameters->params->value);
        }
        free(callParameters->params);
    }
    free(callParameters);
}

void _FreeParamStructure (sdvp_method_t method, void * param) {
    switch (method) {
    case IEC_READ:
        _FreeReadParameters((sdvp_ReadParam_t*) param);
        break;
    case IEC_WRITE:
        _FreeWriteParameters((sdvp_WriteParam_t*) param);
        break;
    case IEC_GET_VARIABLE_ACCESS_ATTRIBUTE:
        _FreeGetVariableAccessAttributesParameters(
                (sdvp_GetVariableAccessAttributeParam_t*) param);
        break;
    case IEC_GET_NAME_LIST:
        _FreeGetNameListParameters((sdvp_GetNameListParam_t*) param);
        break;
    case SDVP_METHOD_61850_READ:
        case SDVP_METHOD_61850_WRITE:
        case SDVP_METHOD_61850_LIST:
        case SDVP_METHOD_DIRECT_READ:
        case SDVP_METHOD_DIRECT_WRITE:
        case SDVP_METHOD_DIRECT_LIST:
        case SDVP_METHOD_LIAB_LIST:
        _FreeSdvpParameters((sdvp_params_t*) param);
        break;
    case SDVP_METHOD_UNDEFINED:
        break;
    }
}

void sdvp_FreeCallbackReply( sdvp_method_t method, void *reply ) {
	switch ( method ) {
		case IEC_READ:
			sdvp_FreeReadReply( (sdvp_ReadReply_t *) reply );
			break;
		case IEC_WRITE:
			sdvp_FreeWriteReply( (sdvp_WriteReply_t *) reply );
			break;
		case IEC_GET_VARIABLE_ACCESS_ATTRIBUTE:
			sdvp_FreeGetVariableAccessAttributesReply ( 
				( sdvp_GetVariableAccessAttributeReply_t *) reply );
			break;
		case IEC_GET_NAME_LIST:
			sdvp_FreeGetNameListReply ( (sdvp_GetNameListReply_t *) reply );
			break;
		default:;
	}
}

/** Creates **/

/**
 * Get all the parameters in the RPC call
 * @param[out] callParamteres This is filled out with the parameteres
 * @param[in] stanza This is the stanza which contains the RPC call and the params
 */
static sdvp_params_t* _CreateSdvpParameters (xmpp_stanza_t * const params) {
    sdvp_params_t* rv = NULL;
    int numberOfParams = 0;
    int paramLoop;
    numberOfParams = _FindNumberOfParams(params);
    rv = _InitiateSdvpParameters(numberOfParams);
    // Set all the params
    for (paramLoop = 0; paramLoop < numberOfParams; paramLoop++) {
        rv->params[paramLoop].value = _GetRpcParam(params, paramLoop);
    }
    return rv;
}

/**
 * Creates the parameter structure for the GetNameList callback
 * @param[in] params This stanza is the params element in the XML-RPC format
 * @return The structure or null if an error were found.
 */
static sdvp_GetNameListParam_t* _CreateGetNameListParam (xmpp_stanza_t * params) {
    const int expectedParams = 4;
    sdvp_GetNameListParam_t* rv = NULL;
    int i = 0;
    bool elementNotFund = false;
    xmpp_stanza_t *structure = NULL, *member = NULL, *value = NULL,
            *type = NULL, *name = NULL;
    char *text, *textname;

    structure = _GetRpcStructureElementFromParams(params);
    if (structure == NULL ) {
        return NULL ;
    }
    rv = _InitiateGetNameListParameters();
    for (member = xmpp_stanza_get_children(structure);
            member && !elementNotFund && (i < expectedParams); member =
                    xmpp_stanza_get_next(member)) {
        if (xmpp_stanza_get_name(member)
                && strcmp(xmpp_stanza_get_name(member), "member") == 0) {

            if (!(name = xmpp_stanza_get_child_by_name(member, "name"))) {
                _FreeGetNameListParameters(rv);
                // TODO: Signal error back
                return NULL ;
            }
            name = xmpp_stanza_get_children(name);

			textname = xmpp_stanza_get_text(name);
            if (strcmp(textname, "ObjectClass") == 0) {
                if (!(value = xmpp_stanza_get_child_by_name(member, "value"))) {
                    _FreeGetNameListParameters(rv);
                    // TODO: Signal error back
                    return NULL ;
                }
                if ((type = xmpp_stanza_get_child_by_name(value, "string"))) {
                    if (xmpp_stanza_get_children(type)) {
                        text = xmpp_stanza_get_text(type);
                        rv->params.objectClass = text;
                    } else {
                        rv->params.objectClass = NULL;
                    }
                } else {
                    elementNotFund = true;
                }

            } else if (strcmp(textname, "ObjectScope") == 0) {
                if ((type = xmpp_stanza_get_child_by_name(value, "string"))) {
                    if (xmpp_stanza_get_children(type)) {
                        text = xmpp_stanza_get_text(type);
                        rv->params.objectScope = text;
                    } else {
                        rv->params.objectScope = NULL;
                    }
                } else {
                    elementNotFund = true;
                }
            } else if (strcmp(textname, "DomainName") == 0) {
                if ((type = xmpp_stanza_get_child_by_name(value, "string"))) {
                    if (xmpp_stanza_get_children(type)) {
                        text = xmpp_stanza_get_text(type);
                        rv->params.domainName = text;
                    } else {
                        rv->params.domainName = NULL;
                    }
                } else {
                    elementNotFund = true;
                }
            } else if (strcmp(textname, "ContinueAfter") == 0) {
                if ((type = xmpp_stanza_get_child_by_name(value, "string"))) {
                    if (xmpp_stanza_get_children(type)) {
                        text = xmpp_stanza_get_text(type);
                        rv->params.continueAfter = text;
                    } else {
                        rv->params.continueAfter = NULL;
                    }
                } else {
                    elementNotFund = true;
                }
            } else {
//                if (!type) {
//                    _FreeGetNameListParameters(rv);
//                    return NULL ;
//                }
            }
			free(textname);
        }
    }
    return rv;
}

/**
 * Creates the parameter structure for the GetVariableAccessAttribute callback
 * @param[in] params This stanza is the params element in the XML-RPC format
 * @return The structure or null if an error were found.
 */
static sdvp_GetVariableAccessAttributeParam_t*
_CreateGetVariableAccessAttributeParam (xmpp_stanza_t * params) {
    const int expectedParams = 1;
    sdvp_GetVariableAccessAttributeParam_t* rv = NULL;
    int i = 0;
    bool elementNotFund = false;
    xmpp_stanza_t *structure = NULL, *member = NULL, *value = NULL,
            *type = NULL, *name = NULL;
    char *text, *textname;

    structure = _GetRpcStructureElementFromParams(params);
    if (structure == NULL ) {
        return NULL ;
    }
    rv = _InitiateGetVariableAccessAttributesParameters();
    for (member = xmpp_stanza_get_children(structure);
            member && !elementNotFund && (i < expectedParams); member =
                    xmpp_stanza_get_next(member)) {
        if (xmpp_stanza_get_name(member)
                && strcmp(xmpp_stanza_get_name(member), "member") == 0) {

            if (!(name = xmpp_stanza_get_child_by_name(member, "name"))) {
                _FreeGetVariableAccessAttributesParameters(rv);
                // TODO: Signal error back
                return NULL ;
            }
            name = xmpp_stanza_get_children(name);

			textname = xmpp_stanza_get_text(name);
            if (strcmp(textname, "Name") == 0) {
                if (!(value = xmpp_stanza_get_child_by_name(member, "value"))) {
                    _FreeGetVariableAccessAttributesParameters(rv);
                    // TODO: Signal error back
                    return NULL ;
                }
                if ((type = xmpp_stanza_get_child_by_name(value, "string"))) {
                    if (xmpp_stanza_get_children(type)) {
                        text = xmpp_stanza_get_text(type);
                        rv->dataReference = text;
                    } else {
                        rv->dataReference = NULL;
                    }
                } else {
                    elementNotFund = true;
                }
            } else {
//                if (!type) {
//                    _FreeGetVariableAccessAttributesParameters(rv);
//                    return NULL ;
//                }
            }
			free(textname);
        }
    }
    return rv;
}

/**
 * Creates the parameter structure for the Read callback
 * @param[in] params This stanza is the params element in the XML-RPC format
 * @return The structure or null if an error were found.
 */
static sdvp_ReadParam_t* _CreateReadParam (xmpp_stanza_t * params) {
    const int expectedParams = 1;
    sdvp_ReadParam_t* rv = NULL;
    int i = 0;
    bool elementNotFund = false;
    xmpp_stanza_t *structure = NULL, *member = NULL, *value = NULL,
            *type = NULL, *name = NULL;
    char *text, *textname;

    structure = _GetRpcStructureElementFromParams(params);
    if (structure == NULL ) {
        return NULL ;
    }
    rv = _InitiateReadParameters();
    for (member = xmpp_stanza_get_children(structure);
            member && !elementNotFund && (i < expectedParams); member =
                    xmpp_stanza_get_next(member)) {
        if (xmpp_stanza_get_name(member)
                && strcmp(xmpp_stanza_get_name(member), "member") == 0) {

            if (!(name = xmpp_stanza_get_child_by_name(member, "name"))) {
                _FreeReadParameters(rv);
                // TODO: Signal error back
                return NULL ;
            }
            name = xmpp_stanza_get_children(name);

			textname = xmpp_stanza_get_text(name);
            if (strcmp(textname, "VariableAccessSpecification") == 0) {
                if (!(value = xmpp_stanza_get_child_by_name(member, "value"))) {
                    _FreeReadParameters(rv);
                    // TODO: Signal error back
					free(textname);
                    return NULL ;
                }
                if ((type = xmpp_stanza_get_child_by_name(value, "string"))) {
                    if (xmpp_stanza_get_children(type)) {
                        text = xmpp_stanza_get_text(type);
                        rv->reference = text;
                    } else {
                        rv->reference = NULL;
                    }
                } else {
                    elementNotFund = true;
                }
            } else {
//                if (!type) {
//                    _FreeReadParameters(rv);
//                    return NULL ;
//                }
            }
			free(textname);
        }
    }
    return rv;
}


/* Takes an ISO-8601-formatted date string and returns the time_t.
 * Returns (time_t)-1 if the parse fails. */
// From: http://www.opensource.apple.com/source/neon/neon-11/neon/src/ne_dates.c
static time_t ne_iso8601_parse (const char *date) {
    struct tm gmt = {0};
    int off_hour, off_min;
    double sec;
    long int fix;
    int n;
    time_t result;

    /*  it goes: ISO8601: 2001-01-01T12:30:00+03:30 */
    if ((n = sscanf(date, ISO8601_FORMAT_P,
            &gmt.tm_year, &gmt.tm_mon, &gmt.tm_mday,
            &gmt.tm_hour, &gmt.tm_min, &sec,
            &off_hour, &off_min)) == 8) {
      gmt.tm_sec = (int)sec;
      fix = - off_hour * 3600 - off_min * 60;
    }
    /*  it goes: ISO8601: 2001-01-01T12:30:00-03:30 */
    else if ((n = sscanf(date, ISO8601_FORMAT_M,
             &gmt.tm_year, &gmt.tm_mon, &gmt.tm_mday,
             &gmt.tm_hour, &gmt.tm_min, &sec,
             &off_hour, &off_min)) == 8) {
      gmt.tm_sec = (int)sec;
      fix = off_hour * 3600 + off_min * 60;
    }
    /*  it goes: ISO8601: 2001-01-01T12:30:00Z */
    else if ((n = sscanf(date, ISO8601_FORMAT_Z,
             &gmt.tm_year, &gmt.tm_mon, &gmt.tm_mday,
             &gmt.tm_hour, &gmt.tm_min, &sec)) == 6) {
      gmt.tm_sec = (int)sec;
      fix = 0;
    }
    else {
      return (time_t)-1;
    }

    gmt.tm_year -= 1900;
    gmt.tm_isdst = -1;
    gmt.tm_mon--;

    result = mktime(&gmt) + fix;
    return result + GMTOFF(gmt);
}

static sdvp_WriteParam_t* _CreateWriteParam (xmpp_stanza_t * params) {
    int expectedParams, nVarParams = 0;
    sdvp_WriteParam_t* rv = NULL;
    int i = 0, j = 0, expectedDataElementsInArray;
    bool elementNotFund = false;
    xmpp_stanza_t *structure = NULL, *member = NULL, *value = NULL,
            *type = NULL;
    xmpp_stanza_t *name = NULL, *array, *arrayEntry;
    char *text, *textname;

    structure = _GetRpcStructureElementFromParams(params);
    if (structure == NULL ) {
        return NULL ;
    }

    // Get the number of elements
    //
    expectedParams = _FindNumberOfParams(params);
    if (expectedParams > 1) {
        nVarParams = expectedParams - 1;
    } else {
        // TODO: This should result in a ServiceError as minimum two is expected
    }

    rv = _InitiateWriteParameters(nVarParams);
    for (member = xmpp_stanza_get_children(structure);
            member && !elementNotFund && (i < expectedParams); member =
                    xmpp_stanza_get_next(member)) {
        if (xmpp_stanza_get_name(member)
                && strcmp(xmpp_stanza_get_name(member), "member") == 0) {

            if (!(name = xmpp_stanza_get_child_by_name(member, "name"))) {
                _FreeWriteParameters(rv);
                // TODO: Signal error back
                return NULL ;
            }
            name = xmpp_stanza_get_children(name);

			textname = xmpp_stanza_get_text(name);
            if (strcmp(textname, "VariableAccessSpecification") == 0) {
                if (!(value = xmpp_stanza_get_child_by_name(member, "value"))) {
                    _FreeWriteParameters(rv);
                    // TODO: Signal error back
                    return NULL ;
                }
                if ((type = xmpp_stanza_get_child_by_name(value, "string"))) {
                    if (xmpp_stanza_get_children(type)) {
                        text = xmpp_stanza_get_text(type);
                        rv->params->reference = text;
                    } else {
                        rv->params->reference = NULL;
                    }
                } else {
                    elementNotFund = true;
                }
            } else if (strcmp(textname, "ListOfData") == 0) {
                if (!(value = xmpp_stanza_get_child_by_name(member, "value"))) {
                    _FreeWriteParameters(rv);
                    // TODO: Signal error back
                    return NULL ;
                }
                if (!(array = xmpp_stanza_get_child_by_name(value, "array"))) {
                    _FreeWriteParameters(rv);
                    // TODO: Signal error back
                    return NULL ;
                }

                expectedDataElementsInArray = _countDataElementsInArray(array);
                _InitiateWriteDataParameters(rv, expectedDataElementsInArray);

//                array = xmpp_stanza_get_children(array);
                arrayEntry = xmpp_stanza_get_children(array);
//HJP Why skip the first element?!
//                arrayEntry = xmpp_stanza_get_next(arrayEntry);

                for (j = 0; arrayEntry;
                     arrayEntry = xmpp_stanza_get_next(arrayEntry)) {

                    if (xmpp_stanza_is_tag(arrayEntry)) {
                        if (!(value = xmpp_stanza_get_child_by_name(arrayEntry,
                                "value"))) {
                            _FreeWriteParameters(rv);
                            // TODO: Signal error back
                            return NULL ;
                        }

                        // MISSING: base64
                        if ((type = xmpp_stanza_get_child_by_name(value,
                                "string"))) {
                            rv->params->dataAttributeValues[j]->dataType =
                                    IEC_VISIBLE_STRING;
                            rv->params->dataAttributeValues[j]->strValue =
                                    xmpp_stanza_get_text(type);
                        } else if ((type = xmpp_stanza_get_child_by_name(value,
                                "i4"))
                                ||
                                (type = xmpp_stanza_get_child_by_name(value,
                                        "int"))) {
                            rv->params->dataAttributeValues[j]->dataType =
                                    IEC_INT32;
							text = xmpp_stanza_get_text(type);
                            rv->params->dataAttributeValues[j]->intValue =
                                    strtol(text, NULL,
                                            10);
							free(text);
                        } else if ((type = xmpp_stanza_get_child_by_name(value,
                                "double"))) {
                            rv->params->dataAttributeValues[j]->dataType =
                                    IEC_FLOAT32;
							text = xmpp_stanza_get_text(type);
                            rv->params->dataAttributeValues[j]->f32Value =
                                    strtof(text, NULL );
							free(text);
                        } else if ((type = xmpp_stanza_get_child_by_name(value,
                                "boolean"))) {
                            rv->params->dataAttributeValues[j]->dataType =
                                    IEC_BOOLEAN;
							text = xmpp_stanza_get_text(type);
                            if (strcmp(text, "1") == 0) {
                                rv->params->dataAttributeValues[j]->boolValue =
                                        true;
                            } else {
                                rv->params->dataAttributeValues[j]->boolValue =
                                        false;
                            }
							free(text);
                        } else if ((type = xmpp_stanza_get_child_by_name(value,
                                "dateTime.iso8601"))) {
                            rv->params->dataAttributeValues[j]->dataType =
                                    IEC_TIME;
							text = xmpp_stanza_get_text(type);
                            rv->params->dataAttributeValues[j]->time =
                                    ne_iso8601_parse(text);
							free(text);
                        }
                        j++;
                    }
                }

            } else {
//                if (!type) {
//                    _FreeWriteParameters(rv);
//                    return NULL ;
//                }
            }
			free(textname);
        }
    }
    return rv;
}


//// CODE: PUBLIC ////
/*
sdvp_GetNameListReply_t* sdvp_CreateGetNameListReply (
        iec_service_errors_t serviceError) {
    sdvp_GetNameListReply_t* reply;
    reply = malloc(sizeof(sdvp_GetNameListReply_t));
    reply->moreFollows = false;
    reply->serviceError = serviceError;
    return reply;
}

void sdvp_FreeGetNameListReply (sdvp_GetNameListReply_t* reply) {
    if (reply) {
        _FreeListOfIdentifiers(reply);
    }
}
*/
void sdvp_InitiateReplyParameters (sdvp_reply_params_t** callParameters,
        int numberOfParams) {
    int i;
    // Initiate the param holder if necessary
    if (!(*callParameters)) {
        (*callParameters) = malloc(sizeof(sdvp_reply_params_t));
    }
    (*callParameters)->numberOfParams = numberOfParams;

    // Allocate memory for the paramteres
    (*callParameters)->params = malloc(
            numberOfParams * sizeof(sdvp_reply_param_t));
    for (i = 0; i < numberOfParams; i++) {
        (*callParameters)->params->strValue = NULL;
    }
}

void sdvp_FreeReplyParameters (sdvp_reply_params_t* callParameters) {
    int paramLoop;
    if (callParameters) {
        if (callParameters->params) {
            for (paramLoop = 0; paramLoop < callParameters->numberOfParams;
                    paramLoop++) {
                if (callParameters->params[paramLoop].type == IEC_VISIBLE_STRING
                        || callParameters->params[paramLoop].type
                                == IEC_UNICODE_STRING
                        || callParameters->params[paramLoop].type
                                == IEC_CODED_ENUM
                        || callParameters->params[paramLoop].type
                                == IEC_OCTET_STRING
                        || callParameters->params[paramLoop].type
                                == IEC_ENUMERATED) {
                    free(callParameters->params[paramLoop].strValue);
                }
            }
            free(callParameters->params);
        }
		free(callParameters);
    }
}

/**
 * Creates the right parameter structure from the stanza based on the method.
 */
void* _CreateParamStructureFromParameters (sdvp_method_t method,
        xmpp_stanza_t * const stanza) {
    void* rv = NULL;
    xmpp_stanza_t * params;
    params = _GetRpcParamsElement(stanza);
    char* s = NULL;
    size_t buflen;
    xmpp_stanza_to_text(params, &s, &buflen);
    syslog(LOG_DEBUG, "Param stanza: %s \n", s);
	free(s);

    switch (method) {
    case IEC_READ:
        rv = _CreateReadParam(params);
        if (rv) {
            (((sdvp_ReadParam_t*) rv))->sender = _CreateFrom(
                    xmpp_stanza_get_attribute(stanza, "from"), "TODO", "TODO");
        }
        break;
    case IEC_WRITE:
        rv = _CreateWriteParam(params);
        if (rv) {
            (((sdvp_WriteParam_t*) rv))->sender = _CreateFrom(
                    xmpp_stanza_get_attribute(stanza, "from"), "TODO", "TODO");
        }
        break;
    case IEC_GET_VARIABLE_ACCESS_ATTRIBUTE:
        rv = _CreateGetVariableAccessAttributeParam(params);
        if (rv) {
            (((sdvp_GetVariableAccessAttributeParam_t*) rv))->sender =
                    _CreateFrom(
                            xmpp_stanza_get_attribute(stanza, "from"), "TODO",
                            "TODO");
        }
        break;
    case IEC_GET_NAME_LIST:
        rv = _CreateGetNameListParam(params);
        if (rv) {
            (((sdvp_GetNameListParam_t*) rv))->sender = _CreateFrom(
                    xmpp_stanza_get_attribute(stanza, "from"), "TODO", "TODO");
        }
        break;
    case SDVP_METHOD_61850_READ:
        case SDVP_METHOD_61850_WRITE:
        case SDVP_METHOD_61850_LIST:
        case SDVP_METHOD_DIRECT_READ:
        case SDVP_METHOD_DIRECT_WRITE:
        case SDVP_METHOD_DIRECT_LIST:
        case SDVP_METHOD_LIAB_LIST:
        rv = _CreateSdvpParameters(params);
        if (rv) {
            (((sdvp_params_t*) rv))->from = _CreateFrom(
                    xmpp_stanza_get_attribute(stanza, "from"), "TODO", "TODO");
        }

        break;
    case SDVP_METHOD_UNDEFINED:
        break;
    }

    return rv;
}

char* _ServiceErrorToString (iec_service_errors_t error) {
    char *rv = "Unknow error happend";
    switch (error) {
    case SERV_ERROR_INSTANCE_NOT_AVAILABLE:
        rv = "instance-not-available";
        break;
    case SERV_ERROR_INSTANCE_IN_USE:
        rv = "instance-in-use";
        break;
    case SERV_ERROR_ACCESS_VIOLATION:
        rv = "access-violation";
        break;
    case SERV_ERROR_ACCESS_NOT_ALLOWED_IN_CURRENT_STATE:
        rv = "access-not-allowed-in-current-state";
        break;
    case SERV_ERROR_PARAMETER_VALUE_INAPPROPRIATE:
        rv = "parameter-value-inappropriate";
        break;
    case SERV_ERROR_PARAMETER_VALUE_INCONSISTENT:
        rv = "parameter-value-inconsistent";
        break;
    case SERV_ERROR_CLASS_NOT_SUPPORTED:
        rv = "class-not-supported";
        break;
    case SERV_ERROR_INSTANCE_LOCKED_BY_OTHER_CLIENT:
        rv = "instance-locked-by-other-client";
        break;
    case SERV_ERROR_CONTROL_MUST_BE_SELECTED:
        rv = "control-must-be-selected";
        break;
    case SERV_ERROR_TYPE_CONFLICT:
        rv = "type-conflict";
        break;
    case SERV_ERROR_FAILED_DUE_TO_COMMUNICATIONS_CONSTRAINT:
        rv = "failed-due-to-communications-constraint";
        break;
    case SERV_ERROR_FAILED_DUE_TO_SERVER_CONSTRAINT:
        rv = "failed-due-to-server-constraint";
        break;
    case SERV_ERROR_NO_ERROR:
        rv = "no-error";
        break;
    }
    return rv;
}

