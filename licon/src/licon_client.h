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

#ifndef LICON_CLIENT_H_
#define LICON_CLIENT_H_

#include "licon_socket.h"

int licon_client_connect(int port);

int licon_client_get_status(char *rx_buf, int rx_maxlen);

int licon_client_set_option(const char *appif, const char *appifname, 
			    const char *option, char *rx_buf, int rx_maxlen);

int  licon_client_disconnect(void);

#endif /* LICON_CLIENT_H_ */
