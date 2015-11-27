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
#ifndef SDVPHELPER_H_
#define SDVPHELPER_H_

#include "sdvptypes.h"


char* _GeneratePacketId (char * packetId, int len);
char* GetUsernameFromJid (char* const jid);
sdvp_from_t* _CreateFrom (const char* name, const char* jid, const char* group);
void _FreeFrom (sdvp_from_t* from);
void* _CallTheRightCallback (sdvp_method_t method, void* param);
#endif
