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

#include "cmddb.h"

#include <module_base.h>

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


enum cvar_type{
    CVAR_TYPE_SET,
    CVAR_TYPE_ACC,
};

struct cmdvar{
    char *name;
    enum cvar_type cvartype;
    enum data_types datatype;
    char *def;
    struct timeval t_update;
    struct event_type *event_type;
    struct cmdvar *next;
};

struct module_object{
    struct module_base base;
    struct cmdvar *vars;
    struct cmddb_desc *descs;
};


struct module_object* module_get_struct(struct module_base *base){
    return (struct module_object*) base;
}



struct cmdvar *cmdvar_create(const char **attr, enum cvar_type cvartype)
{
    struct cmdvar *new = NULL;
    const char *name =  get_attr_str_ptr(attr, "name");
    const char *datatype = get_attr_str_ptr(attr, "datatype");

    if(!datatype)
	datatype = get_attr_str_ptr(attr, "type");

    if(!name)
	return NULL;

    new = malloc(sizeof(*new));
    
    assert(new);

    new->cvartype = cvartype;
    new->t_update.tv_sec  = 0;
    new->t_update.tv_usec = 0;
    new->name =strdup(name);

    new->def = modutil_strdup(get_attr_str_ptr(attr, "default"));
    
    if((!new->def)&&(cvartype ==CVAR_TYPE_ACC ))
	new->def = strdup("0");
	
    new->datatype = uni_data_type_str(datatype); 
    
    new->next = NULL;

    return new;
    
}
    


struct cmdvar *cmdvar_add(struct cmdvar *first, struct cmdvar *new)
{
    struct cmdvar *ptr = first;
    
    if(!first)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return first;
}

void cmdvar_delete(struct cmdvar *var)
{
    if(!var)
	return; 
    
    cmd_var_delete(var->next);

    free(var->name);
    free(var->def);

    free(var);  
    
}


struct uni_data *cmdvar_conv(struct cmdvar *var,const char *value)
{
    return uni_data_create_from_str(var->datatype, value);
}


int cmdvar_initdef(struct cmdvar *var)
{
    char buf[512];
    time_t now = time(NULL);
    time_t t_update = cmddb_value(var->name, buf , sizeof(buf), now);


    if(t_update){

	var->t_update.tv_sec = t_update;
	module_base_send_event(module_event_create(var->event_type, cmdvar_conv(var,buf), &var->t_update));

	return 0;
    }

    return cmddb_insert(CIDAUTO, var->name, var->def, now, 0, now);

}

int cmdvar_cmd_handler(CMD_HND_PARAM)
{
    struct cmdvar *var = (struct cmdvar*) userdata;
    int retval;
    var->t_update.tv_sec = cmd->ttime;

    retval = module_base_send_event(module_event_create(var->event_type, cmdvar_conv(var,cmd->value), &var->t_update));
    
    if(retval){
	char errstr[512];
	int subscnt = event_type_get_subscount(var->event_type);
	if(subscnt == -retval)
	    	snprintf(errstr, sizeof(errstr), 
			 "seterror in all (count %d)", subscnt);
	else
	    snprintf(errstr, sizeof(errstr), 
		     "seterror in %d of %d", -retval,
		     subscnt);
	*retstr = strdup(errstr);
	return CMDSTA_EXECERR;
    }

    return CMDSTA_EXECUTED;
    
}


/**
 * Receive handler for @ref cmdvar 
 * @note See @ref EVENT_HANDLER_PAR for paramaters
 */
int handler_rcv(EVENT_HANDLER_PAR)
{
    struct module_object* this = module_get_struct(handler->base);
    struct cmdvar *var = (struct cmdvar *)handler->objdata;
    struct uni_data *data = event->data;
    char buffer[512];
    float sum = 0;
    float value = 0;
    int update = 1;

   PRINT_MVB(handler->base, "Received from %s.%s name %d datatype %s--------------------------------------\n",  
	     event->source->name, event->type->name, var->cvartype, uni_data_str_type(var->datatype));

    switch(var->cvartype){
      case CVAR_TYPE_ACC:
	cmddb_value(var->name, buffer , sizeof(buffer), time(NULL));
	sscanf(buffer, "%f", &value);
	sum =  uni_data_get_value(data) + value;
	if(sum ==value)
	    update = 0;
	snprintf(buffer, sizeof(buffer), "%f", sum);
	break;
      case CVAR_TYPE_SET:
      default:
	uni_data_get_txt_value(data, buffer, sizeof(buffer));
	break;
    }

    PRINT_MVB(handler->base, "Inserting %p %d datatype %s\n",  var, var->cvartype, uni_data_str_type(var->datatype));

    if(update)
	cmddb_insert(0, var->name, buffer, event->time.tv_sec, 0, time(NULL));

    PRINT_MVB(handler->base, "Inserted %p %d datatype %s <<<<<<<<<<<<<<<<<\n", var, var->cvartype, uni_data_str_type(var->datatype));

    return 0;
}


