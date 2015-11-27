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

#ifndef SDVPTYPES_H_
#define SDVPTYPES_H_

#include <stdbool.h>
#include <strophe.h>
#include <time.h>

typedef enum {
    SDVP_DEBUG_DISABLED, SDVP_DEBUG_ENABLED
} sdvp_debug_t;

typedef enum {
    IEC_BOOLEAN,
    IEC_INT8,
    IEC_INT16,
    IEC_INT32,
    IEC_INT8U,
    IEC_INT16U,
    IEC_INT32U,
    IEC_FLOAT32,
    IEC_FLOAT64,
    IEC_ENUMERATED,
    IEC_CODED_ENUM,
    IEC_OCTET_STRING,
    IEC_VISIBLE_STRING,
    IEC_UNICODE_STRING,
    IEC_STRUCT,
    IEC_TIME
} sdvp_types;

typedef enum {
    SDVP_CONN_CONNECT, SDVP_CONN_DISCONNECT, SDVP_CONN_FAIL
} sdvp_conn_event_t;

// Only first group is listed
// TODO: Select the group with the highest level of rights
typedef struct sdvp_from_t {
    char* name;
    char* jid;
    char* group;
} sdvp_from_t;

typedef struct sdvp_param_t {
    char* value;
} sdvp_param_t;

typedef struct sdvp_params_t {
    sdvp_from_t* from;
    int numberOfParams;
    sdvp_param_t* params;
} sdvp_params_t;

typedef struct sdvp_reply_param_t {
    char* strValue;
    bool boolValue;
    int intValue;
    unsigned int uintValue;
    float f32Value;
//	Float64 f64Value;
    char* structValue;
    sdvp_types type;
} sdvp_reply_param_t;

typedef struct sdvp_reply_params_t {
    int numberOfParams;
    sdvp_reply_param_t* params;
} sdvp_reply_params_t;

typedef struct sdvp_context_t {

} sdvp_context_t;

typedef enum {
    SDVP_METHOD_UNDEFINED,
    SDVP_METHOD_61850_READ,
    SDVP_METHOD_61850_WRITE,
    SDVP_METHOD_61850_LIST,
    SDVP_METHOD_DIRECT_READ,
    SDVP_METHOD_DIRECT_WRITE,
    SDVP_METHOD_DIRECT_LIST,
    SDVP_METHOD_LIAB_LIST,
    IEC_READ,
    IEC_WRITE,
    IEC_GET_NAME_LIST,
    IEC_GET_VARIABLE_ACCESS_ATTRIBUTE,
	LIAB_WATCHDOG,
} sdvp_method_t;

/** Parameter for GetServerDirectory(...) **/
typedef enum {
    IEC_GET_SERVER_DIRECTORY_LOGICAL_DEVICE
//, IEC_GET_SERVER_DIRECTORY_FILE
} IEC_GET_SERVER_DIRECTORY_OBJECT_CLASS_T;

/* ServiceError as defined in 61850-7-2 section 6.1.2.6  */
typedef enum {
    SERV_ERROR_INSTANCE_NOT_AVAILABLE,
    SERV_ERROR_INSTANCE_IN_USE,
    SERV_ERROR_ACCESS_VIOLATION,
    SERV_ERROR_ACCESS_NOT_ALLOWED_IN_CURRENT_STATE,
    SERV_ERROR_PARAMETER_VALUE_INAPPROPRIATE,
    SERV_ERROR_PARAMETER_VALUE_INCONSISTENT,
    SERV_ERROR_CLASS_NOT_SUPPORTED,
    SERV_ERROR_INSTANCE_LOCKED_BY_OTHER_CLIENT,
    SERV_ERROR_CONTROL_MUST_BE_SELECTED,
    SERV_ERROR_TYPE_CONFLICT,
    SERV_ERROR_FAILED_DUE_TO_COMMUNICATIONS_CONSTRAINT,
    SERV_ERROR_FAILED_DUE_TO_SERVER_CONSTRAINT,
    SERV_ERROR_NO_ERROR
} iec_service_errors_t;

typedef struct iec_get_name_list_params_t {
    char *objectClass;
    char *objectScope;
    char *domainName;
    char *continueAfter;
} iec_get_name_list_params_t;

