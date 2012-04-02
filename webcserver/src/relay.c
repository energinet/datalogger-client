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

#define TEXT_ON   "On"
#define TEXT_OFF  "Off"


struct ledswitch{
    char *name;
    char *bfile;
};


struct ledswitch ledlist[] = {\
    { "Relay 1" , "/sys/class/leds/relay_1/brightness" },
    { "Relay 2" , "/sys/class/leds/relay_2/brightness" },
    { "Relay 3" , "/sys/class/leds/relay_3/brightness" },
    { NULL , NULL },
};
    



int set_relay(int index, int state)
{
  FILE *fp;
  char buf[128];

  snprintf(buf, sizeof(buf), ledlist[index].bfile);
  fprintf(stderr, "setrelay: %s\n", buf);

  if((fp = fopen(buf, "w")) != NULL) {
    fprintf(fp, "%d", state ? 1 : 0);
    fflush(fp);
    fclose(fp);
  }

  return 0;
}


int get_relay(int index)
{
  FILE *fp;
  char buf[128];
  int state = -1;

  snprintf(buf, sizeof(buf), ledlist[index].bfile);
  fprintf(stderr, "getrelay: %s\n", buf);

  if((fp = fopen(buf, "r")) != NULL) {
      fscanf(fp, "%d", &state);
      fflush(fp);
      fclose(fp);
  } 

  return state;
}


int relay_form_print_single(int index){

    int state = get_relay(index);
    
    printf("<form action=\"relay.cgi\" method=\"post\" name=\"control_%d\">", index);
    
    printf("<input type='hidden' name='relay_no' value='%d' />", index);
    printf("<label>%s</label>", ledlist[index].name);

    printf("<input style='background-color:%s' type=\"submit\" value=\"%s\" name=\"button\" />", (state == 1)?"lightgreen":"lightgray", TEXT_ON);

    printf("<input style='background-color:%s' type=\"submit\" value=\"%s\" name=\"button\" />", (state == 0)?"#FF3366":"lightgray", TEXT_OFF);

    printf("</form>\n");
    return 0;
}

int relay_form_print(void)
{
    int index = 0;

    while(ledlist[index].name){
	relay_form_print_single(index++);
    }


    return 0;
}

int relay_req(struct sitereq *site)
{
    Q_ENTRY *req = site->req;
    char *relay_set = (char *)req->getStr(req, "button", false);
    char *relay_num = (char *)req->getStr(req, "relay_no", false);
    int index = 0;
    int state = 0;

    if(relay_set&&relay_num){
	index = atoi(relay_num);
	if(strcmp(relay_set, TEXT_ON)==0)
	    state = 1;
	
	set_relay(index, state);
    }

    return 0;
}


int main(int argc, char *argv[])
{
  struct sitereq site;

  openlog("relay.cgi", LOG_PID, LOG_DAEMON);

  siteutil_init(&site, "Relæ", "relay");
  
  syslog(LOG_NOTICE, "Reading configuration file...");
  site.simpel = 1;
  siteutil_top(&site, 0);

  relay_req(&site);

  relay_form_print();

  /* relay_form_print_single(0); */
  /* relay_form_print_single(1); */
  /* relay_form_print_single(2); */

  siteutil_bot(&site);

  siteutil_deinit(&site);

  syslog(LOG_NOTICE, "Done");
  return EXIT_SUCCESS;
}
