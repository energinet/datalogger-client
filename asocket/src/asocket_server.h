/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2011 LIAB ApS <info@liab.dk>
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

#ifndef ASOCKET_SERVER_H_
#define ASOCKET_SERVER_H_

#include <pthread.h>
#include <sys/socket.h>

#include "asocket.h"

struct socket_cmd {
	char *cmd;
	int (*function)(ASOCKET_FUNC_PARAM);
	char *help;
};

struct socket_data {
	void *appdata;
	struct sockaddr *skaddr;
	int run;
	int socket_fd;
	struct asocket_cmd *cmds;
	int (*con_handler)(struct asocket_con* sk_con);
	pthread_t thread;
	int dbglev;
};

int asckt_srv_start(struct socket_data *param);
void asckt_srv_close(struct socket_data *param);

int asocket_handler_stub(ASOCKET_FUNC_PARAM);

#endif /* ASOCKET_SERVER_H_ */

