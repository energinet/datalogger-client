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
#include "xml_cntn_server.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>

#include "xml_cntn.h"

struct xml_cntn_server *xml_cntn_server_create(void *userdata, int port, int inscnt, struct tag_handler *tag_handlers, int dbglev)
{
	struct xml_cntn_server *new = malloc(sizeof(*new));
	assert(new);

	memset(new, 0, sizeof(*new));
        
        new->tag_handlers = tag_handlers;
        new->inscnt = inscnt;
	new->userdata = userdata;
	new->port   = port;
	new->stags  = NULL;
	new->dbglev = dbglev;
	
	return new;

}

void modemd_server_delete(struct xml_cntn_server *server)
{
	free(server);
}


/**
 * Accept incomming connection 
 */
int xml_cntn_server_accept(struct xml_cntn_server* server, int fd)
{
    int client_fd;
    char remote_str[512];
    struct sockaddr_in  remote; //Todo
    socklen_t t;
    int retval = 0;
    int number;

    t = sizeof(remote);
            
    if((client_fd = accept(fd, (struct sockaddr *)&remote, &t))<0){
	PRINT_ERR( "accept returned err: %s", strerror(errno));
        retval = -errno;
        goto out;
    }
    
    PRINT_DBG(server->dbglev, "Socket: client '%s' connected...", inet_ntop(AF_INET, &remote,  remote_str, sizeof(remote_str)));
            
    if(pthread_mutex_lock(&server->cntn_mutex)!=0)
        goto out;
    
    number = spxml_cntn_list_open(server->cnlst, client_fd);
    PRINT_DBG(server->dbglev, "Socket opened %d", number);
    PRINT_DBG(server->dbglev, "Socket active connections %d",  spxml_cntn_list_activecnt(server->cnlst));
    pthread_mutex_unlock(&server->cntn_mutex);

 out:
    return retval;
}

/**
 * Server loop 
 */
void *xml_cntn_server_loop(struct socket_data *this) 
{ 
    struct xml_cntn_server* server = (struct xml_cntn_server*)this->appdata;
    int retval = 0;
    int optval = 1;          /* setsockopt */
    int len;
    int errors = 0;

    while(this->run){
	/* open socket */
        if((this->socket_fd = socket(this->skaddr->sa_family, SOCK_STREAM, 0))<0){
            PRINT_ERR("server socket returned err: %s", strerror(errno));
            errors++;
            sleep(1);
            continue;
        }
	
        if (setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR,
                       (const void *)&optval , sizeof(int)) < 0){
            PRINT_ERR("server setsockopt returned err: %s", strerror(errno));
            retval = -errno;
            goto out;
        }
	
        len =  asocket_addr_len(this->skaddr);
	
        if(bind(this->socket_fd, this->skaddr, len)<0){
            PRINT_ERR("server bind returned err: %s", strerror(errno));
            retval = -errno;
            goto out;
        }
	
        if(listen(this->socket_fd, 1024)<0){
            PRINT_ERR("server listen returned err: %s", strerror(errno));
            retval = -errno;
            goto out;
        }
	
        PRINT_DBG(this->dbglev, "Socket opened...");
	
        while(this->run && retval >= 0){
            retval = xml_cntn_server_accept(server, this->socket_fd);
        }
    }
 out:
    PRINT_DBG(this->dbglev, "Socket returned %d", retval);
    close(this->socket_fd);
    this->socket_fd = -1;
    return NULL;

}


void *xml_cntn_server_ploop(void *param) 
{
	struct socket_data *this = (struct socket_data*)param;
	
	xml_cntn_server_loop(this);

	pthread_exit(NULL);
}




int xml_cntn_server_start( struct xml_cntn_server* server)
{
    int retval = 0;
    struct socket_data *param =  &server->sparm;
    PRINT_DBG(server->dbglev, "starting cntns\n");
    server->cnlst = spxml_cntn_list_create(server->inscnt,server->tag_handlers, server->userdata, server->dbglev );
    
    param->appdata = (void*)server;
    param->skaddr = asocket_addr_create_in(NULL,  server->port);
    param->cmds = NULL;
    param->dbglev = server->dbglev;
    param->run = 1;
    
    if((retval = pthread_create(&server->s_thread, NULL, xml_cntn_server_ploop, param))<0){
        PRINT_ERR("Failure starting loop in %p: %s ", server, strerror(retval));
        return retval;
    } 
    
    return 0;
}


int xml_cntn_server_stop(struct xml_cntn_server* this)
{

    void* retptr;
    int ret = -EFAULT;
    struct socket_data *param =  &this->sparm;
    fprintf(stderr, "stopping s_tread\n");
    param->run = 0;
    spxml_cntn_list_delete(this->cnlst);

    close(param->socket_fd);
    
    ret = pthread_join(this->s_thread, &retptr);
    if(ret < 0)
        fprintf(stderr, "pthread_join returned %s", strerror(-ret));
    
    pthread_mutex_lock(&this->cntn_mutex);
    
    return 0;
}


int xml_cntn_server_broadcast(struct xml_cntn_server *server,  struct xml_item *src)
{
    int count = 0;
    int index = 0;
    struct spxml_cntn *cntn;

    pthread_mutex_lock(&server->cntn_mutex);
    while((cntn = spxml_cntn_list_next(server->cnlst, &index))!=NULL){
        if(spxml_cntn_send(cntn, src, NULL)<0) 
            PRINT_ERR("Could not add event to cntn %p", cntn); 
        count++;
    } 
    
    pthread_mutex_unlock(&server->cntn_mutex);

    return count;
}
