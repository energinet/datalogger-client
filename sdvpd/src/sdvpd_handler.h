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

#ifndef SDVP_HANDLER_H_
#define SDVP_HANDLER_H_

int sdvpd_handler_InitMap(void);

int sdvpd_handler_InitMapFile(const char *path);


void sdvpd_handler_TestMap(const char *mms);

sdvp_GetNameListReply_t* sdvpd_handler_cbGetNameList (sdvp_GetNameListParam_t* params);

sdvp_GetVariableAccessAttributeReply_t* 
sdvpd_handler_cbGetVariableAccessAttributes (sdvp_GetVariableAccessAttributeParam_t* params);


sdvp_ReadReply_t* sdvpd_handler_cbRead (sdvp_ReadParam_t* params);

sdvp_WriteReply_t* sdvpd_handler_cbWrite (sdvp_WriteParam_t* params);


sdvp_reply_params_t* sdvpd_handler_Callback (sdvp_method_t methodType, sdvp_from_t* from,
					     sdvp_params_t* params);


#endif /* SDVP_HANDLER_H_ */
