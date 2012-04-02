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

#include "asocket_server.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <assert.h>

#include "asocket.h"
#include "asocket_priv.h"
#include "asocket_trx.h"






/* int asckt_srv_icom(int client_fd, void* appdata, struct socket_cmd *cmds, int dbglevel)  */
/*  {  */
/*      int retval = 0; */
/*      int run = 1; */
/*      int rx_len = 0, tx_len = 0; */
/*      char *tx_buf = NULL; */
/*      char *rx_buf = NULL; */
/*      char *rx_cmd_ptr = NULL; */
/*      //struct skcmd rx_cmd; */
/*      struct socket_cmd *cmdptr; */


/*      tx_buf = malloc(SOCKET_TC_BUF_LEN); */

/*      assert(tx_buf); */
/* /\*      if(tx_buf == NULL) *\/ */
/* /\*          return -1; *\/ */

/*      rx_buf = malloc(SOCKET_TC_BUF_LEN); */

/*      assert(rx_buf); */
/* /\*      if(rx_buf == NULL){ *\/ */
/* /\*          free(tx_buf); *\/ */
/* /\*          return -1; *\/ */
/* /\*      } *\/ */
     
/*      while(run){ */
/* //         asocket_cmd_init(&rx_cmd, NULL); */
/*          rx_len = recv(client_fd, rx_buf, SOCKET_TC_BUF_LEN-1, 0); */
         
/*          tx_len = 0; */
         
/*          if(rx_len <= 0) */
/*              break; */

/*          printf("received %s\n", rx_buf); */
         
/*          rx_buf[rx_len] = '\0'; */
         
/*          cmdptr = cmds; */

/*          while(cmdptr->cmd){ */
/*              if((rx_cmd_ptr = strstr(rx_buf,cmdptr->cmd))!=NULL){                 */
/*                  break; */
/*              } */
/*              cmdptr++; */
/*          } */

/*          if(cmdptr->cmd && cmdptr->function){ /\* command and function*\/ */
/*              tx_len += sprintf(tx_buf+tx_len,"%s:\n", cmdptr->cmd); */
/*              tx_len += cmdptr->function(tx_buf+tx_len, SOCKET_TC_BUF_LEN-tx_len, appdata,rx_cmd_ptr,  rx_len ); */
             
/*              struct asocket_con* sk_con, struct asocket_cmd *cmd, struct skcmd* rx_msg */
/*              tx_len += cmdptr->function(tx_buf+tx_len, SOCKET_TC_BUF_LEN-tx_len, appdata,rx_cmd_ptr,  rx_len ); */
/*          } else if (cmdptr->cmd) { /\* command but no function: EXIT *\/ */
/*              tx_len += sprintf(tx_buf+tx_len,"closing\n"); */
/*              run = 0; */
/*          } else { */
/*              /\* print help text*\/ */
/*              cmdptr = cmds; */
/*              tx_len += sprintf(tx_buf+tx_len,"Help:\n"); */
/*              while(cmdptr->cmd){ */
/*                  tx_len += sprintf(tx_buf+tx_len,"%20s : %s\n", cmdptr->cmd, cmdptr->help); */
/*                  cmdptr++; */
/*              } */
/*          } */
/*          printf("tx_len %d: %s\n", tx_len, tx_buf); */

/*          send(client_fd, tx_buf, tx_len, 0);     */

/*      } */

/*      free(tx_buf); */
/*      free(rx_buf); */

/*      return retval; */

/*  }  */


void *asckt_srv_loop(void *param)
{
    struct socket_data *this = (struct socket_data*)param;
//    struct sockaddr *local = NULL;
    struct sockaddr_in  remote; //Todo
///    struct sockaddr_un local_un;
    int /*socket_fd,*/ client_fd;
   
    int retval ;
    int optval = 1;          /* setsockopt */
    socklen_t t;
    int len;

    /* open socket */
    if((this->socket_fd = socket(this->skaddr->sa_family, SOCK_STREAM, 0))<0){
        PRINT_ERR("server socket returned err: %s", strerror(errno));
        return NULL;
    }
     
    if (setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0){
        PRINT_ERR( "server setsockopt returned err: %s", strerror(errno));
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

    while(this->run){
        struct asocket_con* sk_con;
        t = sizeof(remote);

        if((client_fd = accept(this->socket_fd, (struct sockaddr *)&remote, &t))<0){
	    PRINT_ERR( "accept returned err: %s", strerror(errno));
            retval = -errno;
            goto out;
        }  
     
	PRINT_DBG(this->dbglev, "Socket: client connected...");

        sk_con = asocket_con_create(client_fd, this->cmds , this->appdata , this->dbglev);
	PRINT_DBG(this->dbglev, "Socket connection struct created");
        asocket_con_rcvr(sk_con);
	PRINT_DBG(this->dbglev, "Socket connection terminated");
        asocket_con_remove(sk_con);  
        close(client_fd);
	
	PRINT_DBG(this->dbglev, "Socket: Connection closed");

    }
  out:
    close(this->socket_fd);
    this->socket_fd = -1;
    return NULL;

}








int asckt_srv_start(struct socket_data *param)
{
    param->run = 1;

    return pthread_create( & param->thread, NULL, asckt_srv_loop, (void *)param);
}


void asckt_srv_close(struct socket_data *param){

    int retval;

    PRINT_DBG(param->dbglev, "closing socket %d",param->socket_fd);   
    param->run = 0;

    if (param->socket_fd >= 0)
        close(param->socket_fd);

    PRINT_DBG(param->dbglev, "socket closed");

    pthread_kill(param->thread, SIGHUP); 

    PRINT_DBG(param->dbglev, "waiting for pthread_join");
        
    if((retval = pthread_join(param->thread, NULL))<0){
        PRINT_ERR("Error: pthread_join returned %d : %s", retval, strerror(retval));
    } 

    PRINT_DBG(param->dbglev, "pthread_join'ed");
    

    return;

}
