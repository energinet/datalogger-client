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
#include <sys/types.h>
#include <unistd.h>

#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s: "str"\n",__FUNCTION__, ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s[%ld]: "str"\n",__FUNCTION__,(long int)syscall(__NR_gettid), ## arg);}}


#include <linux/unistd.h>

#include <pthread.h>
#include <xml_serialize.h>
#include "xml_cntn_sub.h"

#include "xml_cntn_tx.h"

struct spxml_cntn;

struct tag_handler{
    const char *tag;
    int (*function)(struct spxml_cntn *cntn, struct xml_item *item );
};


struct spxml_cntn{
	/**
	 * 1 if running 0 if stoppting or not running */
    int run;
	/**
	 * File descriptor to socket */
    int fd;
	
	/**
	 * Connection fd handler */
	struct xml_cntn_fd *cnfd;

	/**
	 * Transmit handeler 
	 */
	struct xml_cntn_tx *txobj;

	/**
	 * Receive stack object  */
    struct xml_stack *rx_stack;
	/**
	 * Receive document */
    struct xml_doc *rx_doc;
	
	pthread_mutex_t mutex; 

    pthread_t rx_thread;   

    struct tag_handler *tag_handlers;

	/**
	 * Subscribes */
	struct xmc_cntn_sub *subcs;
	int active;
    int reset_on_full;
    void *userdata;
	struct sockaddr *skaddr;
    int dbglev;

};

struct spxml_cntn_list{
	int size;
	struct spxml_cntn **cntns;
};


struct spxml_cntn_list *spxml_cntn_list_create(int size, struct tag_handler *tag_handlers, void *userdata, int dbglev );

void spxml_cntn_list_delete(struct spxml_cntn_list *list);

int spxml_cntn_list_add(struct spxml_cntn_list *list, struct spxml_cntn *cntn);

int spxml_cntn_list_clean(struct spxml_cntn_list *list);

struct spxml_cntn *spxml_cntn_list_next(struct spxml_cntn_list *list, int *index);
int spxml_cntn_list_activecnt(struct spxml_cntn_list *list);
int spxml_cntn_list_open(struct spxml_cntn_list *list, int client_fd);

/**
 * Create a connection
 */
struct  spxml_cntn*  spxml_cntn_create(int client_fd, struct tag_handler *tag_handlers, void *userdata, int dbglev);


/**
 * Delete a connection 
 */
void spxml_cntn_delete(struct spxml_cntn *cntn);


/**
 * Send an error to a connection
 */
int spxml_cntn_tx_add_err(struct spxml_cntn *cntn, const char *errstr, const char *seq);

/**
 * Reserve the connection for adding items 
 * @note 
 * @param cntn The connection to reserve 
 * @return The xml document which to write to og NULL if failed
 */
struct xml_doc* spxml_cntn_tx_add_reserve(struct spxml_cntn *cntn);

/**
 * Commit to a reserved connection 
 * @note to use this function the connection must first be reserved by @ref spxml_cntn_tx_add_reserve before using commiting data.
 * @param cntn The connection to commit to
 */
void spxml_cntn_tx_add_commit(struct spxml_cntn *cntn);


/**
 * Add a publisher 
 */
void spxml_cntn_publs_add(struct spxml_cntn *cntn, const void *publisher);



int spxml_cntn_publs_sub(struct spxml_cntn *cntn, const void *publisher);

int spxml_cntn_open(struct spxml_cntn *cntn, int client_fd);
int spxml_cntn_close(struct spxml_cntn *cntn);

/**
 * Send an item to a connection 
 */
int spxml_cntn_send(struct spxml_cntn *cntn, struct xml_item *src, const char *seq);




#endif /* XMLCNTN_H_ */
