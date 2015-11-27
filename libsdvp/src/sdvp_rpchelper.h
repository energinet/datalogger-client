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
#ifndef SDVPRPCHELPER_H_
#define SDVPRPCHELPER_H_

#include "sdvptypes.h"

int _FindNumberOfParams (xmpp_stanza_t * params);
int _countDataElementsInArray (xmpp_stanza_t * array);
char* _GetRpcParam (xmpp_stanza_t * const stanza, int num);
xmpp_stanza_t* _GetRpcParamsElement(xmpp_stanza_t * const stanza);
xmpp_stanza_t* _GetRpcStructureElementFromParams (xmpp_stanza_t * const stanza);
int _SendRpcCall (xmpp_conn_t * const conn, xmpp_ctx_t *ctx,
        char * const fullServerJid);
char* _GetRpcMethode (xmpp_stanza_t * const stanza);

xmpp_stanza_t* _CreateReply (xmpp_ctx_t *ctx, sdvp_method_t method, void* reply);
#endif
