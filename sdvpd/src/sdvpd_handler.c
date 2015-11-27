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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sdvp.h>
#include <stdbool.h>
#include <syslog.h>
#include <assert.h>
#include <cmddb.h>
#include <sys/time.h>

#include "sdvpd_item.h"
#include "sdvpd_xml.h"

#include "contdaem_client.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif

struct sdvp_map *map = NULL;

struct sdvp_item *sdvpd_handler_InitItem(const char *name, enum sdvpd_item_type type )
{
    struct sdvp_item *item = sdvp_item_Create(sdvpd_item_Init(name, type));
    syslog(LOG_DEBUG, "Created item '%p'\n", item);    
    return item;

}

void sdvpd_handler_InitDeitem(struct sdvp_item *item)
{
    struct sdvpd_item *citem = (struct sdvpd_item *) item->userdata;
    sdvpd_item_Delete(citem);
    sdvp_item_Delete(item);
}



int sdvpd_handler_ReadItem(struct sdvp_item *item, char *buf, size_t maxlen, struct timeval *tv)
{
    syslog(LOG_DEBUG, "Reading item '%p'\n", item);    
    if(!item)
	return -1;

    struct sdvpd_item *citem = (struct sdvpd_item *) item->userdata;

    switch(citem->type){
    case CT_DIRECT:{
	const char *ename = citem->name;
	syslog(LOG_DEBUG, "Reading value of local id '%s'\n", ename  );
	return ContdaemClient_Get(NULL, ename, buf, maxlen,tv);
    }
    case CT_CMD:{
	syslog(LOG_DEBUG, "Reading value of local id '%s' (cmd) \n", citem->name  );
	tv->tv_sec = cmddb_value(citem->name, buf, maxlen, time(NULL));
	tv->tv_usec = 0;
	return strlen(buf);
    }
	//    case CT_HYBRID:
    default:
	return -1;
    }
}


int sdvpd_handler_WriteItem(struct sdvp_item *item, const char *value, char *buf, size_t maxlen
			    , struct timeval *tv)
{
    if(!item)
	return -1;
    
    struct sdvpd_item *citem = (struct sdvpd_item *) item->userdata;

    switch(citem->type){
    case CT_DIRECT:{
	const char *ename = citem->name;
	syslog(LOG_DEBUG, "Writing value of local id '%s' \n", citem->name  );
	return ContdaemClient_Set(NULL, ename, value, buf,maxlen);
    }
    case CT_CMD:{
	syslog(LOG_DEBUG, "Writing value of local id '%s' (cmd) \n", citem->name  );
	cmddb_cancel( citem->name);
	cmddb_insert(0, citem->name, value, tv->tv_sec, 0, time(NULL));
	return snprintf(buf, maxlen, "%s", value); 
    } 
    case CT_CMD_CANCEL:
	syslog(LOG_DEBUG, "Canceling value of local id '%s' (cmd) \n", citem->name  );
	cmddb_cancel( citem->name);
	return snprintf(buf, maxlen, "%s", value); 
	break;
    default:
	return -1;
    }
}



/**
 * Initialize a map for the IEC 61850 items
 * @return a map tree
 */
