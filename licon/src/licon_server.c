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

#include "licon_server.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "licon_if.h"
#include "licon_app.h"
#include "licon_check.h"
#include "asocket_trx.h"
#include "licon_log.h"

int scmd_handler_status(ASOCKET_FUNC_PARAM){
    //    char *outbuf, int max_size, void *appdata, char* payload, int len
    struct licon *lc_config = (struct licon *)sk_con->appdata;
    int ret = 0;
    struct licon_if * if_ptr = lc_config->if_lst;
    struct licon_app * app_ptr = lc_config->app_lst;
    struct licon_check *chk_ptr = lc_config->net_check;
    char *outbuf = malloc(1024*10);
    int  max_size = 1024*10;

    if(lc_config->d_level) fprintf(stderr, "socket handeling status request\n");

    if(lc_config->conn_if)
        ret += snprintf(outbuf+ret, max_size-ret, "Connected to %s\n", lc_config->conn_if->name);
    
    if(lc_config->d_level) fprintf(stderr, "socket handeling status request if status \n");

    ret += licon_if_status(NULL, outbuf+ret, max_size-ret); 

    while(if_ptr){ 
        ret += licon_if_status(if_ptr, outbuf+ret, max_size-ret); 
        if_ptr = if_ptr->next; 
    }     

   
    
    if(lc_config->d_level) fprintf(stderr, "socket handeling status request app status \n");

     ret += licon_app_status(NULL, outbuf+ret, max_size-ret); 

     while(app_ptr){ 
         ret += licon_app_status(app_ptr, outbuf+ret, max_size-ret); 
         app_ptr = app_ptr->next; 
     } 

     ret += licon_check_status(NULL, outbuf+ret, max_size-ret); 

     while(chk_ptr){ 
	 ret += licon_check_status(chk_ptr, outbuf+ret, max_size-ret); 
	 chk_ptr = chk_ptr->next;
     }
     
     if(lc_config->d_level) fprintf(stderr, "%s", outbuf);

    struct skcmd *tx_msg = asocket_cmd_create("Status");
    asocket_cmd_param_block_add(tx_msg, outbuf);

    asocket_con_snd(sk_con, tx_msg);

    asocket_cmd_delete(tx_msg);
    
    //   free(outbuf);

    return ret;
}


int scmd_handler_log(ASOCKET_FUNC_PARAM)
{
    int ret = 0;
    struct skcmd *tx_msg = asocket_cmd_create("Log");
    struct licon_log *log = licon_log_rgetfirst();

    if(log){
        char time[32];
        snprintf(time, sizeof(time), "%ld", log->tv.tv_sec);
        asocket_cmd_param_add(tx_msg, time);      
        asocket_cmd_param_add(tx_msg, log->text);      
        licon_log_delete(log);
    }

    asocket_con_snd(sk_con, tx_msg);
    asocket_cmd_delete(tx_msg);
    
    return ret;
}

int licon_get_status(struct licon *lc_config);

int scmd_handler_state(ASOCKET_FUNC_PARAM)
{
    int ret = 0;
    struct skcmd *tx_msg = asocket_cmd_create("State");
    struct licon *lc_config = (struct licon *)sk_con->appdata;

    switch(licon_get_status(lc_config)){
      case  LICON_IF_DOWN:
	asocket_cmd_param_add(tx_msg, "down");   
	break;
      case LICON_IF_UP:
	asocket_cmd_param_add(tx_msg, "netup"); 
  	break;
      case LICON_APP_START:
      case LICON_APP_RUN:
	asocket_cmd_param_add(tx_msg, "appup");      
	break;
      default:
	asocket_cmd_param_add(tx_msg, "unknown");      
	break;
    }

    asocket_con_snd(sk_con, tx_msg);
    asocket_cmd_delete(tx_msg);
    
    return ret;
}


/**
 * Get the index of an option */
int licon_option_index(const char *opt_str, const char **opt_list)
{
    char option[512];
    char *separator;
    int i = 0;

    strncpy(option, opt_str, sizeof(option));

    separator = strchr(option, '=');
    
    if(separator)
	separator[0] = '\0';

    while(opt_list[i]){
	if(strcmp(option, opt_list[i])==0)
	    return i;
	i++;
    }
    
    return -1;

}
 
