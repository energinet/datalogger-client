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

#ifndef XML_CNTN_RCVR_H_
#define XML_CNTN_RCVR_H_

#include "pthread.h"
#include "xml_cntn.h"

enum xml_cntn_subcl_type{
	SCLTYPE_SEQ,
	SCLTYPE_SUBS,
};

/**
 * Connection sub client 
 */
struct xml_cntn_subcl {
	enum xml_cntn_subcl_type type;
	int rcvcnt;
	char *seqtag;
	int active;
	struct xml_item *item;
	struct xml_cntn_subcl *next;
};

struct xml_cntn_subcl *xml_cntn_subcl_create(enum xml_cntn_subcl_type type, const char *tag);

struct xml_cntn_subcl *xml_cntn_subcl_create_seq();

struct xml_cntn_subcl *xml_cntn_subcl_create_subs(const char *tag);

void xml_cntn_subcl_delete(struct xml_cntn_subcl *subcl);

struct xml_cntn_subcl *xml_cntn_subcl_list_add(struct xml_cntn_subcl *list, struct xml_cntn_subcl *new);

struct xml_cntn_subcl *xml_cntn_subcl_list_rem(struct xml_cntn_subcl *list, struct xml_cntn_subcl *rem);

void xml_cntn_subcl_txprep(struct xml_doc* doc, struct xml_item *item, struct xml_cntn_subcl *subcl);

int xml_cntn_subcl_match(struct xml_cntn_subcl *subcl, const char *seq, const char *tag);

void xml_cntn_subcl_activate(struct xml_cntn_subcl* subcl, unsigned long seq);

void xml_cntn_subcl_cancel(struct xml_cntn_subcl* subcl);


/**
 * Connection receiver object 
 */
struct xml_cntn_rcvr {
	unsigned long seq;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	struct xml_cntn_subcl *subcllst;
};

struct xml_cntn_rcvr *xml_cntn_rcvr_create();

void xml_cntn_rcvr_delete(struct xml_cntn_rcvr *rcvr);

int xml_cntn_rcvr_subcl_activate(struct xml_cntn_rcvr *rcvr, struct xml_cntn_subcl* subcl);

int xml_cntn_rcvr_subcl_cancel(struct xml_cntn_rcvr *rcvr, struct xml_cntn_subcl* subcl);

struct xml_item *xml_cntn_rcvr_subcl_wait(struct xml_cntn_rcvr *rcvr, struct xml_cntn_subcl* subcl, int timeout);


int xml_cntn_rcvr_tag(struct spxml_cntn *cntn, struct xml_item *item);



#endif /* XML_CNTN_RCVR_H_ */


