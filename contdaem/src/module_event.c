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

#include "module_event.h"

#include <dirent.h> 
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>




unsigned long event_type_get_flags(const char *flagstr, unsigned long def_flag)
{
    unsigned long retflag = def_flag;
    char *str = NULL;
    char *ptr = NULL;
    char *ptr_nxt = NULL;

    if(!flagstr)
	return retflag;

    str = strdup(flagstr);
    ptr = str;

    while(ptr){
		ptr_nxt = strchr(ptr, ',');
		if(ptr_nxt){
			ptr_nxt[0] = '\0';
			ptr_nxt++;
		}
		
		if(strcmp(ptr, "all")==0)
			retflag |= FLAG_ALL;
		else if(strcmp(ptr, "log")==0)
			retflag |=  FLAG_LOG;
		else if (strcmp(ptr, "nolog")==0)
			retflag &= ~FLAG_LOG;
		else if (strcmp(ptr, "show")==0)
			retflag |=  FLAG_SHOW;
		else if (strcmp(ptr, "hide")==0)
			retflag &= ~FLAG_SHOW;
		
		ptr = ptr_nxt;
    }
	
    free(str);

    return retflag;
}

const char *event_type_get_flag_str(unsigned long flags, char *flagstr)
{
	int len = 0;

	if(flags & FLAG_LOG)
		len =+ sprintf(flagstr+len, "log,");
	else 
		len =+ sprintf(flagstr+len, "nolog,");

	if(flags & FLAG_SHOW)
		len =+ sprintf(flagstr+len, "show");
	else 
		len =+ sprintf(flagstr+len, "hide");

	fprintf(stderr, "event_type_get_flag_str(%lX, --): %s", flags , flagstr);

	return flagstr;
}


/**
 * Create a new event type
 */
struct event_type *event_type_create(struct module_base *base, void *objdata,
				     const char *name, const char *unit, const char *hname,
				     unsigned long flags)

//struct event_type *event_type_create(const char *name, const char *unit, const char *hname)
{
    struct event_type *new = NULL;

    if(!name){
        PRINT_ERR("no name");
        return NULL;
    }
   
    new = malloc(sizeof(*new));
    assert(new);
    memset(new, 0, sizeof(*new));
    new->name = strdup(name);

    if(unit){
        new->unit = strdup(unit);
	new->flunit = module_conv_get_level(unit, NULL);
    }

    if(hname)
        new->hname = strdup(hname);

    new->flags = flags;
    assert(base);
    new->base = base;
    new->objdata = objdata;
 
    

    return new;

}


struct event_type *event_type_create_attr(struct module_base *base, void *obj, const char **attr)
{
    return event_type_create(base, obj, 
			     get_attr_str_ptr(attr, "name"), 
			     get_attr_str_ptr(attr, "unit"), 
			     get_attr_str_ptr(attr, "text"), 
			     event_type_get_flags(get_attr_str_ptr(attr, "flags"), base->flags)); 

}



struct event_type *event_type_create_copy(struct module_base *base, void *obj, unsigned long flag, struct event_type *source)
{
    return event_type_create(base, obj, source->name, source->unit, source->hname, flag);    
}


struct event_type *event_type_create_copypart(struct module_base *base, void *obj, const char **attr, struct event_type *source)
{
    const char *unit   = get_attr_str_ptr(attr, "unit");
    const char *name   = get_attr_str_ptr(attr, "name");
    const char *text   = get_attr_str_ptr(attr, "text");

    if(!unit)
	unit = source->unit;

    if(!name)
	name = source->name;

    if(!text)
	text = source->hname;
    
    return event_type_create(base, obj, name, unit, text, 
			     event_type_get_flags(get_attr_str_ptr(attr, "flags"), base->flags));    
}





void event_type_delete(struct event_type *type)
{
    if(type->next)
        event_type_delete(type->next);

    if(type->subscribers)
        callback_list_delete(type->subscribers);

    free(type->name); 
    free(type->unit); 
    free(type->hname);

    free(type); 
}



/**
 * Add a event type to a list
 */
struct event_type *event_type_add(struct event_type *first, struct event_type *new)
{
    struct event_type* ptr = first;

    if(!first)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return first;

}

struct event_type *event_type_list_rem(struct event_type *first, struct event_type *rem)
{
    struct event_type *ptr = first;
    struct event_type *next = NULL;
    
