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

#include "sdvp_helper.h"
#include "sdvp_register.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

char* _GeneratePacketId (char * packetId, int len) {
    unsigned short int i;
    srand((unsigned int) time(0) + getpid());
    for (i = 0; i < len - 1; i++) {
        packetId[i] = (rand() % 25 + 65);
        srand(rand());
    }
    packetId[i] = '\0';
    return packetId;
}

char* GetUsernameFromJid (char* jid) {
    return strtok(jid, "@");
}

sdvp_from_t* _CreateFrom (const char* name, const char* jid, const char* group) {
    sdvp_from_t* from;
    from = calloc(1, sizeof(sdvp_from_t));
    from->name = strdup(name);
    from->jid = strdup(jid);
    from->group = strdup(group);
    return from;
}

void _FreeFrom (sdvp_from_t* from) {
    if (from) {
        if (from->name) {
            free(from->name);
        }
        if (from->jid) {
            free(from->jid);
        }
        if (from->group) {
            free(from->group);
        }
        free(from);
    }
}

void* _CallTheRightCallback (sdvp_method_t method, void* param) {
    void* reply = NULL;

    switch (method) {
    case IEC_READ:
        reply = (void*) _CallRead((sdvp_ReadParam_t*) param);
        break;
    case IEC_WRITE:
        reply = (void*) _CallWrite((sdvp_WriteParam_t*) param);
        break;
    case IEC_GET_VARIABLE_ACCESS_ATTRIBUTE:
        reply = (void*) _CallGetVariableAccessAttribute(
                (sdvp_GetVariableAccessAttributeParam_t*) param);
        break;
    case IEC_GET_NAME_LIST:
        reply = (void*) _CallGetNameList((sdvp_GetNameListParam_t*) param);
        break;

    case SDVP_METHOD_61850_READ:
        case SDVP_METHOD_61850_WRITE:
        case SDVP_METHOD_61850_LIST:
        case SDVP_METHOD_DIRECT_READ:
        case SDVP_METHOD_DIRECT_WRITE:
        case SDVP_METHOD_DIRECT_LIST:
        case SDVP_METHOD_LIAB_LIST:
        reply = _CallSdvp(method, (sdvp_params_t*) param);
        break;

    case SDVP_METHOD_UNDEFINED:
        // Ignore for now
        break;
    }
    return reply;
}

