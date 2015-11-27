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
#include "sdvpd_xml.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <syslog.h>

#include "sdvpd_item.h"
#include "xml_parser.h"

#include "contdaem_client.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif


#define XML_ROOT "map"

struct sdvpd_xml_parse{
    struct sdvp_map *map;
    struct etype *etypes;
};


int sdvpd_xml_etype_cmp(const char *str, const char *ename)
{
    if(str[0] == '*'){
	char *name1 = strchr(str , '.');
	assert(name1);
	char *name2 = strchr(ename, '.');
	assert(name2);
	return strcmp(name1,name2);
    }

    return strcmp(str,ename);
    
}



struct etype *sdvpd_xml_etype_find(struct etype *lst, const char *ename){
    
    struct etype *ptr = lst;
   

    
    
    while(ptr){
	if(sdvpd_xml_etype_cmp(ename, ptr->ename)==0)
	    return ptr;
	ptr = ptr->next;
    }

    return NULL;
}

int sdvpd_xml_start_map(XML_START_PAR){
    struct sdvpd_xml_parse *pobj = (struct sdvpd_xml_parse *)data;
    pobj->map = sdvp_map_Create("", NULL);

    ele->data = pobj->map;    
   
    return 0;
}

void sdvpd_xml_end_map(XML_END_PAR){

    ele->data = NULL;    
    
    return;
}

int sdvpd_xml_start_dir(XML_START_PAR){
    struct sdvpd_xml_parse *pobj = (struct sdvpd_xml_parse *)data;
    struct sdvp_map *parent =   (struct sdvp_map *)ele->parent->data;

    if(!parent)
	return 0;

    const char *name = get_attr_str_ptr(attr, "name");

    if(!name){
	syslog(LOG_WARNING, "tag '%s' must have a 'name' attrubute", el);
	return -1;
    }	


    const char *test = get_attr_str_ptr(attr, "if");
    if(test){
	struct etype *etype = sdvpd_xml_etype_find(pobj->etypes, test);
	if(!etype)
	    return 0;

    }
    
    struct sdvp_map *object = sdvp_map_AddChild(parent, sdvp_map_Create(name, NULL));   

    ele->data = object;    

    return 0;
}

void sdvpd_xml_end_dir(XML_END_PAR){
    //    struct sdvpd_xml_parse *pobj = (struct sdvpd_xml_parse *)data;

    ele->data = NULL;

    return;
}

int sdvpd_xml_start_item(XML_START_PAR){
    struct sdvpd_xml_parse *pobj = (struct sdvpd_xml_parse *)data;
    struct sdvp_map *parent =   (struct sdvp_map *)ele->parent->data;

    const char *name = get_attr_str_ptr(attr, "name");

    if(!parent)
	return 0;

    if(!name){
		syslog(LOG_WARNING, "tag '%s' must have a 'name' attrubute", el);
	return -1;
    }	

    const char *ename = get_attr_str_ptr(attr, "ename");
    const char *cmd = get_attr_str_ptr(attr, "cmd");

    if(!ename&&!cmd){
		syslog(LOG_WARNING, "tag '%s' must have a 'ename' attrubute", el);
	return -1;
    }	

    const char *type_str = get_attr_str_ptr(attr, "type");

    if(!type_str){
		syslog(LOG_WARNING, "tag '%s' must have a 'type' attrubute", el);
	return -1;
    }	

    struct sdvp_item *item = NULL;
    enum sdvpd_item_type type = CT_DIRECT;

    if(cmd){

	if(strcmp(type_str, "operate")==0)
	    type = CT_CMD;
	else if (strcmp(type_str, "cancel")==0)
	    type = CT_CMD_CANCEL;
	syslog(LOG_DEBUG, "cmd type %s => %d\n", type_str, type);

	item = sdvp_item_Create(sdvpd_item_Init(cmd, type));
    } else  if(ename){
	struct etype *etype = sdvpd_xml_etype_find(pobj->etypes, ename);
	if(etype==NULL)
	    return 0;

	ename = etype->ename;
	item = sdvp_item_Create(sdvpd_item_Init(ename, type));
    }

    
    sdvp_map_AddChild(parent, sdvp_map_Create(name, item));   

    return 0;
}




/**
 * XML Base tags
 */
struct xml_tag base_xml_tags[] = {                      
    { "item" , "dir" , sdvpd_xml_start_item, NULL, NULL},       
    { "dir" , XML_ROOT , sdvpd_xml_start_dir, NULL, sdvpd_xml_end_dir},  
    { "dir" , "dir", sdvpd_xml_start_dir, NULL, sdvpd_xml_end_dir},       
    { XML_ROOT , "" , sdvpd_xml_start_map, NULL, sdvpd_xml_end_map},       
    { "" , "" , NULL, NULL, NULL }               
};
 

struct sdvp_map * sdvpd_xml_LoadMap(const char *path, struct etype *etypes)
{
    struct sdvpd_xml_parse pobj;
    
    pobj.map = NULL;
    pobj.etypes = etypes;

    struct xml_tag *taglist = NULL;
    taglist = xml_tag_add_lst(taglist, base_xml_tags);
    
    if(parse_xml_file(path, taglist, &pobj)<0){
	pobj.map = NULL;
    }

    free(taglist);

    return pobj.map;

}
