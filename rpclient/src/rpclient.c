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
#include <syslog.h>

#include "rpclient_glob.h"

#define MAXLEN 255
#define MAX_FILESIZE (1024*512)
//#define DEFAULT_PIDFILE   "/var/run/rpclient.pid"
//#define AEEVAR_LOCALID  "localid"

//#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s: "str"\n",__FUNCTION__, ## arg)
//#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}



static char *helptext =
  "\n"
  "gSOAP XML interface towards the LIAB datalogger service.\n"
  "Version "BUILD" "COMPILE_TIME"\n" 
  "Usage: rpclient [OPTIONS]\n"
  "\n"
  "Arguments:"
  "\n"
  "  -h, --help                    Help text (this text)\n"
  "  -H, --host HOST               Connect to HOSTNAME at IP-address or\n"
  "                                servername (mandatory)\n"
  "  -u, --username USER           Connect with username USER\n"
  "  -p, --password PASSWORD       Connect with password PASSWORD\n"
  "  -r, --realm                   Connect with realm (default: remboxdb)\n"
  "  -N, --name                    Reporting name\n"
  "  -f, --forground               Run in forground\n"
  "  -i, --interval                Set interval\n"
  "  -n, --numbertransf              Set number of transvers\n"
  "\n"
  "LIAB ApS 2010-2013 <www.liab.dk>\n\n";

static struct option long_options[] =
  {
    {"help",		  no_argument,	        0, 'h'},
    {"version", 	  no_argument,	        0, 'V'},
    {"host",  		  required_argument,	0, 'H'},
    {"username",  	  required_argument,	0, 'u'},
    {"password",  	  required_argument,	0, 'p'},
    {"realm",             required_argument,    0, 'r'}, 
    {"name",      	  required_argument,	0, 'N'},
    {"forground",  	  no_argument,		0, 'f'},
    {"interval",  	  required_argument,	0, 'i'},
    {"numbertransf",  required_argument,	0, 'n'},
    {0, 0, 0, 0}
  };

int run = 1;

struct cmd_list{
    struct boxCmd cmd;
    struct cmd_list *next;
};


struct cmd_list *cmd_list_create(struct boxCmd *cmd)
{
    struct cmd_list *new = malloc(sizeof(*new));

    memcpy(&new->cmd, cmd, sizeof(new->cmd));

    return new;
}


int uploadData(struct rpclient *client_obj)
{
    int retval = 0;

    syslog(LOG_DEBUG, "send metadata...");
    retval = send_metadata(client_obj,client_obj->rpsoap->soap,  client_obj->rpsoap->address, client_obj->db );
    if(retval < 0){
	syslog(LOG_ERR, "send metadata failed");
	return retval;
    }
    
    syslog(LOG_DEBUG, "send measurments...");
    retval = send_measurments(client_obj,client_obj->rpsoap->soap, client_obj->rpsoap->address,client_obj->db );
    
    if(retval < 0){
	syslog(LOG_ERR, "send measurments failed\n");
	return retval;
    }

    syslog(LOG_DEBUG, "done transmitting");

    return 0;

}



int socket_hndlr_localid_get(ASOCKET_FUNC_PARAM)
{
    struct rpclient *client_obj = ( struct rpclient *)sk_con->appdata;
    struct skcmd *tx_msg = asocket_cmd_create("LocalId");
//    struct soap *soap = client_obj->rpsoap->soap;
    
    if(client_obj->localid){
	asocket_cmd_param_add(tx_msg, "1");
	asocket_cmd_param_add(tx_msg, client_obj->name);
	asocket_cmd_param_add(tx_msg, client_obj->localid);
    } else {
	asocket_cmd_param_add(tx_msg, "0");
	asocket_cmd_param_add(tx_msg, client_obj->name);
    }

    asocket_con_snd(sk_con, tx_msg);
    asocket_cmd_delete(tx_msg);

    return 0;

}


/* int socket_hndlr_localid_chk(ASOCKET_FUNC_PARAM) */
/* { */
/*     struct rpclient *client_obj = ( struct rpclient *)sk_con->appdata; */
/*     struct skcmd *tx_msg = asocket_cmd_create("LocalIdCheked"); */
/*     struct soap *soap = client_obj->rpsoap->soap; */
/*     const char *address =  client_obj->rpsoap->address; */
/*     struct rettext rettxt; */
/*     int err; */
    
