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

#ifndef XMLCNTNTRX_H_
#define XMLCNTNTRX_H_

#include <xml_serialize.h>


int spxml_cntn_trx_tx(struct xml_doc *doc, int fd);

ssize_t spxml_cntn_trx_read(int fd, void *buf, size_t count, int *run,int timeout, int dbglev);

int spxml_cntn_trx_read_doc(int fd, struct xml_stack *rx_stack,int *run,int timeout, int dbglev);


#endif /* XMLCNTNTRX_H_ */
