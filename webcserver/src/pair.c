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

enum {
    SOLVE_NONE,
    SOLVE_NUMBER,
    SOLVE_INTERNET,
};

int pair_client_error(char *local, char *rxerr, int solve)
{


    printf("<h3>FEJL: %s ", local);
    
    if(rxerr)
	printf(" (%s)", rxerr);

    printf("</h3>");

    switch(solve){
      case SOLVE_NUMBER:
	printf("Undersøg om indtastningen er korrekt<br>");
	break;
      case SOLVE_INTERNET:
	printf("Undersøg om boksen er forbundet korrekt til internettet<br>");
	break;
    }

    if(solve){
	printf("Hvis dette ikke løser problemet, så kontakt LIAB ApS<br>");
	printf("Telefon 98 37 01 44<br>");	
    } 

    printf("<br><form method=\"get\" action=\"pair.cgi\">"
	   "<input type=\"submit\" value=\"Start forfra\">"
	   "</form>");

    return 0;
    
}


int pair_client_start(void)
{
    
    printf("<h2>1: Indtast installations-ID</h2>");

    printf("<form method=\"get\" action=\"pair.cgi\">"
	  "<input type=\"text\" name=\"localid\" value=\"\">"
	  "<input type=\"submit\" value=\"Send\">"
	  "</form>");


    return 0;
}


int pair_client_check(struct asocket_con* sk_con, char *localid)
{
    struct skcmd* tx_msg = asocket_cmd_create("LocalIdChk");
    struct skcmd* rx_msg = NULL;

    asocket_cmd_param_add(tx_msg, localid);

    
    asocket_con_trancive(sk_con, tx_msg, &rx_msg);
    
    if(!rx_msg){
	pair_client_error("Lokal server fejl....", NULL, SOLVE_INTERNET);
	asocket_cmd_delete(tx_msg);
	return -1;
    }
	
    if(strcmp(asocket_cmd_param_get(rx_msg, 0), "1")!=0){
	pair_client_error("Fejl i installations-ID....", 
			  asocket_cmd_param_get(rx_msg, 1), SOLVE_NUMBER);
    } else { 
	printf("<h2>2: Bekræft installations-ID</h2>");
	printf("<h3>Installations ID: %s</h3>\n", localid);
	printf("<h3>%s</h3>\n", asocket_cmd_param_get(rx_msg, 1));
	printf("<h3><form method=\"get\" action=\"pair.cgi\">"
	       "<input type=\"hidden\" name=\"localid\" value=\"%s\" readonly>"
	       "Korrekt<input name=\"okbox\" type=checkbox value=\"checked\"><br>"
	       "<input type=\"submit\" value=\"Send\">"
	       "</form></h3>", localid);

	printf("<form method=\"get\" action=\"pair.cgi\">"
	       "<input type=\"submit\" value=\"Start forfra\">"
	       "</form>");
	
    }

    asocket_cmd_delete(rx_msg);
    asocket_cmd_delete(tx_msg);

    return 0;
}

int pair_client_set(struct asocket_con* sk_con, char *localid)
{
    struct skcmd* tx_msg = asocket_cmd_create("LocalIdSet");
    struct skcmd* rx_msg = NULL;

    asocket_cmd_param_add(tx_msg, localid);
    
    asocket_con_trancive(sk_con, tx_msg, &rx_msg);
    
    if(!rx_msg){
	pair_client_error("Lokal server fejl....", NULL, SOLVE_INTERNET);
	asocket_cmd_delete(tx_msg);
	return -1;
    }
	
    if(strcmp(asocket_cmd_param_get(rx_msg, 0), "1")!=0){
	pair_client_error("Fejl i installations id....", asocket_cmd_param_get(rx_msg, 1), SOLVE_NUMBER);
    } else { 
	printf("<h2>3: Boksen er nu tilknyttet:</h2><h2>Installations-ID: %s</h2>\n", asocket_cmd_param_get(rx_msg, 1));
    }

    asocket_cmd_delete(rx_msg);
    asocket_cmd_delete(tx_msg);

    return 0;
}


void pair_client_site(struct sitereq *site)
{
    
    Q_ENTRY *req = site->req;
    char *localid_req = (char *)req->getStr(req, "localid", false);
    char *okbox = (char *)req->getStr(req, "okbox", false);
    
    if(site->localid){
	printf("<h3>Boksen er tilknyttet installations-ID:</h3><h2>%s</h2>", site->localid);
	return;
    }

    if(!site->socket_rp){
	pair_client_error("Fejl i lokal forbindelse", NULL, SOLVE_INTERNET);
	return;
    }

    if((localid_req)&&(okbox)&&(strcmp(okbox,"checked")==0))
	pair_client_set(site->socket_rp, localid_req);
    else if (localid_req)
	pair_client_check(site->socket_rp, localid_req);
    else
	pair_client_start();
    

}



int main(int argc, char *argv[])
{
  struct sitereq site;

  openlog("pair.cgi", LOG_PID, LOG_DAEMON);

  siteutil_init(&site, "Bokstilknytning", "pair");
  
  syslog(LOG_NOTICE, "Reading configuration file...");

  siteutil_top(&site, 0);

  pair_client_site(&site);

  siteutil_bot(&site);

  siteutil_deinit(&site);

  syslog(LOG_NOTICE, "Done");
  return EXIT_SUCCESS;
}

