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

#ifndef ASOCKET_CLIENT_H_
#define ASOCKET_CLIENT_H_

#include "asocket.h"


/* struct asocket_client { */
/*     struct sockaddr *skaddr; */
/*     int socket_fd; */
/* }; */
//struct asocket_con* sk_con;

int asocket_clt_cnt_fd(struct sockaddr *skaddr, int retcnt);


struct asocket_con* asocket_clt_connect(struct sockaddr *skaddr);

/* int asckt_clt_trancive(struct asocket_client *client,  */
/*                        char *tx_buf, int tx_len,  */
/*                        char *rx_buf, int rx_maxlen); */

/* int asckt_clt_run(struct asocket_client *client,  */
/*                   const char *cmd, const char *payload, */
/*                   char *rx_buf, int rx_maxlen); */


int asocket_clt_disconnect(struct asocket_con* sk_con);

#endif /*ASOCKET_CLIENT_H_*/