struct module_event *cmdvar_eread(struct event_type *event)
 {
     struct cmdvar *var = (struct cmdvar*) event->objdata;
     char buf[512];
     struct timeval tv; 
     tv.tv_sec = cmddb_value(var->name, buf , sizeof(buf), time(NULL));
     tv.tv_usec = 0;
     
     return module_event_create(event, cmdvar_conv(var,buf), &tv);
//     return module_event_create(event, cmdvar_conv(var,buf), NULL);

 }



struct event_handler handlers[] = { {.name = ""}};

int module_add_cmdvar(struct module_object* this, struct cmdvar *var, const char **attr)
{
    const char *event_name = get_attr_str_ptr(attr, "event");
    const char *name = get_attr_str_ptr(attr, "name");

    if(!name)
	return -1;

    if(!var)
	return -1;
   
    this->descs = cmddb_desc_add(this->descs, cmddb_desc_create(name, cmdvar_cmd_handler, (void*)var, CMDFLA_SETTING));

    this->vars  = cmdvar_add( this->vars, var);

    var->event_type =  event_type_create_attr(&this->base,(void*)var,attr);
    var->event_type->read = cmdvar_eread;
    
    if(!var->event_type){
        PRINT_ERR("event_type is NULL");
        return -1;
    }    

    this->base.event_types = event_type_add(this->base.event_types, var->event_type); // make it avaliable    
   
    if(event_name){
	struct event_handler *event_handler = NULL;
	/* create event handler */
	event_handler = event_handler_create(get_attr_str_ptr(attr, "name")
					     ,  handler_rcv ,&this->base, (void*)var);

	return module_base_subscribe_event(&this->base, this->base.first, event_name, FLAG_ALL, 
				event_handler, attr);
    }

    PRINT_MVB(&this->base, "name %s datatype %s\n",  var->name, uni_data_str_type(var->datatype));

    return 0;

}


int start_cmdvar(XML_START_PAR)
{
    struct module_object* this = module_get_struct(ele->parent->data);
    struct cmdvar *new = cmdvar_create(attr, CVAR_TYPE_SET);
    

    if(!new)
	return -1;

    return  module_add_cmdvar(this, new, attr);
   
    /* this->descs = cmddb_desc_add(this->descs, cmddb_desc_create(name, cmdvar_cmd_handler, (void*)new, CMDFLA_SETTING)); */

    /* this->vars  = cmdvar_add( this->vars, new); */

    /* new->event_type =  event_type_create_attr(&this->base,(void*)new,attr); */
    /* new->event_type->read = cmdvar_eread; */
    
    /* if(!new->event_type){ */
    /*     PRINT_ERR("event_type is NULL"); */
    /*     return -1; */
    /* }     */

    /* this->base.event_types = event_type_add(this->base.event_types, new->event_type); // make it avaliable     */
   

    /* PRINT_MVB(&this->base, "name %s datatype %s\n",  new->name, uni_data_str_type(new->datatype)); */

    return 0;
}

int start_cmdsum(XML_START_PAR)
{
    struct module_object* this = module_get_struct(ele->parent->data);
    struct cmdvar *new = cmdvar_create(attr, CVAR_TYPE_ACC);

    if(!new)
	return -1;
    
   return  module_add_cmdvar(this, new, attr);
   
}


struct xml_tag module_tags[] = {		   
    { "var" , "module" , start_cmdvar, NULL, NULL}, 
    { "sum" , "module" , start_cmdsum , NULL, NULL},
    { "" , "" , NULL, NULL, NULL }                  
};

int module_init(struct module_base *module, const char **attr)
{
    struct module_object* this = module_get_struct(module);
       
    return cmddb_db_open(get_attr_str_ptr(attr, "path"), /*module->verbose*/0);;

}

void module_deinit(struct module_base *module)
{
    struct module_object* this = module_get_struct(module);
 
    cmddb_db_close();
    cmdvar_delete(this->vars);
    cmddb_desc_delete(this->descs);

    return;
}

void* module_loop(void *parm)
{
    struct module_object *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;
    struct cmdvar *ptr = this->vars;
    int retval;

    base->run = 1;

    PRINT_MVB(base, "Sending initial events");    

    while(ptr){
		cmdvar_initdef(ptr);
		ptr = ptr->next;
    }    

    PRINT_MVB(base, "Initial event send");

    while(base->run){ 
	struct timeval tv;
	gettimeofday(&tv, NULL);
	cmddb_exec_list(this->descs, NULL, tv.tv_sec);
	modutil_sleep_nextsec(&tv);
    } 
    
    PRINT_MVB(base, "loop returned");
    
    return NULL;

}

struct module_type module_type = {                  \
    .name       = "cmddb",                       \
    .xml_tags   = module_tags,                      \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct module_object), \
};


struct module_type *module_get_type()
{
    return &module_type;
}
