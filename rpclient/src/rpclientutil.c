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

#define _GNU_SOURCE
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>

#include <asocket_trx.h>
#include <asocket_server.h>

#include "soapH.h" // obtain the generated stub
//#include "liabdatalogger.nsmap" // obtain the namespace mapping table
#include "rpserver.h"
#include "rpclient_db.h"
#include "rpclient_cmd.h"
#include "rpclient_soap.h"
#include "boxcmdq.h"
#include "version.h"
#include "rpclient.h"
#include "pidfile.h"
#include <logdb.h>
#include <aeeprom.h>


#include "rpclient_glob.h"

#define MAXLEN 255

static char *helptext =
  "\n"
  "Utility for the gSOAP XML interface towards the LIAB datalogger service.\n"
  "Version "BUILD" "COMPILE_TIME"\n" 
  "Usage: rpclientutil [OPTIONS]\n"
  "\n"
  "Arguments:"
  "\n"
  "  -h, --help                    Help text (this text)\n"
  "  -v, --version                 Get version information\n"
  "  -H, --host <hostname>         Connect to <hostname> at IP-address or\n"
  "                                servername (mandatory)\n"
  "  -u, --username <user>         Connect with username <user>\n"
  "  -p, --password <password>     Connect with password <password>\n"
  "  -s, --set-file <path>         Send file with the <path> to server\n"
  "  -g, --get-file <path>         Get a file from the server, to be placed in <dest>\n"
  "                                the serverfile is specified by --server-file <file>\n"
  "  -f, --server-file <file>      Server file name\n"
  "  -d, --dbg-lev <level>         Set the debuf level\n"
  "\n"
  "LIAB ApS 2010-2011 <www.liab.dk>\n\n";

static struct option long_options[] =
  {
    {"help",		  no_argument,		    0, 'h'},
    {"version", 	  no_argument,		    0, 'v'},
    {"host",  		  required_argument,	0, 'H'},
    {"username",  	  required_argument,	0, 'u'},
    {"password",  	  required_argument,	0, 'p'},
    {"set-file",      required_argument,	0, 's'},
    {"get-file",      required_argument,	0, 'g'},
	{"server-file",      required_argument,	0, 'f'},
	{"dbg-lev",       optional_argument,	0, 'd'},
    {0, 0, 0, 0}
  };


int main(int argc, char *argv[])
{
    int optc;
    int option_index = 0;
	int retval = EXIT_FAILURE;
	int dbglev = 0;
	struct rpclient_soap rpsoap;
	struct rpclient client_obj;

	memset(&client_obj, 0, sizeof(client_obj));
	memset(&rpsoap, 0, sizeof(rpsoap));

	client_obj.rpsoap = &rpsoap;
	client_obj.interval = 200; 
	client_obj.loop = -1;
	client_obj.db  = NULL;
 
 	char *get_filepath = NULL;
	char *set_filepath = NULL;
	char *serverfile = NULL;

	while ( (optc = getopt_long (argc, argv, "hvH:u:p:s:f:g:d::", long_options, &option_index)) != EOF ) {
		switch (optc) {
		  case 0:
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0)
				break;
			printf ("option %s", long_options[option_index].name);
			if (optarg)
				printf (" with arg %s", optarg);
			printf ("\n");
			break;
		  case 'h':
			printf("%s", helptext);
			exit(EXIT_SUCCESS);
		  case 'H':
			rpsoap.address = strndup(optarg, MAXLEN);
			break;
		  case 'u':
			rpsoap.username = strndup(optarg, MAXLEN);
			break;
		  case 'p':
			rpsoap.password = strndup(optarg, MAXLEN);
			break;
		  case 's':
			set_filepath = strdup(optarg);
			break;
		  case 'g':
			get_filepath = strdup(optarg); 
			break;
		  case 'f':
			serverfile = strdup(optarg);
			break;
		  case 'd':
			if(optarg)
				dbglev = atoi(optarg);
			else
				dbglev = 1;
			break;
		  case '?':
			/* getopt_long already printed an error message. */
		  default:
		  printf("%s", helptext);
		  exit(EXIT_SUCCESS);
		}
	}
	
	PRINT_DBG(dbglev, "INFO: debug level: %x", dbglev);

	rpsoap.dbglev = dbglev;
	client_obj.dbglev = dbglev;

	if(!client_obj.type){
		client_obj.type = strdup("test");
	}



	if(!client_obj.name)
		client_obj.name = platform_user_get();
  
	if(!client_obj.localid){
		client_obj.localid = aeeprom_get_entry(AEEVAR_LOCALID);      
  }
  
	if(!client_obj.rpversion){
		client_obj.rpversion = strdup(BUILD" "COMPILE_TIME );
	}
  
	PRINT_DBG(dbglev, "INFO: box type :   %s", client_obj.type);  
	PRINT_DBG(dbglev, "INFO: box name :   %s", client_obj.name);  
	PRINT_DBG(dbglev, "INFO: localid  :   %s", client_obj.localid);  
	PRINT_DBG(dbglev, "INFO: rpversion:   %s", client_obj.rpversion);  
	
	if (rpsoap.address == NULL ) {
		PRINT_ERR("Error. You must specify an address.");
		PRINT_ERR("%s", helptext);
		exit(EXIT_FAILURE);
	}
	
	PRINT_DBG(dbglev, "INFO: address  :   %s", rpsoap.address);  
  
	if ( argc < 2 ) {
		PRINT_ERR("Error. You must specify at least one option.");
		PRINT_ERR( "%s",  helptext);
		exit(EXIT_FAILURE);
  }

	
	if((retval=rpclient_soap_init(&rpsoap))<0){
		PRINT_ERR("rpclient_soap_init returned fault\n");
		goto out;
	}

	if(set_filepath){
		if(!serverfile)
			serverfile = set_filepath;
		fprintf(stderr, "set file '%s'\n", set_filepath);
		retval = file_set(&client_obj, rpsoap.soap, rpsoap.address,  serverfile,set_filepath);
	} else if(get_filepath){
		if(!serverfile){
			fprintf(stderr, "Error: No serverfile specified\n");
			goto out;
		}
		fprintf(stderr, "get file '%s'\n", get_filepath);
		retval = file_get(&client_obj, rpsoap.soap, rpsoap.address, 
						  serverfile, get_filepath, NULL);
	}


	rpclient_soap_deinit(&rpsoap);
  out:
  
	return retval;
}
  
