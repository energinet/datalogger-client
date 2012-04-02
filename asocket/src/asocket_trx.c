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

#include "asocket_trx.h"

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
#include <stdarg.h>


#include "asocket_priv.h"


int asocket_cmd_stub(ASOCKET_FUNC_PARAM){
    
    printf("ack: %s\n",rx_msg->cmd );

    struct skcmd *tx_msg = asocket_cmd_create("Ack");

    asocket_cmd_param_add(tx_msg, rx_msg->cmd);

    asocket_con_snd(sk_con, tx_msg);

    asocket_cmd_delete(tx_msg);

    return 0;
}



/* struct asocket_cmd socket_cmd_list[] = { */
/*     { "status"   ,   asocket_cmd_stub, "Help text 1"}, */
/*     { "exit",  NULL , "Close the connection"},  */
/*     { NULL,   NULL } */
/* }; */


struct asocket_cmd *asocket_cmd_match(struct asocket_cmd *cmds, const char *name)
{
    struct asocket_cmd *cmdptr;
    cmdptr = cmds;

    while(cmdptr->cmd){
        if(strcmp(name,cmdptr->cmd)==0){                
            return cmdptr;
        }
        cmdptr++;
    }

    return NULL;
}


int asocket_cmd_run(struct asocket_con* sk_con, struct asocket_cmd *cmd, struct skcmd *msg)
{
    
    if(cmd->cmd && cmd->function){ /* command and function*/
        return cmd->function(sk_con, cmd, msg);
    } else if (cmd->cmd) { /* command but no function: EXIT */
        printf("Disconnecting\n");
        sk_con->run = 0;
    } else {
        /* print help text*/
        struct asocket_cmd *cmdptr = sk_con->cmds;
        printf("Help:\n");
        while(cmdptr->cmd){
            printf("%20s : %s\n", cmdptr->cmd, cmdptr->help);
            cmdptr++;
        }
    }

    return 0;
}


struct asocket_con* asocket_con_create(int skfd, struct asocket_cmd *cmds , void* appdata , int dbglev)
{
    struct asocket_con* sk_con = malloc(sizeof(*sk_con));
    assert(sk_con);

    memset(sk_con, 0, sizeof(*sk_con));

    sk_con->fd    = skfd;
    sk_con->cmds    = cmds;
    sk_con->appdata = appdata;
    sk_con->dbglev  = dbglev;

    pthread_mutex_init(&sk_con->tx_mutex , NULL);
    sk_con->run     =  1;

    return sk_con;
}

void asocket_con_remove(struct asocket_con* sk_con)
{
    //ToDo
    sk_con->run = 0;
    pthread_mutex_destroy(&sk_con->tx_mutex );
    free(sk_con->skaddr);
    free(sk_con);
}





int asocket_con_snd(struct asocket_con* sk_con, struct skcmd *msg) 
{
    //printf("snd: %s\n", msg->cmd);

    char *tx_buf =  malloc(SOCKET_TC_BUF_LEN);
    int tx_len = 0;
    int retval = 0;

    assert(tx_buf);

    tx_len = asocket_cmd_write(msg, tx_buf, SOCKET_TC_BUF_LEN);

    retval = send(sk_con->fd, tx_buf, tx_len, 0);   

    free(tx_buf);

    return retval;
}

int asocket_con_snd_vp(struct asocket_con* sk_con, const char *id , int param_cnt,  ...)
{
  va_list ap;
  int i, retval;
  struct skcmd* tx_msg = asocket_cmd_create(id);

  va_start (ap, param_cnt);
  for(i=0;i<param_cnt;i++){
      asocket_cmd_param_add(tx_msg, va_arg(ap,const char *));
  }
  va_end(ap);
  retval = asocket_con_snd(sk_con, tx_msg);
  asocket_cmd_delete(tx_msg);
  return retval;
  
}