int sdvpd_handler_InitMap()
{
    map = sdvp_map_Create("", NULL);
    struct sdvp_map *ldliab = sdvp_map_AddChild(map, sdvp_map_Create("LD_LIAB", NULL));
    struct sdvp_map *lnhp = sdvp_map_AddChild(ldliab, sdvp_map_Create("LN_HP", NULL));
    
    struct sdvp_map *object = sdvp_map_AddChild(lnhp, sdvp_map_Create("heating", NULL));
    sdvp_map_AddChild(object, sdvp_map_Create("htret", 
					      sdvpd_handler_InitItem("pt1000.htret", CT_DIRECT) ) );
    sdvp_map_AddChild(object, sdvp_map_Create("htfwd", 
					      sdvpd_handler_InitItem("vfs.htfwd", CT_DIRECT) ) );
    sdvp_map_AddChild(object, sdvp_map_Create("hflow", 
					      sdvpd_handler_InitItem("vfs.hflow", CT_DIRECT) ) );
    sdvp_map_AddChild(object, sdvp_map_Create("hpwr", 
					      sdvpd_handler_InitItem("wpower2.pwrh", CT_DIRECT) ) );

    object = sdvp_map_AddChild(lnhp, sdvp_map_Create("hotwater", NULL));
    sdvp_map_AddChild(object, sdvp_map_Create("whout", 
					      sdvpd_handler_InitItem("pt1000.whout", CT_DIRECT) ) );
    sdvp_map_AddChild(object, sdvp_map_Create("wcin", 
					      sdvpd_handler_InitItem("vfs.wcin", CT_DIRECT) ) );
    sdvp_map_AddChild(object, sdvp_map_Create("wflow", 
					      sdvpd_handler_InitItem("vfs.wflow", CT_DIRECT) ) );
    sdvp_map_AddChild(object, sdvp_map_Create("wprw", 
					      sdvpd_handler_InitItem("wpower1.pwrw", CT_DIRECT) ) );


    object = sdvp_map_AddChild(lnhp, sdvp_map_Create("heatpump", NULL));    
    sdvp_map_AddChild(object, sdvp_map_Create("pwhp", 
					      sdvpd_handler_InitItem("inputs.pwhp", CT_DIRECT) ) );
   sdvp_map_AddChild(object, sdvp_map_Create("button", 
					      sdvpd_handler_InitItem("inputs.button", CT_DIRECT) ) );
    sdvp_map_AddChild(object, sdvp_map_Create("tttop", 
					      sdvpd_handler_InitItem("pt1000.tttop", CT_DIRECT) ) );
    
    
    sdvp_map_AddChild(object, sdvp_map_Create("relay1", 
					      sdvpd_handler_InitItem("relays.relay1", CT_DIRECT)));
    sdvp_map_AddChild(object, sdvp_map_Create("relay2", 
					      sdvpd_handler_InitItem("relays.relay2", CT_DIRECT)));
    sdvp_map_AddChild(object, sdvp_map_Create("relay3", 
					      sdvpd_handler_InitItem("relays.relay3", CT_DIRECT)) );
    
    sdvp_map_AddChild(object, sdvp_map_Create("pws_relay1", 
					      sdvpd_handler_InitItem("pws_relay1", CT_CMD)) );
    sdvp_map_AddChild(object, sdvp_map_Create("pws_relay2", 
					      sdvpd_handler_InitItem("pws_relay2", CT_CMD)) );
    sdvp_map_AddChild(object, sdvp_map_Create("pws_relay3", 
					      sdvpd_handler_InitItem("pws_relay3", CT_CMD)) );
 
    object = sdvp_map_AddChild(lnhp, sdvp_map_Create("extern", NULL));
    sdvp_map_AddChild(object, sdvp_map_Create("rtind", 
					      sdvpd_handler_InitItem("modbus.htfwd", CT_DIRECT) ) );
    sdvp_map_AddChild(object, sdvp_map_Create("rtout", 
					      sdvpd_handler_InitItem("modbus.htfwd", CT_DIRECT) ) );
    sdvp_map_AddChild(object, sdvp_map_Create("pwmtr", 
					      sdvpd_handler_InitItem("inputs.pwmtr", CT_DIRECT) ) );
    

    struct sdvp_map *lnint = sdvp_map_AddChild(ldliab,sdvp_map_Create("LN_INT", NULL));
    struct sdvp_map *lm81  = sdvp_map_AddChild(lnint, sdvp_map_Create("lm81", NULL));
    
    sdvp_map_AddChild(lm81, sdvp_map_Create("intern", 
					    sdvpd_handler_InitItem("lm81.intern", CT_DIRECT)) );
    sdvp_map_AddChild(lm81, sdvp_map_Create("v18", 
					    sdvpd_handler_InitItem("lm81.v18", CT_DIRECT)) );
    sdvp_map_AddChild(lm81, sdvp_map_Create("v33", 
					    sdvpd_handler_InitItem("lm81.v33", CT_DIRECT)) );
    sdvp_map_AddChild(lm81, sdvp_map_Create("v50", 
					    sdvpd_handler_InitItem("lm81.v50", CT_DIRECT)) );
    
    struct sdvp_map *ptr  = map;

    while(ptr){
	char buf[128];
	sdvp_item_GetRef(ptr, buf, sizeof(buf));
	syslog(LOG_INFO, "MMS: %s %p %p\n", buf, ptr, ptr->item);
	ptr = sdvp_map_TreeNext(ptr);
    }

    return 0;
}

/**
 * Initialize a map for the IEC 61850 items
 * @return a map tree
 */
