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
#include <assert.h>
#include <qDecoder.h>
#include <asocket_trx.h>
#include <asocket_client.h>

#include "ethmacget.h"

#define DEFAULT_RPC_PORT 4568

struct sitemenu_item sitemenu[] = {				
#ifndef SDVP
    { "LIAB DACS", "http://dacs.liab.dk/" , NULL },	
#else
    { "Styrdinvarmepumpe.dk", "http://styrdinvarmepumpe.dk/" , NULL },	
#endif
    { "Live", "live.cgi", "live" },					
#ifndef SDVP
    { "Kommandoer", "cmd.cgi", "cmd" },					
#endif
    { "Kurver", "grafer.cgi" , "graf" }, 
#ifndef SDVP			
    { "Export", "export.cgi" , "export" },				
#endif
    { "Bokstilknytning", "pair.cgi" , "pair" },				
#ifdef SDVP
    { "Flowkalibrering", "calib.cgi" , "calib" },			
#endif
    { NULL, NULL, NULL }						
};


struct siteadd *siteadd_create(const char *str)
{
	struct siteadd *new = malloc(sizeof(*new));
	assert(new);
	assert(str);
	
	new->str = strdup(str);
	new->next = NULL;

	return new;
}

struct siteadd *siteadd_list_add(struct siteadd *list,struct siteadd *new)
{
	if(!new){
		return list;		
	}
	
	new->next = list;
	return new;
	
}

void siteadd_delete(struct siteadd *sadd)
{
	if(!sadd)
		return;
	
	siteadd_delete(sadd->next);

	free(sadd->str);
	free(sadd);
}



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
		*name = ethmac_get();
		return -1;
    }

    asocket_con_trancive(sk_con, tx_msg, &rx_msg);

    if(!rx_msg){
//	pair_client_error("???", NULL, SOLVE_INTERNET);
		asocket_cmd_delete(tx_msg);
		*name = strdup("error: rx_msg");
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
	else 
		*name = strdup("error: param");
	
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

int siteutil_init(struct sitereq *site, char *subtitle, char *sname){
	
	siteutil_init_notpe(site, subtitle, sname);

	qCgiResponseSetContentType(site->req, "text/html;charset=UTF-8");

	return 0;
}

int siteutil_init_notpe(struct sitereq *site, char *subtitle, char *sname)
{
    memset(site, 0, sizeof(*site));
    site->req = qCgiRequestParse(NULL);
    site->sname = sname;
    site->subtitle = subtitle;
    site->socket_rp = siteutil_rpc_connect();
    siteutil_rpc_nameget(site->socket_rp, &site->boxid, &site->localid);
    

	

    return 0;
}


void siteutil_deinit(struct sitereq *site)
{
    
    Q_ENTRY *req = site->req;
    req->free(req);
    siteutil_rpc_disconnect(site->socket_rp);

}


const char *siteutil_topimage(void)
{
#ifndef SDVP
	return "<img src=\"../logoliab.png\" alt=\"Hjem\" id=\"logo-image\">";
#else
	return "<img src=\"../logosdvp.png\" alt=\"Hjem\" id=\"logo-image\">";
#endif
}


const char *siteutil_stylesheet(void)
{
#ifndef SDVP
	return "<link type=\"text/css\" rel=\"stylesheet\" media=\"all\" href=\"../liabzen.css\">";
#else
	return "<link type=\"text/css\" rel=\"stylesheet\" media=\"all\" href=\"../sdvpzen.css\">";
#endif
}

void siteutil_top(struct sitereq *site,  struct siteadd *header )
{

    /* Page header */
    Q_ENTRY *args = qEntry();
    char menu[1024];
	int headerlen = 0;
    char headerstr[1024] = "";
    Q_ENTRY *req = site->req;

    siteutil_topmenu(site->sname,menu,sizeof(menu));
    
    args->putStr(args, "${PAGE_TOPMENU}", menu, true);
    args->putStr(args, "${PAGE_TITLE}", "Styreboks", true);
    args->putStr(args, "${PAGE_SUBTITLE}", site->subtitle, true);

	args->putStr(args, "${PAGE_STYLESHEET}", siteutil_stylesheet(), true);

    if(site->simpel)
		args->putStr(args, "${PAGE_TOPIMG}", "", true);    
    else
		args->putStr(args, "${PAGE_TOPIMG}", siteutil_topimage(), true);    


	while(header){
		headerlen += snprintf(headerstr+headerlen, sizeof(headerstr)-headerlen,
							  "%s\n", header->str);
		header = header->next;
	}
	
	args->putStr(args, "${PAGE_EXTHDR}", headerstr, true);
    
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
