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

#ifndef XML_CNTN_TX_H_
#define XML_CNTN_TX_H_
#include <sys/types.h>

#include <pthread.h>
#include <xml_serialize.h>
#include "xml_cntn_fd.h"

struct xml_cntn_tx{
	/**
	 * Run flag */
    int run;

	/**
	 * File descriptor to socket */
    int fd;

	/**
	 * Connection fd handler */
	struct xml_cntn_fd *cnfd;

	/**
	 * Pointer to the tx doc for filling  */
    struct xml_doc *tx_doc;
	/**
	 * Pointer to the tx doc in transmit  */
    struct xml_doc *tx_doc_buf;
	/**
	 * Transmit doc 1 */
    struct xml_doc *tx_doc1;
	/**
	 * Transmit doc 2 */
    struct xml_doc *tx_doc2;

	/**
	 * Mutex for the tx buffer */
    pthread_mutex_t tx_mutex_buf;
 
	/**
	 * Attributes for tx buffer mutex */
	pthread_mutexattr_t tx_mutex_attr;

	/**
	 * Condition var for the tx buffer */
    pthread_cond_t tx_cond;

	/**
	 * The transmit threadv */
    pthread_t tx_thread;   

	/**
	 * Active flag */
	int active;

	/**
	 * Debug level */
	int dbglev;

	int reset_on_full;

};


/**
 * Create transmit connection
 */
struct  xml_cntn_tx*  xml_cntn_tx_create(int client_fd, int dbglev);

/**
 * Delete transmit connection
 */
void xml_cntn_tx_delete(struct  xml_cntn_tx* txcn);


/**
 * Reserve the connection for adding items 
 * @note 
 * @param cntn The connection to reserve 
 * @return The xml document which to write to og NULL if failed
 */
struct xml_doc* xml_cntn_tx_add_reserve(struct xml_cntn_tx *txcn);

/**
 * Commit to a reserved connection 
 * @note to use this function the connection must first be reserved by @ref spxml_cntn_tx_add_reserve before using commiting data.
 * @param cntn The connection to commit to
 */
void xml_cntn_tx_add_commit(struct xml_cntn_tx *txcn);


/**
 * Open a new connection 
 */
int xml_cntn_tx_open(struct xml_cntn_tx *txcn, int client_fd);

/**
 * Close a connection 
 */
int xml_cntn_tx_close(struct xml_cntn_tx *txcn);

/**
 * Check if there is more the trasmit in buffers
*/
int xml_cntn_tx_isactive(struct xml_cntn_tx *txcn);


#endif /* XML_CNTN_TX_H_ */