int sdvpd_handler_InitMapFile(const char *path)
{

    struct etype *etypes = ContdaemClient_List(NULL, NULL);


    map = sdvpd_xml_LoadMap(path, etypes);

    struct sdvp_map *ptr  = map; 

    syslog(LOG_ERR, "MMS map %p\n", ptr);

    if(ptr == NULL){
	syslog(LOG_ERR, "Error MMS map is empty\n");
	return -1;
    }

    while(ptr){
	char buf[128];
	sdvp_item_GetRef(ptr, buf, sizeof(buf));
	syslog(LOG_INFO, "MMS: %s %p %p\n", buf, ptr, ptr->item);
	ptr = sdvp_map_TreeNext(ptr);
    }

    return 0;
}

/**
 * Test function 
 * @param mms The mms reference 
 */
void sdvpd_handler_TestMap(const char *mms)
{
    struct sdvp_map *ptr = sdvp_map_SearchRef(map, mms);
    char buf[128];
    if(ptr)
	ptr = ptr->childs;
 
    while(ptr){
	sdvp_item_GetRef(ptr, buf, sizeof(buf));
	ptr = sdvp_map_ListNext(ptr);
    }
}


sdvp_GetNameListReply_t* sdvpd_handler_cbGetNameList (sdvp_GetNameListParam_t* params) 
{
    sdvp_GetNameListReply_t* rv;
    struct sdvp_map *ptr = map;
    int i = 0;
    syslog(LOG_DEBUG, "sdvpd_handler_cbGetNameList %p", params);    


    assert(params);
    
    ptr = sdvp_map_SearchRef(map, params->params.objectClass);
    
    if(ptr)
	ptr = ptr->childs;

    rv = sdvp_InitiateGetNameListReply(sdvp_map_ListSize(ptr));
    syslog(LOG_DEBUG, "size: %d", sdvp_map_ListSize(ptr));


    while(ptr){
	char buf[128];
	sdvp_item_GetRef(ptr, buf, sizeof(buf));
	syslog(LOG_DEBUG, "mms: %s", buf);
	rv->listOfIdentifier[i++] = strdup(buf);
	ptr = sdvp_map_ListNext(ptr);
    }
    return rv;
}

sdvp_GetVariableAccessAttributeReply_t* 
sdvpd_handler_cbGetVariableAccessAttributes (sdvp_GetVariableAccessAttributeParam_t* params)
{
    assert(params);
	sdvp_GetVariableAccessAttributeReply_t *rv;

    syslog(LOG_DEBUG, "cbGetVariableAccessAttributes: %s",
	   params->dataReference );

    struct sdvp_map *ptr = map;
    
    ptr = sdvp_map_SearchRef(map, params->dataReference);

    if(ptr)
	ptr = ptr->childs;

	rv = sdvp_InitiateGetVariableAccessAttributesReply(sdvp_map_ListSize(ptr));

    int i = 0;

    while(ptr){
		char buf[128];
		sdvp_item_GetRef(ptr, buf, sizeof(buf));
		syslog(LOG_DEBUG, "mms: %s", buf);
		rv->dataAttributeNames[i++] = strdup(buf);
		ptr = sdvp_map_ListNext(ptr);
    }
    
    rv->serviceError =  SERV_ERROR_NO_ERROR;
    
    return rv;
}

sdvp_ReadReply_t* sdvpd_handler_cbRead (sdvp_ReadParam_t* params)
{
    sdvp_ReadReply_t* rv;
    char value_str[512] = "";
    char ref_str[512] = "";
    struct timeval tv;
   
    struct sdvp_map *ptr = sdvp_map_SearchRef(map, params->reference);
    syslog(LOG_DEBUG, "Found map for reading '%p' %s \n", ptr , (ptr)?ptr->name:"");
    if(ptr){
		rv = sdvp_InitiateReadReply(3);
		sdvpd_handler_ReadItem(ptr->item, value_str, sizeof(value_str), &tv);
		sdvp_item_GetRef(ptr, ref_str, sizeof(ref_str));
		rv->listOfAccessResults[0]->strValue = strdup(ref_str);
		rv->listOfAccessResults[0]->type = IEC_VISIBLE_STRING;
		rv->listOfAccessResults[1]->strValue = strdup(value_str);
		rv->listOfAccessResults[1]->type =  IEC_UNICODE_STRING;
		rv->listOfAccessResults[2]->intValue = tv.tv_sec;
		rv->listOfAccessResults[2]->type =  IEC_TIME;
    } else {
		// ToDo: hvordan returnere man en fejl/???
		rv = sdvp_InitiateReadReply(1);
		rv->listOfAccessResults[0]->boolValue = false;
		rv->listOfAccessResults[0]->type = IEC_BOOLEAN;
    }

    return rv;    
}

