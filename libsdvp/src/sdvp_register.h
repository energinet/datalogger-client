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
#ifndef SDVPREGISTER_H_
#define SDVPREGISTER_H_

#include "sdvptypes.h"

void sdvp_RegisterCallback (sdvp_reply_params_t* (*callback)
        (sdvp_method_t, sdvp_from_t*, sdvp_params_t*));

void sdvp_RegisterRead (sdvp_ReadReply_t* (*cbRead) (sdvp_ReadParam_t* params));

void sdvp_RegisterWrite (
        sdvp_WriteReply_t* (*cbWrite) (sdvp_WriteParam_t* params));

void sdvp_RegisterGetNameList (
        sdvp_GetNameListReply_t* (*cbGetNameList) (
                sdvp_GetNameListParam_t* params));

void sdvp_RegisterGetVariableAccessAttribute (
        sdvp_GetVariableAccessAttributeReply_t* (*cbGetVariableAccessAttribute) (
                sdvp_GetVariableAccessAttributeParam_t* params));

sdvp_reply_params_t* _CallSdvp (sdvp_method_t method, sdvp_params_t* p);

sdvp_ReadReply_t* _CallRead (sdvp_ReadParam_t* params);

sdvp_WriteReply_t* _CallWrite (sdvp_WriteParam_t* params);

sdvp_GetNameListReply_t* _CallGetNameList (sdvp_GetNameListParam_t* params);

sdvp_GetVariableAccessAttributeReply_t* _CallGetVariableAccessAttribute (
        sdvp_GetVariableAccessAttributeParam_t* params);

int _CallbackIsRegistered (sdvp_method_t method);

#endif /* SDVPREGISTER_H_ */