    if(ptr == rem){ //First item is to be removed
        next = rem->next;
        rem->next = NULL;
        return next;
    }

    while(ptr){
        if(ptr->next == rem){
            ptr->next = rem->next;
            rem->next = NULL;
        }
    }
 
    return first;

}



/**
 * Get event type by name
 */
struct event_type *event_type_get_by_name(struct event_type *list, const char *name)
{
    struct event_type* ptr = list;
    
    if(!list)
        return NULL;

    if(name[0]=='*')
	return list;

    while(ptr){
	fprintf(stderr, "%p \"%s\" %s, %p\n",ptr, ptr->name , name,  ptr->next);
        if(strcmp(ptr->name, name)== 0)
            return ptr;
        ptr = ptr->next;
    }

    PRINT_ERR("unknown event %s", name);
    ptr = list;
    fprintf(stderr, "Events in module:");
    while(ptr){
	fprintf(stderr, "%p \"%s\",",ptr, ptr->name );
        ptr = ptr->next;
    }
    fprintf(stderr, "\n");
    return NULL;
    

}

int event_type_get_subscount(struct event_type *etype)
{
    struct callback_list *callback = NULL;
    int count = 0;

    if(etype)
	callback = etype->subscribers;

    while(callback){
	count++;
	callback = callback->next;
    }

    return count;
}

/**
 * Get number of event types
 * @param event_list is the event list
 * @return the number of event types
 */
int event_type_get_count(struct event_type *event_list)
{
    int i = 0;
    if(!event_list)
        return 0;

    while(event_list[i].name[0] != '\0')
        i++;

    return i; 
}



struct module_event *event_type_read(struct event_type *event)
{
    if(!event->read)
	return NULL;

    return event->read(event);
          
}



int event_type_write(struct event_type *event, struct uni_data *data)
{
	int retval; 

    if(!event->write)
	return -EACCES;

    if(!data)
	return -EINVAL;
	
    retval = event->write(event, data);

	uni_data_delete(data);

	return retval;
    
}



/**************************/
/* Event Handlers         */
/**************************/

/**
 * Create event handler 
 */
struct event_handler *event_handler_create(const char *name,  int (*function)(EVENT_HANDLER_PAR) ,struct module_base *base, void *objdata)
{
    struct event_handler *new = malloc(sizeof(*new));

    assert(new);
    assert(name);
    assert(function);

    memset(new, 0 , sizeof(*new));
    
    new->name = strdup(name);
    new->base = base;
    new->objdata = objdata;
    new->function = function;

    return new;
}




struct event_handler *event_handler_list_add(struct event_handler *first , struct event_handler *new)
{
    struct event_handler* ptr = first;

    if(!first)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return first;    
}


void event_handler_delete(struct event_handler *handler)
{
    if(!handler)
	return;

    event_handler_delete(handler->next);

    free(handler->name);
    free(handler);

}




struct event_handler * event_handler_get(struct event_handler *handler_list, const char *name)
{
    //int i = 0;
    struct event_handler *ptr = handler_list;
    
    if(!name)
        return NULL;

    while(ptr){ 
	if(strcmp(ptr->name, name )==0) 
	    return ptr;
	ptr = ptr->next;
    }

    return NULL;   
}





struct callback_list* callback_list_create(struct event_handler *handler)
{
    struct callback_list* new = NULL;
    
    if(!handler){
        PRINT_ERR("no function");
        return NULL;
    }

    new = malloc(sizeof(struct callback_list));

    assert(new);
    
    
    memset(new, 0, sizeof(*new));

    new->handler = handler;

    return new;
}


void callback_list_delete(struct callback_list* call)
{
    if(!call)
	return;
    
    callback_list_delete(call->next);

//    printf("freeing callback\n");

    free(call);
}

struct callback_list* callback_list_add(struct callback_list* first, struct callback_list* new)
{
    struct callback_list* ptr = first;

    if(!first)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return first;

    
}

int callback_list_run(struct callback_list* list, struct module_event* event)
{ 
    while(list){ 
	struct event_handler *handler = list->handler; 
	handler->function(handler, event); 
	list = list->next; 
    } 
    
    return 0;
    
} 


struct module_event* module_event_create(struct event_type *type, struct uni_data *data, 
					 struct timeval *time)
{
    struct module_event *new = malloc(sizeof(*new));
    assert(type->base);
    assert(new);