int sdvpd_handler_GetWparamValue(sdvp_write_params_t* wparam, char *buf, size_t maxlen, 
				 struct timeval *tv)
{
    if(!wparam){
	goto out;
    }
    
    if(wparam->numberOfDataAttValues<1){
	goto out;
    }

    if(wparam->numberOfDataAttValues==2 && wparam->dataAttributeValues[1]->dataType == IEC_TIME){
	    tv->tv_sec = wparam->dataAttributeValues[1]->time;
	    tv->tv_usec = 0;
    } else {
		gettimeofday(tv, NULL);
    }

    
    switch(wparam->dataAttributeValues[0]->dataType){
    case IEC_BOOLEAN:
	return snprintf(buf,maxlen, "%s", (wparam->dataAttributeValues[0]->boolValue)?"true":"false");
    case IEC_INT8:
    case IEC_INT16:
    case IEC_INT32:
	return snprintf(buf,maxlen, "%d", wparam->dataAttributeValues[0]->intValue);
    case IEC_INT8U:
    case IEC_INT16U:
    case IEC_INT32U:
	return snprintf(buf,maxlen, "%u", wparam->dataAttributeValues[0]->uintValue);
    case IEC_FLOAT32:
	return snprintf(buf,maxlen, "%f", wparam->dataAttributeValues[0]->f32Value);
	//case IEC_FLOAT64:
	//case IEC_ENUMERATED:
	//case IEC_CODED_ENUM:
	//case IEC_OCTET_STRING:
    case IEC_VISIBLE_STRING:
    case IEC_UNICODE_STRING: //ToDo convert???
	return snprintf(buf,maxlen, "%s", wparam->dataAttributeValues[0]->strValue);
	break;
	// case IEC_STRUCT:
    default:
	buf[0] = '\0';
	return -2;
    }

 out:
    buf[0] = '\0';
    return -1;
}


sdvp_WriteReply_t* sdvpd_handler_cbWrite (sdvp_WriteParam_t* params)
{
    sdvp_WriteReply_t* rv;
    char value_str[512] = "";
    char write_str[512] = "";
    int success = false;
    sdvp_write_params_t* wparam = params->params;
    struct timeval tv;

    if(!wparam)
	goto out; 

    gettimeofday(&tv, NULL);

    //    tv.tv_sec += 10;

    struct sdvp_map *ptr = sdvp_map_SearchRef(map, wparam->reference);

    if(ptr && sdvpd_handler_GetWparamValue(wparam, write_str, sizeof(write_str), &tv)){
	    syslog(LOG_DEBUG, "Found map for writing '%s' to  %s \n", write_str , ptr->name);
	    sdvpd_handler_WriteItem(ptr->item, write_str, value_str, sizeof(value_str),&tv);
	    success = true;
    } else {
	    syslog(LOG_INFO, "Could not write '%s' \n", wparam->reference);
    }
 out:
    
    rv = sdvp_InitiateWriteReply(success);
    
    return rv;
}