/*     struct boxInfo boxinfo; */
/*     rp_boxinfo_set(client_obj, &boxinfo); */
    
/*     int retries = 0; */
/*     retval = -1; */
    

/*     do{ */
/* 	err = soap_call_liabdatalogger__pair(soap, address, NULL,&boxinfo,asocket_cmd_param_get(rx_msg, 0), 0 /\*dopair*\/, &rettxt); */
/* 	if(err=SOAP_OK){ */
/* 	    char buf[512]; */
/* 	    sprintf(buf,"%d", rettxt.retval); */
/* 	    asocket_cmd_param_add(tx_msg, buf); */
/* 	    if(rettxt.retval==1){ */
/* 		asocket_cmd_param_add(tx_msg, rettxt.text); */
/* 	    else */
/* 		asocket_cmd_param_add(tx_msg, "err"); */
/* 	    } */
	    
/* 	    fprintf(stderr, "socket_hndlr_localid_chk retval %d\n", rettxt.retval); */
	    
/* 	} */
	

/*     }while(rpclient_soap_hndlerr(rpsoap,soap, rpsoap->dainfo, retries++, __FUNCTION__)); */

/*     /\* */
/*     if((err = soap_call_liabdatalogger__pair(soap, address, NULL,&boxinfo,asocket_cmd_param_get(rx_msg, 0), 0 /\*dopair*\/, &rettxt))== SOAP_OK){ */
/* 	char buf[512]; */
/* 	sprintf(buf,"%d", rettxt.retval); */
/* 	asocket_cmd_param_add(tx_msg, buf); */
/* 	if(rettxt.retval==1) */
/* 	    asocket_cmd_param_add(tx_msg, rettxt.text); */
/* 	else */
/* 	    asocket_cmd_param_add(tx_msg, "err"); */

/* 	fprintf(stderr, "socket_hndlr_localid_chk retval %d\n", rettxt.retval); */
/*     } else { */
/* 	soap_print_fault(soap, stderr); */
/* 	fprintf(stderr,"boxInfo fault\n"); */
/* 	asocket_cmd_param_add(tx_msg, "-1"); */
/* 	return -1; */
/*     }*\/ */


/*     asocket_con_snd(sk_con, tx_msg); */
/*     asocket_cmd_delete(tx_msg); */

/*     return 0; */

/* } */


void rpclient_localiid_set(struct rpclient *client_obj, const char *localid)
{
    if(!localid)
	return;

    aeeprom_set_entry(AEEVAR_LOCALID, localid);
    
    client_obj->localid = strdup(localid);
}




int socket_hndlr_localid_setchk(struct rpclient *client_obj, struct asocket_con* sk_con, struct skcmd* rx_msg, int dopair)
{

    struct skcmd *tx_msg = NULL ;
    struct soap *soap = client_obj->rpsoap->soap;
    const char *address =  client_obj->rpsoap->address;
    struct rettext rettxt;
    int retval;
    
    if(dopair){
	tx_msg = asocket_cmd_create("LocalIdSetted");
    } else {
	tx_msg = asocket_cmd_create("LocalIdCheked");
    }

    retval = rpclient_db_localid(client_obj, soap,  address,asocket_cmd_param_get(rx_msg, 0), dopair, &rettxt );

    if(retval < 0){
	asocket_cmd_param_add(tx_msg, "-1");
    } else {
	char buf[512];
	sprintf(buf,"%d", rettxt.retval);
	asocket_cmd_param_add(tx_msg, buf);
	if(rettxt.retval==1){
	    asocket_cmd_param_add(tx_msg, rettxt.text);
	    if(dopair){
		rpclient_localiid_set(client_obj, asocket_cmd_param_get(rx_msg, 0));
	    }
	} else {
	    asocket_cmd_param_add(tx_msg, "err");	
	}
    }

    asocket_con_snd(sk_con, tx_msg);
    asocket_cmd_delete(tx_msg);

    return retval;

}

