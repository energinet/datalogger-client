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

#include "asocket_client.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <asocket_trx.h>
#include <netinet/tcp.h>

struct asocket_con* asocket_clt_connect(struct sockaddr *skaddr) {

	int len;
	int retval;
	int client_fd;

	/* open socket */
	if ((client_fd = socket(skaddr->sa_family, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "server socket returned err: %s\n", strerror(errno));
		return NULL;
	}

	len = asocket_addr_len(skaddr);

	retval = connect(client_fd, skaddr, len);

	if (retval < 0) {
		fprintf(stderr, "connect error: %s (%d)\n", strerror(errno), errno);
		fprintf(stderr, "closing socket\n");
		close(client_fd);
		return NULL;

	}

	if (skaddr->sa_family == AF_INET) {
		int val;
		val = 10; /* 10 sec before starting probes */
		retval = setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &val,
				sizeof(val));

		val = 10; /* 10 sec before starting probes */
		retval
				= setsockopt(client_fd, SOL_TCP, TCP_KEEPIDLE, &val,
						sizeof(val));

		val = 2; /* 2 probes max */
		retval = setsockopt(client_fd, SOL_TCP, TCP_KEEPCNT, &val, sizeof(val));

		val = 10; /* 10 seconds between each probe */
		retval = setsockopt(client_fd, SOL_TCP, TCP_KEEPINTVL, &val,
				sizeof(val));
	}

	return asocket_con_create(client_fd, NULL, NULL, 0);;

}

int asocket_clt_disconnect(struct asocket_con* sk_con) {

	close(sk_con->fd);

	asocket_con_remove(sk_con);
	return 0;
}
