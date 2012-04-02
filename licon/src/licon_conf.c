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

#include "licon_conf.h"
#include "licon_if.h"
#include "licon_app.h"
#include "licon_check.h"
#include <xml_parser.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <errno.h>

char *strdup(const char *s);

int start_net_if(XML_START_PAR)
{
    struct licon *lc_config = (struct licon *)data;
    struct licon_if *new = licon_if_create(get_attr_str_ptr(attr, "if"),
                                           get_attr_str_ptr(attr, "mode"),
					   get_attr_str_ptr(attr, "apn"));

    if(!new){
        return -EFAULT;
    }
        
    lc_config->if_lst = licon_if_add(lc_config->if_lst, new);

    new->waittime  = licon_waittime_create(get_attr_str_ptr(attr, "waittimes"));


    ele->data = new;

    return 0;
}

void end_net_if(XML_END_PAR){

    ele->data = NULL;
}


int start_precmd(XML_START_PAR)
{
    struct licon_if *if_st = (struct licon_if *)ele->parent->data;
    
    const char* precmd = get_attr_str_ptr(attr, "precmd");
    const char* postcmd = get_attr_str_ptr(attr, "postcmd");
    const char* repcmd = get_attr_str_ptr(attr, "repcmd");

    if(precmd)
		if_st->precmd = strdup(precmd);
    
    if(postcmd)
	if_st->postcmd = strdup(postcmd);

    if(repcmd){
        if_st->repcmd = strdup(repcmd);
    }

    if_st->maxfail = get_attr_int(attr, "maxfail", if_st->maxfail );

    return 0;
}

int start_app(XML_START_PAR)
{
    struct licon *lc_config = (struct licon *)data;
    struct licon_app *new = licon_app_create(get_attr_str_ptr(attr, "name"),APPTYPE_APP, NULL,NULL, NULL);

    if(!new){
        return -EFAULT;
    }
	
	new->errfile   = licon_strdup(get_attr_str_ptr(attr, "err_file"));
    new->max_fail  = get_attr_int(attr, "err_max"  ,   0);
    new->ignorerr  = get_attr_int(attr, "ignore"   ,   0);
    new->enabled   = get_attr_int(attr, "enabled"  ,   1);
    new->uptime_ok = get_attr_int(attr, "uptime_ok",  new->uptime_ok);
    new->waittime  = licon_waittime_create(get_attr_str_ptr(attr, "waittimes"));

    lc_config->app_lst = licon_app_add(lc_config->app_lst, new);

    ele->data = new;

    return 0;
}

void end_app(XML_END_PAR){

    ele->data = NULL;
}

int start_appcmd(XML_START_PAR)
{
    struct licon_app *app = (struct licon_app *)ele->parent->data;
    
    const char* cmdstart = get_attr_str_ptr(attr, "start");
    const char* cmdstop = get_attr_str_ptr(attr, "stop");
    const char* pidfile = get_attr_str_ptr(attr, "pidfile");
    
    if(cmdstart)
	app->cmd_start = strdup(cmdstart);
    if(cmdstop)
	app->cmd_stop = strdup(cmdstop); 
    if(pidfile)
	app->pidfile = strdup(pidfile);

    return 0;
}

int start_check(XML_START_PAR)
{
    struct licon *lc_config = (struct licon *)data;

    lc_config->net_check = licon_check_create( get_attr_str_ptr(attr, "cmd"),
					       get_attr_int(attr, "expect", 0 ),
					       get_attr_int(attr, "err_max", 10 ),
					       get_attr_int(attr, "interval",  120));

    if(!lc_config->net_check)
	return -EFAULT;

    return 0;
}


int start_tunnel(XML_START_PAR)
{
    struct licon *lc_config = (struct licon *)data;
    struct licon_app *new =  licon_tun_create(get_attr_str_ptr(attr, "name"));

    if(!new){
        return -EFAULT;
    }
        
    new->max_fail =  get_attr_int(attr, "err_max",  0);
    new->enabled  = get_attr_int(attr, "enabled",  1);
    new->ignorerr  = get_attr_int(attr, "ignore"   ,   0);
    lc_config->app_lst = licon_app_add(lc_config->app_lst, new);

    return 0;

}

struct xml_tag xml_tags[] = {					       \
    { "conf"       , ""            , NULL        , NULL, NULL },       \
    { "check"      , "conf"        , start_check , NULL, NULL },       \
    { "net_if"     , ""            , start_net_if, NULL, end_net_if }, \
    { "cmd"        , "net_if"      , start_precmd, NULL, NULL },       \
    { "application", ""            , start_app   , NULL, end_app },    \
    { "cmd"        , "application" , start_appcmd, NULL, NULL },       \
    { "tunnel"     , "conf"        , start_tunnel, NULL, NULL},	       \
    { "" , "" , NULL, NULL, NULL }				       \
};
 




int licon_conf_read_file(struct licon *lc_config , const char *filename)
{
    return parse_xml_file(filename, xml_tags, (void*)lc_config);
}
