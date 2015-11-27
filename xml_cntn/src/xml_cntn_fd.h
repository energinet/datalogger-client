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

#ifndef XML_CNTN_FD_H_
#define XML_CNTN_FD_H_
#include <sys/types.h>
#include <pthread.h>

/**
 * connection fd handler 
 */
struct xml_cntn_fd{
	/**
	 * Run flag */
	int run;
	/**
	 * Pointer to the fd
	 */
	int *fd;

	/**
	 * Indicates if the file descriptor is ready for use */
	int ready;

	/**
	 * Mutex for the tx buffer */
    pthread_mutex_t mutex;
 
	/**
	 * Condition var for the tx buffer */
    pthread_cond_t cond;
};


/**
 * Create a fd handler 
 */
struct xml_cntn_fd * xml_cntn_fd_create(int *fd);

/**
 * Delete a fd handler 
 */
void xml_cntn_fd_delete(struct xml_cntn_fd *cnfd);

/**
 * Wait for a fd to be opened 
 * @note Used in the receive og transmit loop 
 * @return 0 on new fd and negative on close or error
 */
int xml_cntn_fd_wait(struct xml_cntn_fd *cnfd);

/**
 * Open a new fd 
*/
int xml_cntn_fd_open(struct xml_cntn_fd *cnfd, int client_fd);

/**
 * Close a fd 
 */
int xml_cntn_fd_close(struct xml_cntn_fd *cnfd);

/**
 * Test if a fd is in use 
 */
int xml_cntn_fd_isactive(struct xml_cntn_fd *cnfd);



#endif /* XML_CNTN_FD_H_ */



