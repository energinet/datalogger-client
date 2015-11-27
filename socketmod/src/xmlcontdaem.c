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

#include "xmlcontdaem.h"

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
#include <asocket_trx.h>
#include <asocket_server.h>
#include <netinet/tcp.h>

int module_util_xml_to_time(struct timeval *time, const char *str)
{
    if(sscanf(str, "%lu.%lu", &time->tv_sec, &time->tv_usec)!=2)
	return -1;

    time->tv_usec *=1000;

    return 0;
}

struct xml_item *module_event_xml_upd(struct  module_event* event, struct xml_doc *doc)
{
    struct xml_item *upd_item = xml_item_create(doc, "upd");
    
    if(!upd_item){
	PRINT_ERR("no upd_item");
	return NULL;
    }

    xml_item_add_attr(doc, upd_item, "ename", xml_doc_text_printf(doc, "%s.%s", event->source->name, event->type->name));
    xml_item_add_attr(doc, upd_item, "time", xml_doc_text_printf(doc,"%u.%3.3u", event->time.tv_sec, event->time.tv_usec/1000 ));
    if(!xml_item_add_child(doc, upd_item, uni_data_xml_item(event->data, doc, event->type->flunit))){
	PRINT_ERR("could not create data");
	return NULL;
    }

    return upd_item;
}

struct xml_item *event_type_xml(struct event_type *etype, struct xml_doc *doc)
{
    struct xml_item *item = xml_item_create(doc, "etype");
    
	char tmpbuf[128];
	memset(tmpbuf, 0 , sizeof(tmpbuf));

    if(!item){
	PRINT_ERR("item is NULL");
	return NULL;
    }

    xml_item_add_attr(doc, item, "ename", xml_doc_text_printf(doc, "%s.%s", etype->base->name, etype->name));
    if(etype->unit)
		xml_item_add_attr(doc, item, "unit", xml_doc_text_printf(doc, "%s", etype->unit));
    if(etype->write)
	xml_item_add_attr(doc, item, "write","true");

    xml_item_add_attr(doc, item, "flags", xml_doc_text_dup(doc,event_type_get_flag_str(etype->flags, tmpbuf),0));
	
    if(etype->hname)
		xml_item_add_text(doc, item, xml_doc_text_printf(doc, "%s", etype->hname), 0);

    xml_item_add_attr(doc, item, "read", (etype->read)?"true":"false");
    //xml_item_add_attr(doc, item, "write", (etype->write)?"true":"false");
    
	
    return item;
}



struct xml_item *uni_data_xml_item(struct uni_data *data, struct xml_doc *doc, struct convunit *flunit)
{
    struct xml_item *item = xml_item_create(doc, "data");
    char value[256];
    int len;
    if(!item){
	PRINT_ERR("item is NULL");
	return NULL;
    }
/*
    if(data->type != DATA_FLOAT)
	xml_item_add_attr(doc,item, "typ", 
			  uni_data_str_type(data->type));

    if(data->mtime)
	xml_item_add_attr(doc,item, "ms", 
			  xml_doc_text_printf(doc, "%d", data->mtime));
 
	len = uni_data_get_txt_value(data, value, sizeof(value));
*/
	len = uni_data_get_txt_fvalue(data, value, sizeof(value), flunit);

    item->text =  xml_doc_text_dup(doc, value, len);

    return item;

}



struct uni_data *uni_data_create_from_xmli(struct xml_item *item)
{
    enum data_types type =  uni_data_type_str(xml_item_get_attr(item, "typ"));
    
    if(!item->text)
	return NULL;
    
    return uni_data_create_from_str(type, item->text);
}




