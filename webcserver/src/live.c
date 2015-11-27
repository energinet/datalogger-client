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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <syslog.h>
#include <asocket_trx.h>
#include <asocket_client.h>
#include <qDecoder.h>
#include <errno.h>
#include "siteutil.h"

#define DEFAULT_PORT 6523

struct asocket_con* sk_con = NULL;

int live_client_connect(void)
{
    struct sockaddr * skaddr = asocket_addr_create_in( "127.0.0.1", DEFAULT_PORT);

    sk_con = asocket_clt_connect(skaddr);

    free(skaddr);

    if(!sk_con){
        fprintf(stderr, "Socket connect failed\n");
        return -errno;
    }

    return 0;
}


int live_client_disconnect(void)
{
    return asocket_clt_disconnect(sk_con);
}


int live_client_upd(const char *flags, const char *name)
{
    struct skcmd* tx_msg = asocket_cmd_create("ReadsUpdate");
    struct skcmd* rx_msg = NULL;

    if(!flags)
	flags = "show";
    
    if(!name)
	name = "*.*";
    
    asocket_cmd_param_add(tx_msg, flags);  
    asocket_cmd_param_add(tx_msg, name);  

    asocket_con_trancive(sk_con, tx_msg, &rx_msg);

    asocket_cmd_delete(tx_msg);
    
    if(!rx_msg)
        return -EFAULT;

    asocket_cmd_delete(rx_msg);
    
    return 0;
} 


int live_client_print_line(struct sitereq *site)
{
    Q_ENTRY *req = site->req;
    char *mask = (char *)req->getStr(req, "show", false);
    char *name = (char *)req->getStr(req, "name", false); 
    
    struct skcmd* tx_msg = asocket_cmd_create("ReadGet");
    struct skcmd* rx_msg = NULL;

    live_client_connect();
    live_client_upd(mask, name);

    printf("<table class=\"sticky-enabled\">\n"
	   "<thead><tr><th>Text</th><th>Værdi</th><th>Enhed</th></tr></thead>\n"
	   "<tbody>");

    while(1){
	asocket_con_trancive(sk_con, tx_msg, &rx_msg);
	if(!rx_msg)
	    break;
	
	if(!rx_msg->param)
	    break;
	
	printf(" <tr class=\"odd\" ><td style=\"width: 150px;\">%s</td><td style=\"text-align: right; width: 50px;\">%s</td><td>%s</td></tr>\n",
	       asocket_cmd_param_get(rx_msg, 1),
	       asocket_cmd_param_get(rx_msg, 2), asocket_cmd_param_get(rx_msg, 3)  );

	asocket_cmd_delete(rx_msg);
    
    }

    printf("</table>\n");

    asocket_cmd_delete(tx_msg);

    live_client_disconnect();
    return 0;
}




int main(int argc, char *argv[])
{

    struct sitereq site;
	
    openlog("live.cgi", LOG_PID, LOG_DAEMON/*|LOG_PERROR*/);

	syslog(LOG_ERR, "Done");

    siteutil_init(&site, "Udlæsninger", "live");
    
    syslog(LOG_DEBUG, "Reading configuration file...");
    
	struct siteadd *header = siteadd_create("<META HTTP-EQUIV=\"refresh\" CONTENT=\"3\" />");

    siteutil_top(&site, header);

	siteadd_delete(header);

    live_client_print_line(&site);

    siteutil_bot(&site);

    siteutil_deinit(&site);

    syslog(LOG_DEBUG, "Done");
    return EXIT_SUCCESS;

}