/**
 * Get a pointer to the option setval */
const char *licon_option_setval(const char *opt_str)
{
    const char *setval =  strchr(opt_str, '=');
    
    if(setval)
	setval++;

    return setval;
}


int scmd_handler_setifoption(ASOCKET_FUNC_PARAM)
{
    const char *opt_list[] = { "alwayson", NULL};
    struct licon *lc_config = (struct licon *)sk_con->appdata;
    //struct licon_if * if_lst = lc_config->if_lst;
    
    struct skcmd *tx_msg = asocket_cmd_create("IfOption");
    
    const char* ifname  = asocket_cmd_param_get(rx_msg, 0);
    struct licon_if *if_obj = licon_if_get(lc_config->if_lst, ifname);

    asocket_cmd_param_add(tx_msg, ifname);

    if(!if_obj){
	asocket_cmd_param_add(tx_msg, "error interface not found");
	goto out;
    }

    const char* opt_str  = asocket_cmd_param_get(rx_msg, 1);

    switch(licon_option_index(opt_str, opt_list)){
      case 0: /* alwayson */ {
	  char optret[512];
	  licon_if_alwayson(if_obj, atoi(licon_option_setval(opt_str)));
	  sprintf(optret, "%s:%d", opt_list[0], if_obj->alwayson);
	  asocket_cmd_param_add(tx_msg, optret);
      } break;
      default:
	asocket_cmd_param_add(tx_msg, "error option not found");
	break;
    }

  out:

    asocket_con_snd(sk_con, tx_msg);
    asocket_cmd_delete(tx_msg);
    
    return 0;
}


int scmd_handler_setappoption(ASOCKET_FUNC_PARAM)
{
    const char *opt_list[] = { "enabled", NULL};
    struct licon *lc_config = (struct licon *)sk_con->appdata;
    //struct licon_if * if_lst = lc_config->if_lst;
    
    struct skcmd *tx_msg = asocket_cmd_create("AppOption");
    
    const char* appname  = asocket_cmd_param_get(rx_msg, 0);
    struct licon_app *app_obj = licon_app_get(lc_config->app_lst, appname);

    asocket_cmd_param_add(tx_msg, appname);

    if(!app_obj){
	asocket_cmd_param_add(tx_msg, "error application not found");
	goto out;
    }

    const char* opt_str  = asocket_cmd_param_get(rx_msg, 1);

    switch(licon_option_index(opt_str, opt_list)){
      case 0: /* enabled */ {
	  char optret[512];
	  const char *setval = licon_option_setval(opt_str);
	  if(strcmp(setval, "1")==0)
	      app_obj->enabled = 1;
	  else if(strcmp(setval, "0")==0)
	      app_obj->enabled = 0;

	  sprintf(optret, "%s:%s", opt_list[0], licon_option_setval(opt_str));
	  asocket_cmd_param_add(tx_msg, optret);
      } break;
      default:
	asocket_cmd_param_add(tx_msg, "error option not found");
	break;
    }

  out:

    asocket_con_snd(sk_con, tx_msg);
    asocket_cmd_delete(tx_msg);
    
    return 0;

};


struct asocket_cmd socket_cmd_list[] = {
    { "StatusGet"   , scmd_handler_status    , "Get status" },
    { "LogGet"      , scmd_handler_log       , "Get log entry" },   
    { "StateGet"    , scmd_handler_state     , "Get current connection state" },
    { "IfOptionSet" , scmd_handler_setifoption , "Set interface option" },
    { "AppOptionSet", scmd_handler_setappoption , "Set application option" },
    { "exit",  NULL , "Close the connection"}, 
    { NULL,   NULL }
};

struct socket_data param;


int licon_server_start(struct licon *lc_config)
{
    param.appdata = (void*)lc_config;
    param.skaddr = asocket_addr_create_in(NULL, lc_config->sck_port);
    param.cmds = socket_cmd_list;
    param.dbglev = lc_config->d_level; 
    return asckt_srv_start(&param);

}


void licon_server_stop(void)
{
    asckt_srv_close(&param);
}
