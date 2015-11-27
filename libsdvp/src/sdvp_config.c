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

#include "sdvp.h"
#include "define.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

sdvp_config_t* sdvp_ConfigNew () {
    sdvp_config_t* sdvpConfig = NULL;
    sdvpConfig = malloc(sizeof(*sdvpConfig));
    memset(sdvpConfig, 0, sizeof(*sdvpConfig));
    return sdvpConfig;
}

/**
 * Sets the username (a copy is made) in the config
 * @param [in/out] config The configuration to which the username is copied
 * @param [in] username The username to be set (username@domain)
 * @return The passed in config with the new name set
 */
sdvp_config_t* sdvp_ConfigSetUsername (sdvp_config_t* config, const char *username) {
    if (config->user) {
        free(config->user);
    }
    config->user = strdup(username);
    return config;
}

/**
 * Sets the password (a copy is made) in the config
 * @param [in/out] config The configuration to which the password is copied
 * @param [in] password The password to be set
 * @return The passed in config with the new password set
 */
sdvp_config_t* sdvp_ConfigSetPassword (sdvp_config_t* config, const char *password) {
    if (config->password) {
        free(config->password);
    }
    config->password = strdup(password);
    return config;
}

/**
 * Sets the host (a copy is made) in the config
 * @param [in/out] host The configuration to which the host is copied
 * @param [in] host The host to be set
 * @return The passed in config with the new host set
 */
sdvp_config_t* sdvp_ConfigSetHost (sdvp_config_t* config, const char *host) {
    if (config->host) {
        free(config->host);
    }
    config->host = strdup(host);
    return config;
}

/**
 * Sets the callback in the config
 * @param [in/out] config The configuration to which the callback is set
 * @param [in] callback The callback to be set
 * @return The passed in config with the new callback set
 */
//sdvp_config_t* sdvp_ConfigSetCallback (sdvp_config_t* config,
//        sdvp_reply_params_t* (*callback) (sdvp_method_t, sdvp_from_t*, sdvp_params_t*)) {
//    config->callback = callback;
//    return config;
//}

/**
 * Sets the callback in the config
 * @param [in/out] config The configuration to which the callback is set
 * @param [in] callback The callback to be set
 * @return The passed in config with the new callback set
 */
sdvp_config_t* sdvp_ConfigSetCallbackOnConnectionChange (sdvp_config_t* config,
        void (*callback) (sdvp_conn_event_t)) {
    config->callbackOnConnectionChange = callback;
    return config;
}

/**
 * Sets the debug state in the config
 * @param [in/out] config The configuration to which the debug state is set
 * @param [in] username The debug state to be set
 * @return The passed in config with the debug state set
 */
sdvp_config_t* sdvp_ConfigSetDebug (sdvp_config_t* config,
        sdvp_debug_t debugState) {
    config->debug = debugState;
    return config;
}