    memset(new, 0, sizeof(*new));
    new->source = type->base;
    new->type = type;
    new->data = data;
    if(time)
	memcpy(&new->time, time, sizeof(new->time));
    else 
	gettimeofday(&new->time,NULL);
    
    return new;
}

struct module_event* module_event_copy(struct module_event* source)
{
    struct module_event *new = NULL;

    if(!source)
	return NULL;

    //  fprintf(stderr, "copy event\n");

    new = malloc(sizeof(*new));
    assert(new);

    memset(new, 0, sizeof(*new));

    new->source = source->source;
    new->type = source->type;
    memcpy(&new->time, &source->time, sizeof(new->time));

    
    new->data = uni_data_copy(source->data);
    
    // fprintf(stderr, "copyed event\n");

    return new;
    
}


int module_event_get_txt_time( struct module_event *event, char* buffer, int max_size)
{
    int ptr = 0;

    ptr += snprintf(buffer+ptr, max_size-ptr , "%ld.%2.2ld",  
                    event->time.tv_sec, event->time.tv_usec/10000);

    return ptr;
    
}


struct module_event* module_event_add(struct  module_event* list, struct  module_event* new)
{
    struct module_event* ptr = list;

    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;

    
}



void module_event_delete(struct module_event* rem)
{

    //fprintf(stderr, "deleting %p\n", rem);
    if(!rem)
        return;

    module_event_delete(rem->next);

    if(rem->data)
	uni_data_delete(rem->data);
   
    free(rem);
}


int  module_event_sprint(struct  module_event* event, char *buf, int maxlen)
{
    int len = 0;

    assert(event);
    assert(buf);
    assert(event->source);
    assert(event->type);
    assert(event->data);
    snprintf(buf+len,maxlen-len, "%s.%s                ", event->source->name, event->type->name);
    len += 20;
    snprintf(buf+len,maxlen-len, "%s................................                       ", event->type->hname);
    len += 30;

    len += uni_data_get_txt_value(event->data, buf+len, maxlen-len);

    len += snprintf(buf+len,maxlen-len, " [%s]", event->type->unit);

    return len;

}



struct module_base* event_search_next_module__(struct module_base* current, const char *name )
{
    while((current)&&(name)&&(strcmp(name, current->name)!=0)){
	current = current->next;
    }
    
    return current;
}

void event_search_init(struct event_search* search, struct module_base* module_list, const char *name, unsigned long mask)
{
    assert(search);

    if(!module_list){
	PRINT_ERR("module_list is NULL");
    }

    memset(search, 0, sizeof(*search));

    if(name){
	char *dotptr = NULL;
	/* get module and event names */
	strcpy(search->name, name);
	dotptr = strchr(search->name, '.');
	if(dotptr){
	    dotptr[0]= '\0';
	    if(dotptr[1]!='*')
		search->e_name = dotptr+1;
	} else {
	    search->e_name = search->name;
	}
	if(search->name[0]!='*')
	    search->m_name = search->name;
    }

    search->m_curr = event_search_next_module__( module_list, search->m_name);
    if(search->m_curr)
	search->e_next = search->m_curr->event_types;
    else 
	search->e_next = NULL;
    search->mask   = mask;
    search->dbglev = 0;

    if(search->dbglev)
	fprintf(stderr, "Search init module '%s' event '%s' first %p %p\n", 
		search->m_name, search->e_name, search->m_curr, search->e_next );

    return;

}


struct event_type *event_search_next(struct event_search* search)
{
    assert(search);
    
    while(search->m_curr){
	if(search->dbglev)fprintf(stderr, "evt s: m %s\n", search->m_curr->name);

	while(search->e_next){
	    struct event_type *current = search->e_next;
	    if(search->dbglev)fprintf(stderr, "evt s: e %s\n", current->name);
	    search->e_next = search->e_next->next;
	    if((current->flags & search->mask)&&(current->flags & FLAG_AVAI)){
		if(search->dbglev)fprintf(stderr, "OK flags %s\n", current->name);
		if((!search->e_name)||(strcmp(search->e_name, current->name)==0)){
		    if(search->dbglev)fprintf(stderr, "MATCH %s\n", current->name);
		    return current;
		}
	    }
	}

	search->m_curr = event_search_next_module__( search->m_curr->next, search->m_name);
	
	if(search->m_curr)
	    search->e_next = search->m_curr->event_types;


    }
    if(search->dbglev)fprintf(stderr, "not found\n");
    return NULL;
}
