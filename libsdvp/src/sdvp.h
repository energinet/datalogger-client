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

#ifndef SDVP_H_
#define SDVP_H_

#include "sdvptypes.h"
#include "sdvp_register.h"
#include "sdvp_map.h"
#include "sdvp_item.h"

sdvp_config_t* sdvp_ConfigNew ();
sdvp_config_t* sdvp_ConfigSetUsername (sdvp_config_t* config, const char *username);
sdvp_config_t* sdvp_ConfigSetPassword (sdvp_config_t* config, const char *password);
sdvp_config_t* sdvp_ConfigSetHost (sdvp_config_t* config, const char *host);
//sdvp_config_t* sdvp_ConfigSetCallback (sdvp_config_t* config,
//        sdvp_reply_params_t* (*callback) (sdvp_method_t, sdvp_from_t*, sdvp_params_t*));
sdvp_config_t* sdvp_ConfigSetCallbackOnConnectionChange (sdvp_config_t* config,
		void (*callback) (sdvp_conn_event_t));

sdvp_config_t* sdvp_ConfigSetDebug (sdvp_config_t* config,
        sdvp_debug_t debugState);

void sdvp_InitiateReplyParameters (sdvp_reply_params_t** callParameters,
        int numberOfParams);
int sdvp_ConnectionCreate (sdvp_config_t *sdvpConfig);
void sdvp_ConnectionStop (sdvp_config_t *sdvpConfig);


void sdvp_Shutdown ();

void sdvp_InitialiseWithDebug ();
void sdvp_Initialise ();

#endif /* SDVP_H_ */
