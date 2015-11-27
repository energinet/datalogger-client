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

#include "asocket_client.h"

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
#include <arpa/inet.h>
#include <asocket_trx.h>
#include <netinet/tcp.h>





struct asocket_con* asocket_clt_connect(struct sockaddr *skaddr)
{
    int client_fd = asocket_clt_cnt_fd(skaddr,1);

    if(client_fd < 0)
	return NULL;

    return asocket_con_create(client_fd, NULL , NULL , 0);;
}



int asocket_clt_cnt_fd(struct sockaddr *skaddr, int retcnt){
    
    int len;
    int retval;
    int client_fd;
    int errors = 0;

    while (errors < retcnt) {
	/* open socket */
	if ((client_fd = socket(skaddr->sa_family, SOCK_STREAM, 0)) < 0) {
	    fprintf(stderr, "server socket returned err: %s\n", strerror(errno));
	    errors++;
	    sleep(1);
	    continue;
	}
	
	len = asocket_addr_len(skaddr);
	
	retval = connect(client_fd, skaddr, len);
	
	if (retval < 0) {
	    close(client_fd);
	    errors++;
	    sleep(1);
	    continue;
	} else {
	    break;
	}
	
    }
    
    if (errors >= retcnt){
	fprintf(stderr, "connect error: %s (%d)\n", strerror(errno), errno);		
	return -1;
    }
    
    if(skaddr->sa_family == AF_INET){
        int val;
        val = 10; /* 10 sec before starting probes */
        retval = setsockopt (client_fd, SOL_SOCKET, SO_KEEPALIVE , &val, sizeof (val));
        //   printf("SO_KEEPALIVE ret %d\n", retval);
        
        val = 10; /* 10 sec before starting probes */
        retval = setsockopt (client_fd, SOL_TCP, TCP_KEEPIDLE, &val, sizeof (val));
        // printf("TCP_KEEPIDLE ret %d\n", retval);
        
        val = 2; /* 2 probes max */
        retval = setsockopt (client_fd, SOL_TCP, TCP_KEEPCNT, &val, sizeof (val));
        // printf("TCP_KEEPCNT ret %d\n", retval);
        
        val = 10; /* 10 seconds between each probe */
        retval = setsockopt (client_fd, SOL_TCP, TCP_KEEPINTVL, &val, sizeof (val));
        // printf("TCP_KEEPINTVL ret %d\n", retval);
    }

    return client_fd;

}



/* int asckt_clt_trancive(struct asocket_client *client,  */
/*                        char *tx_buf, int tx_len,  */
/*                        char *rx_buf, int rx_maxlen){ */
/*     int retval = 0; */
    
/*     retval = send(client->socket_fd, tx_buf, tx_len , 0);     */

/*     if(retval < 0){ */
/*         fprintf(stderr, "Error: in send : %s (%d)",strerror(errno), errno); */
/*         return -errno; */
/*     } */


/*     retval = recv(client->socket_fd, rx_buf , rx_maxlen, 0); */

    
/*     if(retval < 0){ */
/*         fprintf(stderr, "Error: in rcev : %s (%d)",strerror(errno), errno); */
/*         return -errno; */
/*     } */

/* //    printf("received %d: %s \n", retval, rx_buf); */
    

/*     return retval; */

/* } */

/* int asckt_clt_run(struct asocket_client *client,  */
/*                   const char *cmd, const char *payload, */
/*                   char *rx_buf, int rx_maxlen) */
/* { */
/*     char tx_buf[1024]; */
/*     int tx_len; */

/*     if(payload) */
/*         tx_len = sprintf(tx_buf, "%s:%s", cmd, payload); */
/*     else  */
/*         tx_len = sprintf(tx_buf, "%s:", cmd); */

/*     printf("sending %s\n", tx_buf); */

/*     return asckt_clt_trancive(client, tx_buf, tx_len, rx_buf, rx_maxlen); */
    
/* } */


int asocket_clt_disconnect(struct asocket_con* sk_con)
{
    
    close(sk_con->fd);

    asocket_con_remove(sk_con);  
    return 0;
}