sdvp_reply_params_t* sdvpd_handler_Callback (sdvp_method_t methodType, sdvp_from_t* from,
					     sdvp_params_t* params) {
    sdvp_reply_params_t* replyParams = NULL;
    char value_str[512] = "";
    struct timeval tv;
    gettimeofday(&tv, NULL);

    syslog(LOG_DEBUG, "Got call from %s: %s (%s) (params %d)\n", from->name, from->jid, from->group, params->numberOfParams);

    switch (methodType) {
    case SDVP_METHOD_UNDEFINED:
        syslog(LOG_WARNING, "Warning: Undefined method called!\n");
        break;
    case SDVP_METHOD_61850_READ:
	if (params->numberOfParams == 1) {    
	    syslog(LOG_DEBUG, "Wants to read '%s' \n", params->params[0].value );
	    //  ContdaemClient_Get(NULL, "pt1000.htret", value_str, sizeof(value_str));	
	    struct sdvp_map *ptr = sdvp_map_SearchRef(map, params->params[0].value);
	    syslog(LOG_DEBUG, "Found map '%p' %s \n", ptr , (ptr)?ptr->name:"");

	    if(ptr)
		sdvpd_handler_ReadItem(ptr->item, value_str, sizeof(value_str), &tv);
	    sdvp_InitiateReplyParameters(&replyParams, 2);
	    replyParams->params[0].strValue = strdup(value_str);
	    replyParams->params[0].type = IEC_UNICODE_STRING;
	    replyParams->params[1].strValue = strdup("Reply param 2\n");
	    replyParams->params[1].type = IEC_UNICODE_STRING;
	} else {
	    syslog(LOG_ERR, "Error: %d!=1 params in 61850 read!\n", params->numberOfParams);
	}
        break;
    case SDVP_METHOD_61850_WRITE:
        if (params->numberOfParams == 2) {
            syslog(LOG_DEBUG, "Wants to write '%s' to '%s'\n", params->params[1].value , params->params[0].value );
	    // ContdaemClient_Set(NULL,  "relays.relay3",  params->params[1].value,
	    //		       value_str, sizeof(value_str));
	    struct sdvp_map *ptr = sdvp_map_SearchRef(map, params->params[0].value);
	    if(ptr)
		sdvpd_handler_WriteItem(ptr->item, params->params[1].value, 
					value_str, sizeof(value_str), &tv);
	    sdvp_InitiateReplyParameters(&replyParams, 2);
	    replyParams->params[0].strValue = strdup(value_str);
	    replyParams->params[0].type = IEC_UNICODE_STRING;
	    replyParams->params[1].strValue = strdup("Reply param 2\n");
	    replyParams->params[1].type = IEC_UNICODE_STRING;
	} else {
	    syslog(LOG_ERR, "Error: %d!=2 params in 61850 write!\n", params->numberOfParams);
	}
        break;
    case SDVP_METHOD_DIRECT_READ:
        if (params->numberOfParams == 1) {
	    syslog(LOG_DEBUG, "Wants to read '%s'\n", params->params[0].value);
	    ContdaemClient_Get(NULL, params->params[0].value, value_str, sizeof(value_str), NULL);	
	    sdvp_InitiateReplyParameters(&replyParams, 1);
            replyParams->params[0].strValue = strdup(value_str);
        } else {
	    syslog(LOG_ERR, "Error: %d!=1 params in direct read!\n", params->numberOfParams);
	}
        break;
    case SDVP_METHOD_DIRECT_WRITE:
        if (params->numberOfParams == 2) {
            syslog(LOG_DEBUG, "Wants to write '%s' to '%s'\n", params->params[1].value,
                    params->params[0].value);
	    ContdaemClient_Set(NULL,  params->params[0].value,  params->params[1].value,
			       value_str, sizeof(value_str));	
	    sdvp_InitiateReplyParameters(&replyParams, 1);
            replyParams->params[0].strValue = strdup(value_str);
	    replyParams->params[0].type = IEC_UNICODE_STRING;
        } else {
	    syslog(LOG_ERR, "Error: %d!=2 params in direct write!\n", params->numberOfParams);
	}
        break;
    case SDVP_METHOD_DIRECT_LIST:
	syslog(LOG_DEBUG, "Wants to get list\n");
	break;
    case SDVP_METHOD_LIAB_LIST:
	if(params->numberOfParams == 0){
	    syslog(LOG_DEBUG, "Wants to get LIAB list\n");
	    struct etype *etypes = ContdaemClient_List(NULL, NULL);
	    struct etype *ptr = etypes;
	    int i = 0;
	    sdvp_InitiateReplyParameters(&replyParams, ContdaemClient_etype_lst_Length(etypes));
	    while(ptr){
		ContdaemClient_etype_toString(ptr, value_str, sizeof(value_str));
		replyParams->params[i].type = IEC_UNICODE_STRING;
		replyParams->params[i++].strValue = strdup(value_str);
		ptr = ptr->next;
	    }
	    ContdaemClient_etype_Delete(etypes);
	} else {
	    syslog(LOG_ERR, "Error: %d!=2 params in direct write!\n", params->numberOfParams);
	}
	break;
    default:
	syslog(LOG_WARNING, "Warning: Got unsupported call from libsdvp %d!\n", methodType );
	break;
    }

    return replyParams;
}