typedef struct sdvp_data_attribute_value_t {
    // TODO: Create this fully with type and etc
    char* strValue;
    bool boolValue;
    int intValue;
    unsigned int uintValue;
    float f32Value;
    double doubleValue;
    unsigned int time;
//  Float64 f64Value;
    sdvp_types dataType;
} sdvp_data_attribute_value_t;

typedef struct sdvp_access_results_t {
    // TODO: Create this fully with type and etc
    char* strValue;
    bool boolValue;
    int intValue;
    unsigned int uintValue;
    float f32Value;
//  Float64 f64Value;
    sdvp_types type;
} sdvp_access_results_t;

typedef struct iec_write_params_t {
    char *reference;
    int numberOfDataAttValues;
    sdvp_data_attribute_value_t** dataAttributeValues;
} sdvp_write_params_t;

typedef struct iec_get_name_list_reply_t {
    char **listOfIdentifier;
    bool moreFollows;
} iec_get_name_list_reply_t;

typedef struct sdvp_GetNameListParam_t {
    sdvp_from_t* sender;
    iec_get_name_list_params_t params;
} sdvp_GetNameListParam_t;

typedef struct sdvp_GetNameListReply_t {
    int numberOfIdentifiers;
    char **listOfIdentifier;
    bool moreFollows;
    iec_service_errors_t serviceError;
} sdvp_GetNameListReply_t;

typedef struct sdvp_ReadParam_t {
    sdvp_from_t* sender;
    char* reference;
} sdvp_ReadParam_t;

typedef struct sdvp_ReadReply_t {
    int numberOfAccessResults;
    sdvp_access_results_t **listOfAccessResults;
    iec_service_errors_t serviceError;
} sdvp_ReadReply_t;

typedef struct sdvp_WriteParam_t {
    sdvp_from_t* sender;
    sdvp_write_params_t* params;
} sdvp_WriteParam_t;

typedef struct sdvp_WriteReply_t {
    bool success;
    iec_service_errors_t serviceError;
} sdvp_WriteReply_t;

typedef struct sdvp_GetVariableAccessAttributeParam_t {
    sdvp_from_t* sender;
    char* dataReference;
} sdvp_GetVariableAccessAttributeParam_t;

typedef struct sdvp_GetVariableAccessAttributeReply_t {
    int numberOfDataAttNames;
    char **dataAttributeNames;
    iec_service_errors_t serviceError;
} sdvp_GetVariableAccessAttributeReply_t;

typedef struct sdvp_config_t {
    char *user;
    char *password;
    char *host;
    int port;
//	sdvp_reply_params_t* (*callback)(sdvp_method_t, sdvp_from_t*,
//			sdvp_params_t*);
    void (*callbackOnConnectionChange) (sdvp_conn_event_t);
    sdvp_debug_t debug;
} sdvp_config_t;

sdvp_GetNameListReply_t* sdvp_InitiateGetNameListReply (int numberOfIdentifiers);
sdvp_GetVariableAccessAttributeReply_t*
sdvp_InitiateGetVariableAccessAttributesReply (int numberOfDataAttNames);
sdvp_ReadReply_t* sdvp_InitiateReadReply (int numberOfAccessResults);
sdvp_WriteReply_t* sdvp_InitiateWriteReply (bool success);
void sdvp_FreeReplyParameters (sdvp_reply_params_t* callParameters);
void _FreeGetNameListReply (sdvp_GetNameListReply_t* reply);
void _FreeGetVariableAccessAttributesParameters (
        sdvp_GetVariableAccessAttributeParam_t* param);
void _FreeReadParameters (sdvp_ReadParam_t* param);
void _FreeWriteParameters (sdvp_WriteParam_t * param);

void sdvp_InitiateReplyParameters (sdvp_reply_params_t** callParameters,
        int numberOfParams);
void* _CreateParamStructureFromParameters (sdvp_method_t method,
        xmpp_stanza_t * const stanza);
void _FreeSdvpParameters (sdvp_params_t* callParameters);
char* _ServiceErrorToString (iec_service_errors_t error);
#endif /* SDVPTYPES_H_ */
