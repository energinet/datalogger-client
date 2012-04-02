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

#include "licon_client.h"
#include "liabconnect.h"

#include <asocket_client.h>


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <netinet/in.h>
#include <asocket_trx.h>


struct asocket_con* sk_con = NULL;

int licon_client_connect(int port)
{
    int timeout = 100;

    struct sockaddr * skaddr = asocket_addr_create_in( "127.0.0.1", port);

    do{

	sk_con = asocket_clt_connect(skaddr);
	if(sk_con)
	    break;
	sleep(1);	
	fprintf(stderr, "Socket connect failed (wait)\n");
    }while(timeout--);

    free(skaddr);	
    
    if(!sk_con){
	fprintf(stderr, "Socket connect failed\n");
	return -errno;
    }



    return 0;
}


int licon_client_get_status(char *rx_buf, int rx_maxlen)
{
    
    struct skcmd* tx_msg = asocket_cmd_create("StatusGet");
    struct skcmd* rx_msg = NULL;
    
    asocket_con_trancive(sk_con, tx_msg, &rx_msg);

    asocket_cmd_delete(tx_msg);
    
    if(!rx_msg)
        return -EFAULT;

    if(!rx_msg->param)
        return -EFAULT;

    
    strncpy(rx_buf, rx_msg->param->param, rx_maxlen);

    asocket_cmd_delete(rx_msg);

    return strlen(rx_buf);

}

int licon_client_set_option(const char *appif, const char *appifname, const char *option, char *rx_buf, int rx_maxlen)
{
    struct skcmd* tx_msg = NULL;
    struct skcmd* rx_msg = NULL;

    if(strcmp(appif,"if")==0)
	tx_msg = asocket_cmd_create("IfOptionSet");
    else if(strcmp(appif,"app")==0)
	tx_msg = asocket_cmd_create("AppOptionSet");
    else 
	return -1;
    
    asocket_cmd_param_add(tx_msg, appifname);
    asocket_cmd_param_add(tx_msg, option);

    sk_con->dbglev = 1;
    fprintf(stderr, "set opt 1..\n");

    asocket_con_trancive(sk_con, tx_msg, &rx_msg);

    fprintf(stderr, "set opt 2\n");

     if(!rx_msg)
        return -EFAULT;

     fprintf(stderr, "set opt 3\n");

    if(!rx_msg->param)
        return -EFAULT;

    fprintf(stderr, "set opt 4\n");

    strncpy(rx_buf, rx_msg->param->param, rx_maxlen);

    fprintf(stderr, "set opt 5\n");

    asocket_cmd_delete(rx_msg);

    fprintf(stderr, "set opt 6\n");

    return strlen(rx_buf);
    
}


int  licon_client_disconnect(void)
{
    return asocket_clt_disconnect(sk_con);
}
