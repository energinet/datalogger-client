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

#ifndef ASOCKET_TRX_H_
#define ASOCKET_TRX_H_

#include "asocket.h"

#define SOCKET_TC_BUF_LEN 1024*20

int asocket_cmd_stub(ASOCKET_FUNC_PARAM);
struct asocket_cmd *asocket_cmd_match(struct asocket_cmd *cmds, const char *name);
int asocket_cmd_run(struct asocket_con* sk_con, struct asocket_cmd *cmd, struct skcmd *msg);


struct asocket_con* asocket_con_create(int skfd, struct asocket_cmd *cmds , void* appdata , int dbglev);
void asocket_con_remove(struct asocket_con* sk_con);


int asocket_con_snd(struct asocket_con* sk_con, struct skcmd *msg);
int asocket_con_snd_vp(struct asocket_con* sk_con, const char *id , int param_cnt,  ...);

int asocket_con_snd_nack(struct asocket_con* sk_con,const char *errmsg);


int asocket_con_rcv(struct asocket_con* sk_con, struct skcmd **rx_msg);


int asocket_con_rcvr(struct asocket_con* sk_con);

int asocket_con_trancive(struct asocket_con* sk_con, struct skcmd* tx_msg, struct skcmd** rx_msg);



#endif /* ASOCKET_H_ */

