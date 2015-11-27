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
#include "xml_cntn_fd.h"
#include "xml_cntn_gen.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>
#include <sys/socket.h>



struct xml_cntn_fd * xml_cntn_fd_create(int *fd)
{
	int retval;
	struct xml_cntn_fd *new = malloc(sizeof(*new));
	assert(new);
	memset(new, 0, sizeof(*new));

	new->fd = fd;
	new->run = 1;
	*new->fd = -1;
	new->ready = 0;

	if((retval = pthread_mutex_init(&new->mutex, NULL))!=0){
		PRINT_ERR("Failure in pthread_mutex_init tx_mutex_buf %p: %s ", new, strerror(retval));  
		return NULL;
	}

	if((retval = pthread_cond_init(&new->cond, NULL))!=0){
		PRINT_ERR("Failure in pthread_cond_init %p: %s ", new, strerror(retval));  
		return NULL;
	}
	
	

	return new;

}



void xml_cntn_fd_delete(struct xml_cntn_fd *cnfd)
{
	pthread_mutex_lock(&cnfd->mutex);
	cnfd->run=0;
	pthread_cond_broadcast(&cnfd->cond);
	pthread_mutex_unlock(&cnfd->mutex);


	pthread_cond_destroy(&cnfd->cond);
    pthread_mutex_destroy(&cnfd->mutex);
	free(cnfd);
}



int xml_cntn_fd_wait(struct xml_cntn_fd *cnfd){

	int retval = 0;
	cnfd->ready = 0;

	while((cnfd->ready == 0)&&(cnfd->run==1)){
		if((retval = pthread_cond_wait(&cnfd->cond, &cnfd->mutex))!=0){
			PRINT_ERR("ERROR-> pthread_cond_wait failed: %s (%d) in %p",strerror(retval), retval,  cnfd );
			return -retval;
		}
	}

	if(cnfd->run!=1){
		retval = -1;
	}

	return retval;
}

int xml_cntn_fd_open(struct xml_cntn_fd *cnfd, int client_fd)
{
	int retval = 0;

	pthread_mutex_lock(&cnfd->mutex);
	
	if(*cnfd->fd == -1){
		*cnfd->fd = client_fd;
		cnfd->ready = 1;
		pthread_cond_broadcast(&cnfd->cond);
	} else {
		retval = -1;
	}

	pthread_mutex_unlock(&cnfd->mutex);

	return retval;
}


int xml_cntn_fd_close(struct xml_cntn_fd *cnfd)
{
	pthread_mutex_lock(&cnfd->mutex);

	if(*cnfd->fd != -1)
		close(*cnfd->fd);

	*cnfd->fd = -1;
	
	pthread_mutex_unlock(&cnfd->mutex);
	
	return 0;
}


int xml_cntn_fd_isactive(struct xml_cntn_fd *cnfd)
{
	int active = 0;

	if(pthread_mutex_trylock(&cnfd->mutex)!=0){
		return 1;
	}
	
	if(cnfd->ready)
		active = 1;

	pthread_mutex_unlock(&cnfd->mutex);

	return active;		

}
