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

#include "module_base.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/statvfs.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * @ingroup modules 
 * @{
 */

/**
 * @defgroup modules_syslog Module for system log reading
 * @{
 */

enum filter_ret{
    FILTERRET_NONE,
    FILTERRET_HIT,
    FILTERRET_REJECT,
};


struct logfilter{
    char *text;
    enum filter_ret type;
    struct logfilter *next;
};

struct module_object{
    struct module_base base;
    char *cmd;
    struct logfilter *filters;
};


struct logfilter *logfilter_create(const char *text, enum filter_ret type)
{
    struct logfilter *new = malloc(sizeof(*new));
    assert(new);

    if(!text){
	PRINT_ERR("error in logfile: no file path"); 
	free(new);
	return NULL;
    }

    new->text = strdup(text);
    new->type = type;
    new->next = NULL;

    return new;
}

void logfilter_delete(struct logfilter *filter)
{
    if(filter->next)
        logfilter_delete(filter->next);

    free(filter->text);
    free(filter);
}


struct logfilter *logfilter_lst_add(struct logfilter *list, struct logfilter *new)
{
    struct logfilter *ptr = list;
    
    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}



enum filter_ret logfilter_filter(struct logfilter *filter, const char *str){
    
    if(!filter)
	return FILTERRET_NONE;
    
    if(strstr(filter->text, str)!=NULL)
	return filter->type;
    
    return FILTERRET_NONE;
    
}


enum filter_ret logfilter_lst_filter(struct logfilter *list, const char *str){
    
    enum filter_ret ret = logfilter_filter(list, str);

    if(ret != FILTERRET_NONE)
	return ret;

    return logfilter_filter(list->next, str);
    
}


struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}


int start_filter(XML_START_PAR)
{
    struct module_object* this = module_get_struct(ele->parent->data);
    const char *text = get_attr_str_ptr(attr, "text");
    int reject  = get_attr_int(attr, "reject", 0);

    struct logfilter *filter = logfilter_create(text, (reject)?FILTERRET_REJECT:FILTERRET_HIT);
    
    this->filters = logfilter_lst_add(this->filters, filter);   
    
    PRINT_MVB(&this->base, "syslog added: %s %d", filter->text , filter->type );

    return 0;
}




struct xml_tag module_tags[] = {                         \
    { "filter" , "module" , start_filter, NULL, NULL},   \
    { "" , "" , NULL, NULL, NULL }                       \
};
    


int module_init(struct module_base *module, const char **attr)
{
    struct module_object* this = module_get_struct(module);
    const char* path  = get_attr_str_ptr(attr, "path");
    char buf[512];

    if(! path){
        PRINT_ERR("path is NULL");
        return -1;
    }    

    sprintf(buf,"tail -F %s", path);
    
    this->cmd = strdup(buf);

    return 0;
}


void module_deinit(struct module_base *module)
{
    struct module_object *this = module_get_struct(module);
    logfilter_delete(this->filters);
    free(this->cmd);
}


#define EVENT_BUF_LEN      1024 * 16 



void* module_loop(void *parm)
{
    struct module_object *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;
    FILE *fp;

    char buf[1035];
    
    fp = popen(this->cmd, "r");

    if (fp == NULL) {
	printf("Failed to run command\n" );
	return NULL;
    }    

    base->run = 1;
    
    while(base->run&&(fgets(buf, sizeof(buf)-1, fp) != NULL)){ 
	printf("read : '%s'\n", buf);
    } 
    
    PRINT_MVB(base, "loop returned");
    
    /*closing the INOTIFY instance*/
    pclose( fp );

    return NULL;
    
}

struct module_type module_type = {                  \
    .name       = "syslog",                       \
    .xml_tags   = module_tags,                      \
    .handlers   = NULL ,                        \
    .type_struct_size = sizeof(struct module_object), \
};


struct module_type *module_get_type()
{
    return &module_type;
}


/**
 * @} 
 */

/**
 * @} 
 */