int asocket_con_snd_nack(struct asocket_con* sk_con,const char *errmsg) 
{
    //ToDo
//    printf("nack: %s\n", errmsg);

    struct skcmd *tx_msg = asocket_cmd_create("NotAck");

    asocket_cmd_param_add(tx_msg, errmsg);

    asocket_con_snd(sk_con, tx_msg);

    asocket_cmd_delete(tx_msg);
    
    return 0;

}

int asocket_con_rcv(struct asocket_con* sk_con, struct skcmd **rx_msg)
{
    int rx_len = 0;
    char *rx_buf = malloc(SOCKET_TC_BUF_LEN);
    int retval = 0;
    char errmsg[128];
     
    assert(rx_buf);
    //asocket_cmd_init(rx_msg, NULL);

//    fprintf(stderr, "waiting\n");

    while(1){
        rx_len += recv(sk_con->fd, rx_buf+rx_len, (SOCKET_TC_BUF_LEN-1)-rx_len, 0);
    
        rx_buf[rx_len] = '\0';

        PRINT_DBG(sk_con->dbglev, "rx_len %d \"%s\"\n", rx_len, rx_buf);

        
        if(rx_len == 0){
	    PRINT_DBG(sk_con->dbglev, "EOF\n");
            sk_con->run = 0;
            goto out;
        } else if(rx_len <= 0){
            switch(errno){
              case EAGAIN: /* timeout */
/*                 if((retval = asocket_con_snd_vp(sk_con, "Alive",0))<0){ */
/*                     fprintf(stderr, "snd ret: %d\n", retval); */
/*                     sk_con->run = 0; */
/*                     goto out; */
/*                 } */
                rx_len = 0;
                continue;
              default:
                PRINT_ERR( "Socket returned: %d , %s (%d)\n", rx_len, strerror(errno), errno);
                break;
            }
            retval = -errno;
            sk_con->run = 0;
            goto out;
        } /* else { */
/*             break; */
/*         } */
//        fprintf(stderr, "cmd_len %d\n");

        retval = asocket_cmd_read(rx_msg, rx_buf, rx_len, errmsg);
        
        if(retval > 0){ 
            break;
        } else if(retval < 0){
            asocket_con_snd_nack(sk_con,errmsg);
            retval = 0;
            goto out;
        } 
    }

    PRINT_DBG(sk_con->dbglev, "cmd_read %d\n", retval);
     
  out:
    free(rx_buf);

    return retval;
}



int asocket_con_rcvr(struct asocket_con* sk_con) 
{
     int retval = 0;
     struct skcmd *rx_msg;
     struct asocket_cmd *cmd = NULL;
   
      struct timeval tv;
      //int val;
      
      tv.tv_sec = 60 ;
      tv.tv_usec = 0  ;

      setsockopt (sk_con->fd, SOL_SOCKET, SO_RCVTIMEO, (char 
*)&tv, sizeof tv);
     

     while(sk_con->run){

//         rx_msg = asocket_cmd_create("");

         retval = asocket_con_rcv(sk_con, &rx_msg);

         if(retval < 0){
             fprintf(stderr, "Connection error\n");
             break;
         } else if (retval == 0)
             continue;

         cmd = asocket_cmd_match(sk_con->cmds, rx_msg->cmd);

         if(!cmd){
             asocket_con_snd_nack(sk_con,"cmd not found");
             continue;
         }


         retval = asocket_cmd_run(sk_con, cmd, rx_msg);
         
         asocket_cmd_delete(rx_msg);

         if(retval < 0){
             printf("asocket_cmd_run returned\n");
             break;
         }
             
         
     }

     return retval;
}


int asocket_con_trancive(struct asocket_con* sk_con, struct skcmd* tx_msg, struct skcmd** rx_msg)
{
    int retval;
    PRINT_DBG(sk_con->dbglev, "sending\n");

    retval = asocket_con_snd(sk_con, tx_msg);
    PRINT_DBG(sk_con->dbglev, "retval %d receiving \n", retval);

    return asocket_con_rcv(sk_con, rx_msg);  

}
