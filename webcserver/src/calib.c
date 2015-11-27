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

int calib_client_connect(void)
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

int calib_client_disconnect(void)
{
    return asocket_clt_disconnect(sk_con);
}


int calib_status(char *buffer)
{
    
    struct skcmd* tx_msg = asocket_cmd_create("ReadsUpdate");
    struct skcmd* rx_msg = NULL;
    const char *ptr;
    int state = 0;
    calib_client_connect();
    /* prepare */
    asocket_cmd_param_add(tx_msg, "all");  
    asocket_cmd_param_add(tx_msg, "flowcalib.*");  
    asocket_con_trancive(sk_con, tx_msg, &rx_msg);
    asocket_cmd_delete(tx_msg);   
    if(!rx_msg)
        return -EFAULT;
    asocket_cmd_delete(rx_msg);
    rx_msg = NULL;

    /* get */
    tx_msg = asocket_cmd_create("ReadGet");    
    asocket_con_trancive(sk_con, tx_msg, &rx_msg);

    if(!rx_msg)
	goto out;
    if(!rx_msg->param)
	goto out;

    ptr = asocket_cmd_param_get(rx_msg, 2);

    sscanf(ptr, "%d", &state);

    ptr = strchr(ptr, ':');
    
    if(ptr)
	strcpy(buffer, ptr+1);
    else
	buffer[0]= '\0';

  out:
    asocket_cmd_delete(rx_msg);
    asocket_cmd_delete(tx_msg);  
    calib_client_disconnect();

    return state;

}


int calib_button_start(char *stage)
{

    if(!stage){
	printf("<form method=\"get\" action=\"calib.cgi\">"
	       "<input type=\"hidden\" value=\"start\" name=\"stage\" >"
	       "<input type=\"submit\" value=\"Start kalibrering\">"
	       "</form>");
	return 0;
    }

    
    printf("<form method=\"get\" action=\"calib.cgi\">"
	   "<input type=\"submit\" value=\"Stop kalibrering\">"
	   "</form>");

    return 0;
}



void calib_step_print(int bold, const char *text)
{
    if(bold)
	printf("<li><b>%s</b>\n", text);
    else 
	printf("<li>%s\n", text);
}


void calib_site(struct sitereq *site)
{

//    Q_ENTRY *req = site->req;
//    char *stage = (char *)req->getStr(req, "stage", false);
    char buffer[256];
    int state = calib_status(buffer);

    printf("<UL>");
    calib_step_print(state == 1 , "Trin 1: Sørg for at alle vandhaner er lukket.");
    calib_step_print(state == 0 , "Trin 2: Start kalibreringen ved at trykke og holde <font color=\"#00aa00\"> Test </font> knappen på styreboksen 5 sekunder.");
    calib_step_print(state == 2, "Trin 3: Fyld en 10 liters spand med vand fra den varme hane..");
    calib_step_print(state == 3, "Trin 4: Se status på denne side om testen gik godt.");  
    calib_step_print(state == 4, "Trin 5: Send resultatet ved at trykke og holde <font color=\"#00aa00\"> Test </font> knappen på styreboksen 5 sekunder. </b>.");   
    printf("</UL>");
    
    printf("<br><br><table> <tr class=\"odd\" ><td style=\"width: 300px;\">Status:<br>%s</td></tr></table><br>", buffer);

    printf("Reset status ved at trykke på <font color=\"#00aa00\"> Test </font> i mindre end 3 sekunder"); 
    return;
    
}



int main(int argc, char *argv[])
{
  struct sitereq site;

  openlog("pair.cgi", LOG_PID, LOG_DAEMON);

  siteutil_init(&site, "Flowkalibrering", "calib");
  
  syslog(LOG_NOTICE, "Reading configuration file...");

  struct siteadd *header = siteadd_create("<META HTTP-EQUIV=\"refresh\" CONTENT=\"3\" />");
	
  siteutil_top(&site, header);

  siteadd_delete(header);


  calib_site(&site);

  siteutil_bot(&site);

  siteutil_deinit(&site);

  syslog(LOG_NOTICE, "Done");
  return EXIT_SUCCESS;
}
