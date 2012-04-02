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
#include "aeeprom.h"


const  char *HelpText =
 "aeeprom_util: util for aeeprom... \n"
 " -g <name>       : Get variable of <name>\n"
 " -s <name>=<val> : Set variable of <name> to <val>\n"
 " -R <name>       : Remove variable <name>\n"
 " -h              : Help (this text)\n"
 "Christian Rostgaard, September 2010\n" 
 "";



int main(int narg, char *argp[])
{
    int optc;
    int debug_level = 0;

    while ((optc = getopt(narg, argp, "g:s:R:Dh")) > 0) {
	switch(optc){
	  case 'g':
	    {
		char* val = aeeprom_get_entry(optarg);
		if(val)
		    printf("%s=%s\n", optarg, val);
		else
		    printf("%s=N/A\n", optarg);
	    }
	    break;
	  case 's':
	    {
		char *name = strdup(optarg);
		char *value = strchr(name, '=');
		
		if(value){
		    value[0]='\0';
		    value++;
		    fprintf(stderr, "Set %s to %s\n",  name, value);
		    aeeprom_set_entry(name, value);
		}else
		    fprintf(stderr, "ERR no var name %s\n", name);

		free(name);
	    }
	    break;
	  case 'R':
	    aeeprom_set_entry(optarg, NULL);
	    break;
	  case 'D':
	    {
		char* dump = aeeprom_dump();
		if(dump)
		    printf("File:\n%s\n::END::", dump);
		else{
		    printf("Error");
		    exit(1);
		}
	    }
	    break;
	  case 'h':
	  default:
	  fprintf(stderr, "%s", HelpText);
	  exit(0);
	  break;
	}
    }

    exit(0);
}
