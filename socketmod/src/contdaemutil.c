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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
//#include <asocket_trx.h>
//#include <asocket_client.h>

#include <contlive.h>

#define DEFAULT_PORT 6523

const  char *HelpText =
 "contdaemutil: util for contdaem... \n"
 " -l              : List event types and values\n"
 " -a              : Show all\n"
 " -e <ename>      : Set event name (default *.*)\n"
 "Christian Rostgaard, June 2010\n" 
 "";


int live_client_write(int i);

int live_client_print_line(struct asocket_con* sk_con, const char *ename, const char *flags)
{

	char value[64];
	char hname[64];
	char unit[64];
	char ename_red[64];


		
	contlive_get_updlst(sk_con, flags, ename);
	
	while(contlive_get_next(sk_con, ename_red, value, hname, unit)==1){
		printf("%-20s %-20s   %15s   %s\n",ename_red, hname, value, unit);
	}



}

int live_client_print_single(struct asocket_con* sk_con, const char *ename, const char *flags)
{
	char value[64];
	char hname[64];
	char unit[64];
	
	contlive_get_single(sk_con, ename, value, hname, unit);

	printf(" %-20s   %15s   %s\n",hname, value, unit);
}

int main(int narg, char *argp[])
{
    int optc;
    int print_events = 0;
	char *write_value = NULL;
	char *ename = NULL;
	char *flags = NULL;
	struct asocket_con* sk_con;
	int dbglev = 0;
	int retval = 0;
	char *read_ename = NULL;

    while ((optc = getopt(narg, argp, "e:g:s:f::ld::h")) > 0) {
        switch (optc){ 
		  case 'e':
			ename = strdup(optarg);
			break;
		  case 'g':
			read_ename = strdup(optarg);
			break;
		  case 's':
			write_value = strdup(optarg);
			break;
		  case 'f':
			flags = strdup(optarg);
			break;
          case 'l': // list db events 
            print_events = 1;
            break;
		  case 'd':
			if(optarg)
				dbglev = 1;
			else
				dbglev = atoi(optarg);
			break;
          case 'h': // Help
          default:
		  fprintf(stderr, "%s", HelpText);
            exit(0);
            break;
        }
    }

	sk_con = contlive_connect(NULL, 0, dbglev);

	if(!sk_con){
		fprintf(stderr, "Error opening socket\n");
		return -1;
	}

	if(write_value){
		if(!ename){
			fprintf(stderr, "Error: no ename to write\n");
			goto out;
		}
			
		retval = contlive_set_single(sk_con, ename , write_value);
	}
		
    if(print_events)
		live_client_print_line(sk_con, ename, flags);


	if(read_ename)
		live_client_print_single(sk_con, read_ename, flags);


  out:
	contlive_disconnect(sk_con);

    return 0;
    
     

}



