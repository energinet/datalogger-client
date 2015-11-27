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

#ifndef XMLCNTN_SERVER_H_
#define XMLCNTN_SERVER_H_
#include <sys/types.h>

#include <asocket_trx.h>
#include <asocket_server.h>

//#include <xmlparser.h>

struct xml_item;
/**
 * Modemd server opbect
 */
struct xml_cntn_server{
    struct socket_data sparm;
    /**
     * List of connections */
    struct spxml_cntn_list *cnlst;
    pthread_mutex_t cntn_mutex;
    pthread_t s_thread;  
    int port;
    int dbglev;
    /**
     * Number if cntn instances */
    int inscnt;
    struct tag_handler *tag_handlers;
    void* userdata;
    struct modemd_publisher *publishers;
    struct stag_hook *stags;
};


struct xml_cntn_server *xml_cntn_server_create(void *userdata, int port, int inscnt, struct tag_handler *tag_handlers, int dbglev);


void xml_cntn_server_delete(struct xml_cntn_server *server);

int xml_cntn_server_start(struct xml_cntn_server *server);

int xml_cntn_server_stop(struct xml_cntn_server *server);

int xml_cntn_server_broadcast(struct xml_cntn_server *server,  struct xml_item *src);

#endif /* XMLCNTN_SERVER_H_ */
