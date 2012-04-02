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

#ifndef XMLCNTN_H_
#define XMLCNTN_H_


#include <xml_serialize.h>
#include <module_base.h>

struct spxml_cntn;

struct tag_handler{
    const char *tag;
    int (*function)(struct spxml_cntn *cntn, struct xml_item *item );
};

struct spxml_cntn{
    int run;
    int fd;
    struct xml_doc *tx_doc;
    struct xml_doc *tx_doc_buf;
    struct xml_doc *tx_doc1;
    struct xml_doc *tx_doc2;
    struct xml_stack *rx_stack;
    struct xml_doc *rx_doc;
    pthread_mutex_t tx_mutex_buf;
    pthread_mutex_t tx_mutex;
    pthread_cond_t tx_cond;
    pthread_t tx_thread;   
    pthread_t rx_thread;   
    struct spxml_cntn *next;
    struct tag_handler *tag_handlers;
    void *userdata;
    int reset_on_full;
    int dbglev;
};


struct  spxml_cntn*  spxml_cntn_create(int client_fd, struct tag_handler *tag_handlers, void *userdata, int dbglev);

void spxml_cntn_delete(struct spxml_cntn *cntn);

struct spxml_cntn* spxml_cntn_add(struct spxml_cntn *list ,struct spxml_cntn *new);

struct spxml_cntn* spxml_cntn_rem(struct spxml_cntn *list ,struct spxml_cntn *rem);

struct spxml_cntn* spxml_cntn_clean(struct spxml_cntn *list);


int spxml_cntn_tx_add_err(struct spxml_cntn *cntn, const char *errstr, const char *seq);


struct xml_doc* spxml_cntn_tx_add_reserve(struct spxml_cntn *cntn);

void spxml_cntn_tx_add_commit(struct spxml_cntn *cntn);


#endif /* XMLCNTN_H_ */
