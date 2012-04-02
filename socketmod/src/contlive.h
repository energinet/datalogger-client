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

#ifndef CONTLIVE_H_
#define CONTLIVE_H_

#include <asocket.h>
#include <asocket_trx.h>
#include <asocket_client.h>

#define DEFAULT_PORT 6523


struct asocket_con* contlive_connect(const char *address, int port, int dbglev);

int contlive_disconnect(struct asocket_con* sk_con);

int contlive_get_updlst(struct asocket_con* sk_con, const char *flags, const char *ename);

int contlive_get_next(struct asocket_con* sk_con, char *ename, char *value, char *hname, char *unit);

int contlive_get_single(struct asocket_con* sk_con, const char *ename, char *value, char *hname, char *unit);

int contlive_set_single(struct asocket_con* sk_con, const char *ename , const char *value);


#endif /* ENDATA_H_ */

