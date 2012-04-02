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

#include "siteutil.h"

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

#include <qDecoder.h>
#include <asocket_trx.h>
#include <asocket_client.h>

#define DEFAULT_RPC_PORT 4568

struct sitemenu_item sitemenu[] = {				\
    { "Styrdinvarmepumpe.dk", "http://styrdinvarmepumpe.dk/" , NULL },	\
    { "Live", "live.cgi", "live" },					\
    { "Kurver", "grafer.cgi" , "graf" },				\
    { "Bokstilknytning", "pair.cgi" , "pair" },				\
    { "Flowkalibrering", "calib.cgi" , "calib" },			\
    { NULL, NULL, NULL }						\
};

struct asocket_con* siteutil_rpc_connect(void)
{
    struct asocket_con* sk_con = NULL;
    struct sockaddr * skaddr = asocket_addr_create_in( "127.0.0.1", DEFAULT_RPC_PORT);

    sk_con = asocket_clt_connect(skaddr);

    free(skaddr);

    if(!sk_con){
        fprintf(stderr, "Socket connect failed\n");
        return NULL;
    }

    return sk_con;
}


int siteutil_rpc_disconnect(struct asocket_con* sk_con)
{
    if(!sk_con)
	return -1;

    return asocket_clt_disconnect(sk_con);
}



int siteutil_rpc_nameget(struct asocket_con* sk_con, char **name, char **localid)
{
    struct skcmd* tx_msg = asocket_cmd_create("LocalIdGet");
    struct skcmd* rx_msg = NULL;
    int retval = 1;

    if(!sk_con){
	asocket_cmd_delete(tx_msg);
	return -1;
    }

    asocket_con_trancive(sk_con, tx_msg, &rx_msg);

    if(!rx_msg){
//	pair_client_error("???", NULL, SOLVE_INTERNET);
	asocket_cmd_delete(tx_msg);
	return -1;
    }
	
    if(strcmp(asocket_cmd_param_get(rx_msg, 0), "1")==0){
	if(asocket_cmd_param_get(rx_msg, 2))
	    *localid = strdup(asocket_cmd_param_get(rx_msg, 2));
	retval = 2;
    } else {
	retval = 1;
    }
    
    if(asocket_cmd_param_get(rx_msg, 1))
	*name = strdup(asocket_cmd_param_get(rx_msg, 1));

    asocket_cmd_delete(rx_msg);
    asocket_cmd_delete(tx_msg);

    return retval;
    
}


int siteutil_topmenu(const char *sname, char * buf, size_t maxlen)
{
    struct sitemenu_item *ptr = sitemenu;
    struct sitemenu_item *ptr_nxt = ptr+1;
    int len = 0;

    len += snprintf(buf+len, maxlen-len, "<ul class=\"menu\">\n");
    
    while(ptr->name){
	ptr_nxt = ptr+1;
	if(ptr == sitemenu)
	    len += snprintf(buf+len, maxlen-len, "<li class=\"leaf first\">\n");
	else if (ptr_nxt->name == NULL)
	    len += snprintf(buf+len, maxlen-len, "<li class=\"leaf last\">\n");
	else
	    len += snprintf(buf+len, maxlen-len, "<li class=\"leaf \">\n");

	if(sname &&  ptr->sname && strcmp(sname, ptr->sname)==0)
	    len += snprintf(buf+len, maxlen-len, 
			    "<a href=\"%s\" title=\"%s\"><span style=\"color:black\">"
			    "%s</span></a></li>", 
			    ptr->link, ptr->name, ptr->name); 
	else
	    len += snprintf(buf+len, maxlen-len, "<a href=\"%s\" title=\"%s\">%s</a></li>", 
			    ptr->link, ptr->name, ptr->name); 
	ptr = ptr_nxt;
    }
    
    len += snprintf(buf+len, maxlen-len, "</ul>");

    return len;

}

int siteutil_init(struct sitereq *site, char *subtitle, char *sname)
{
    char buf[2048];

    memset(site, 0, sizeof(*site));
    site->req = qCgiRequestParse(NULL);
    site->sname = sname;
    site->subtitle = subtitle;
    site->socket_rp = siteutil_rpc_connect();
    siteutil_rpc_nameget(site->socket_rp, &site->boxid, &site->localid);
    
    sprintf(buf, "text/html;charset=UTF-8");
    qCgiResponseSetContentType(site->req, buf);

    return 0;
}


void siteutil_deinit(struct sitereq *site)
{
    
    Q_ENTRY *req = site->req;
    req->free(req);
    siteutil_rpc_disconnect(site->socket_rp);

}


void siteutil_top(struct sitereq *site,  int refresh)
{

    /* Page header */
    Q_ENTRY *args = qEntry();
    char menu[1024];
    char refrshstr[1024];
    Q_ENTRY *req = site->req;

    siteutil_topmenu(site->sname,menu,sizeof(menu));
    
    args->putStr(args, "${PAGE_TOPMENU}", menu, true);
    args->putStr(args, "${PAGE_TITLE}", "Styreboks", true);
    args->putStr(args, "${PAGE_SUBTITLE}", site->subtitle, true);
    if(site->simpel)
	args->putStr(args, "${PAGE_TOPIMG}", "", true);    
    else
	args->putStr(args, "${PAGE_TOPIMG}", "<img src=\"../logo.png\" alt=\"Hjem\" id=\"logo-image\">", true);    

    if(refresh){
	sprintf(refrshstr, "<META HTTP-EQUIV=\"refresh\" CONTENT=\"%d\" />", refresh);
	args->putStr(args, "${PAGE_EXTHDR}", refrshstr, true);
    } else {
	args->putStr(args, "${PAGE_EXTHDR}", "", true);
    }
    
    if(qSedFile(args, "/var/www/htdocs/page.begin.html.in", stdout) == 0) 
	qCgiResponseError(req, "File(%s) not found.", "/var/www/htdocs/page.begin.html.in");

    args->free(args);

}


void siteutil_bot(struct sitereq *site)
{
    Q_ENTRY *req = site->req;
    Q_ENTRY *args = qEntry();
    char id_buf[512]; 
    int len = 0;
    int maxlen = sizeof(id_buf);

    len += snprintf(id_buf+len,maxlen-len,  "Box-ID: %s", site->boxid);

    if(site->localid)
	len += snprintf(id_buf+len,maxlen-len,  ", Installations-ID: %s", site->localid);
    
    args->putStr(args, "${PAGE_BOTTAG}", id_buf, true);

    if(qSedFile(args, "/var/www/htdocs/page.end.html.in", stdout) == 0) 
	qCgiResponseError(req, "File(%s) not found.", "/var/www/htdocs/page.end.html.in");

    args->free(args);

}