int socket_hndlr_localid_set(ASOCKET_FUNC_PARAM)
{
    struct rpclient *client_obj = ( struct rpclient *)sk_con->appdata;
    socket_hndlr_localid_setchk(client_obj, sk_con, rx_msg, 1);
}

int socket_hndlr_localid_chk(ASOCKET_FUNC_PARAM)
{
    struct rpclient *client_obj = ( struct rpclient *)sk_con->appdata;
    socket_hndlr_localid_setchk(client_obj, sk_con, rx_msg, 0);
}


struct socket_cmd socket_cmd_list[] = {
    { "LocalIdGet" , socket_hndlr_localid_get, "Check local ID"},
    { "LocalIdChk" , socket_hndlr_localid_chk, "Check local ID"},
    { "LocalIdSet" , socket_hndlr_localid_set, "Set local ID"}, //ToDo
    { "exit",  NULL , "Close the connection"}, 
    { "Exit",  NULL , "Close the connection"}, 
    { NULL,   NULL }
};

int rpclient_socket_start(struct socket_data *param, struct rpclient *client_obj)
{
    param->appdata = (void*)client_obj;
    param->skaddr = asocket_addr_create_in(NULL, 4568);
    param->cmds = socket_cmd_list;
    return asckt_srv_start(param);

}



int theloop(struct rpclient *client_obj)
{
    struct timeval tv_now;
    struct timeval tv_prev;
    int loop = client_obj->loop;
    int interval = client_obj->interval;
    memset(&tv_prev, 0, sizeof(tv_prev));
	
    while(loop&&run){
		gettimeofday(&tv_now, NULL);

		if( (tv_now.tv_sec > (tv_prev.tv_sec+interval))||
			(tv_now.tv_sec < tv_prev.tv_sec) ){
			rpclient_stfile_write("theloop: upload data");
			if(uploadData(client_obj)<0)
				return -1;
			if(loop>0){
				loop--;
				rpclient_stfile_write("theloop: loop = %d", loop);
			}
			gettimeofday(&tv_prev, NULL);
		} else {
			rpclient_stfile_write("theloop: cmd_run");
			cmd_run(client_obj,NULL, client_obj->rpsoap->address, tv_now.tv_sec);
			rpclient_stfile_write("theloop: idle");		
		}
	
		sleep(1);
    }
	
    return 0;
    
}

void signal_handler(int sig)
{
    static int sigintcnt = 0;
    static int sighupcnt = 0;

    switch(sig) {
      case SIGHUP:
	sighupcnt++;
	fprintf(stderr, "signal \"hup\" received %d\n",sighupcnt);
	break;
      case SIGINT:
	sigintcnt++;
	run = 0;
	fprintf(stderr, "signal \"interrupt\" received %d\n",sigintcnt);
	if(sigintcnt > 10)
	    exit(0);
      break;
    default:
      break;
  }

}


