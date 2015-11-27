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

#ifndef XMLCNTN_CLIENT_H_
#define XMLCNTN_CLIENT_H_

#include <xml_item.h>

struct xml_cntn_client{
  int fd;
  int run;
  struct xml_stack *stack;
  struct xml_doc *doc;
  int dbglev;
};



struct xml_cntn_client* xml_cntn_client_create(int dbglev);


void xml_cntn_client_delete(struct xml_cntn_client *client);

int xml_cntn_client_open(struct xml_cntn_client *client, const char *host, int port , int retcnt);

int xml_cntn_client_close(struct xml_cntn_client *client);

void xml_cntn_client_seq(struct xml_cntn_client *client, char *seq_str, size_t maxlen);

int xml_cntn_client_tx(struct xml_cntn_client *client, struct xml_item *txitem, const char *seq);

struct xml_item *xml_cntn_client_rx(struct xml_cntn_client *client, int timeout);

struct xml_item *xml_cntn_client_trx(struct xml_cntn_client* client, struct xml_item *txitem , int timeout);



#endif /* XMLCNTN_H_ */
