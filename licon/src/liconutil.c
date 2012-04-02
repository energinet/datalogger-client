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

#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep

#include "liabconnect.h"
#include "licon_client.h"

const  char *HelpText =
 "liconutil:  \n"
 " -s                  : Print status\n"
 " -i <name> <option>  : Set interface option\n"
 " -a <name> <option>  : Set application option\n"
 " -h           : Help (this text)\n"
 "Christian Rostgaard, April 2010\n" 
 "";


void licon_status_print(int port, int dbglev)
{
    char rx_buf[1024*10];
    char rx_len = 0;

    if(dbglev) fprintf(stderr, "connection to port %d\n", port);
    licon_client_connect(port);
    if(dbglev) fprintf(stderr, "connected to socket (port %d)\n",port);
    rx_len = licon_client_get_status(rx_buf, sizeof(rx_buf));

    if(dbglev) fprintf(stderr, "status received\n");

    licon_client_disconnect();
    printf( "%s\n", rx_buf);
    

    

}

void licon_option_set(const char *appif, const char *appifname,  const char *option, int port, int dbglev)
{
    char buffer[1024];
    int retval = 0;

    fprintf(stderr, "Setting '%s' option '%s'...\n", appifname, option);
    if(dbglev) fprintf(stderr, "connection to port %d\n", port);
    licon_client_connect(port);
    if(dbglev) fprintf(stderr, "connected to socket (port %d)\n",port);
    retval = licon_client_set_option(appif, appifname, option, buffer, sizeof(buffer));

    if(retval <0)
	fprintf(stderr, "Set returned error: %s\n", strerror(-retval));
    else
	fprintf(stderr, "Set returned: %s\n", buffer);

    licon_client_disconnect();
    
}



int main(int narg, char *argp[])
{
    int dbglev = 0;
    int sck_port = DEFAULT_PORT;
    int optc;

    while ((optc = getopt(narg, argp, "i:a:s::d::h")) > 0) {
	switch(optc){
	  case 'i':
	    licon_option_set("if", optarg,  argp[optind], sck_port, dbglev);
	    break;
	  case 'a':
	    licon_option_set("app", optarg,  argp[optind], sck_port, dbglev );
	    break;
	  case 's':
	    if(optarg)
		sck_port = atoi(optarg);
	    licon_status_print(sck_port, dbglev);
	    break;
	  case 'd':
	    if(optarg)
		dbglev = atoi(optarg);
	    else 
		dbglev = 1;
	    
	    break;
	  case 'h':
	  default:
	  fprintf(stderr, "%s", HelpText);
	  exit(0);
	  break;
	}
    }
}