int main(int argc, char *argv[])
{
    int optc;
    int option_index = 0;
  int retval = EXIT_FAILURE;
  int forground = 0;
  int dbglev = 0;
  struct rpclient_soap rpsoap;
  struct rpclient client_obj;
  struct socket_data sparam;
  struct pidfile *pidfile = NULL;

  signal(SIGINT, signal_handler);
  signal(SIGHUP, signal_handler);

  memset(&client_obj, 0, sizeof(client_obj));
  memset(&rpsoap, 0, sizeof(rpsoap));
  memset(&sparam, 0,  sizeof(sparam));

  rpsoap.authrealm = DEFAULT_REALM;

  client_obj.rpsoap = &rpsoap;
  client_obj.sparam = &sparam;
  client_obj.interval = 200; 
  client_obj.loop = -1;
  client_obj.db  = NULL;
  client_obj.run = 1;

  openlog("rpclient", LOG_PID|LOG_PERROR, LOG_DAEMON);
  setlogmask(LOG_UPTO(LOG_INFO));
	
  
  while ( (optc = getopt_long (argc, argv, "hH:u:p:vs:fi:t::N:n:d::", long_options, &option_index)) != EOF ) {
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
        rpsoap.address = optarg;
        break;
      case 'u':
        rpsoap.username = optarg;
        break;
      case 'p':
        rpsoap.password = optarg;
        break;
      case 'r':
	rpsoap.authrealm = optarg;
	break;
      case 'N':
	platform_set_name(optarg);
	break;
      case 'f':
        forground = 1;
        fprintf(stderr, "run in forground\n");
        break;
      case 'i':
        client_obj.interval = atoi(optarg);
        break;
      case 'n':
        client_obj.loop = atoi(optarg);
        break;
      case 't':{
	const char *ifname;
	char result[64];
	if(optarg)
	    ifname = optarg;
	else {
	    ifname = "eth0";
	}
	fprintf(stdout, "ifname:      \t%s\n", ifname);
	get_iface_info(IF_IP, ifname, result, sizeof(result));
	fprintf(stdout, "ip:      \t%s\n", result);
	get_iface_info(IF_NETMASK, ifname, result, sizeof(result));
	fprintf(stdout, "mask:     \t%s\n", result);
	get_iface_info(IF_BROADCAST, ifname, result, sizeof(result));
	fprintf(stdout, "broadcast:\t%s\n", result);
	get_iface_info(IF_MAC, ifname, result, sizeof(result));
	fprintf(stdout, "mac:      \t%s\n", result);
	get_iface_info(IF_GATEWAY, ifname, result, sizeof(result));
	fprintf(stdout, "gateway:  \t%s\n", result);
	fprintf(stdout, "name:     \t%s\n", platform_get_name());
	exit(0);
	   }
      case 'd':
	if(optarg)
	    dbglev = atoi(optarg);
	else
	    dbglev = 1;
	setlogmask(LOG_UPTO(LOG_DEBUG));
	break;
      case '?':
          /* getopt_long already printed an error message. */
      default:
        printf("%s", helptext);
        exit(EXIT_SUCCESS);
    }
  }

  syslog(LOG_INFO, "INFO: debug level: %x", dbglev);

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
  
  syslog(LOG_INFO, "INFO: box type :   %s", client_obj.type);  
  syslog(LOG_INFO, "INFO: box name :   %s", client_obj.name);  
  syslog(LOG_INFO, "INFO: localid  :   %s", client_obj.localid);  
  syslog(LOG_INFO, "INFO: rpversion:   %s", client_obj.rpversion);  
    
  if (rpsoap.address == NULL ) {
      fprintf(stderr, "Error. You must specify an address.\n");
      fprintf(stderr, "%s\n", helptext);
      exit(EXIT_FAILURE);
  }

  syslog(LOG_INFO, "INFO: address  :   %s", rpsoap.address);  
  
  if ( argc < 2 ) {
      fprintf(stderr, "Error. You must specify at least one option.\n");
      fprintf(stderr, "%s\n",  helptext);
      exit(EXIT_FAILURE);
  }

  if(!forground){
      syslog(LOG_INFO, "Deamonizing");
      if(daemon(0,dbglev))
          syslog(LOG_ERR, "Daemon failed: %s", strerror(errno));

  }

  syslog(LOG_NOTICE, "Running in forgeound");  
  
  pidfile = pidfile_create(DEFAULT_PIDFILE, PMODE_EXIT, dbglev);
  assert(pidfile);
  
  client_obj.db = logdb_open(DEFAULT_DB_FILE, 15000, 0);

  if(!client_obj.db){
      syslog(LOG_ERR, "Coild not open log database\n");
      goto out;
  }

  if((retval=rpclient_soap_init(&rpsoap))<0){
      syslog(LOG_ERR, "rpclient_soap_init returned fault\n");
      goto out_db;
  }


  if((retval = rpclient_cmd_init(NULL, dbglev))<0){
      syslog(LOG_ERR,"Error opening boxvcmd");
      goto out_soap;
  }
  
  rpclient_socket_start(&sparam, &client_obj);

  retval = theloop(&client_obj);

  rpclient_cmd_deinit();
  
  asckt_srv_close(&sparam);

  out_soap:

  rpclient_soap_deinit(&rpsoap);

  out_db:

  logdb_close(client_obj.db);

  out:
  if(pidfile)
	  pidfile_delete(pidfile);

  if(retval < 0)
      exit(EXIT_FAILURE);
  else 
      exit(EXIT_SUCCESS);
  
}
  
