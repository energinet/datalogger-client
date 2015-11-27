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
#include "contdaem_client.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>
#include <xml_doc.h>
#include <xml_item.h>
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#include <syslog.h>

struct xml_cntn_client* def_client = NULL;

#define CD_CLIENT_DEF(client)  client=(client)?client:def_client;


struct xml_item *ContdaemClient_trancive(struct xml_cntn_client* client, struct xml_item *txitem , int timeout)
{
    struct xml_item *rxitem = NULL;
    int errcnt = 10;
    CD_CLIENT_DEF(client);
    char txseq[32];

    syslog(LOG_DEBUG, "%s dbg: client pointer %p\n", __FUNCTION__, client);
//	xml_item_printf(rxitem_); //Could be replaced by xml_item_print(item, char buf, size_t len) for syslog compat

    xml_cntn_client_seq(client, txseq, sizeof(txseq));
    
    if(xml_cntn_client_tx(client, txitem, txseq)<0){
		syslog(LOG_WARNING, "Error sending message\n");
	goto out;
    }

    syslog(LOG_DEBUG, "%s dbg: msg send %p, seq %s\n", __FUNCTION__, txitem, txseq);

    do{
	xml_doc_reset(client->doc);
	
	syslog(LOG_DEBUG, "%s dbg: wating for responce \n", __FUNCTION__);
	struct xml_item *rxitem_ = xml_cntn_client_rx(client,  timeout);
	
	syslog(LOG_DEBUG, "%s dbg: got responce %p\n", __FUNCTION__, rxitem_ );
	if(!rxitem_){
	    syslog(LOG_WARNING, "No received item\n");
	    break;
	}

	const char *rxseq = xml_item_get_attr(rxitem_, "seq");
	syslog(LOG_DEBUG, "%s dbg: got responce seq %s\n", __FUNCTION__, rxseq );

//	xml_item_printf(rxitem_); //Could be replaced by xml_item_print(item, char buf, size_t len) for syslog compat

	if(strcmp(rxseq, txseq)==0)
	    rxitem = xml_item_create_copy(NULL, rxitem_ ,ICOPY_ALL);
	else
	    errcnt--;
    }while(rxitem==NULL&&errcnt<0);
 out:
    return rxitem;
  
}

int ContdaemClient_Init(struct xml_cntn_client** client_ret)
{
    int retval = 0;
    struct xml_cntn_client* client = NULL;
    const char *host = "localhost";
    int port = DEFAULT_CONTDAEM_PORT;

    client = xml_cntn_client_create(1);

    if(xml_cntn_client_open(client, host, port , 10)<0){
		syslog(LOG_ERR, "Error connecting to %s:%d\n", host, port);
	retval = -1;
    }

    syslog(LOG_DEBUG, "%s dbg: client pointer %p\n", __FUNCTION__, client);

    if(client_ret)
	*client_ret = client;
    else
	def_client = client;
 
    return retval;
}

void ContdaemClient_Deinit(struct xml_cntn_client* client)
{
    CD_CLIENT_DEF(client);
    xml_cntn_client_delete(client);
}

struct etype *ContdaemClient_etype_Create(const char *ename, const char *hname, const char *unit, int readable, int writeable)
{
    struct etype *etype = malloc(sizeof(*etype));
    assert(etype);
    
    
    etype->ename = (ename)?strdup(ename):NULL;
    etype->hname = (hname)?strdup(hname):NULL;
    etype->unit  = (unit)?strdup(unit):NULL;
    etype->writeable = writeable;
    etype->readable = readable;
    etype->next = NULL;
    
    return etype;
    
}



void ContdaemClient_etype_Delete(struct etype *etype)
{
    if(!etype)
	return;
    
    ContdaemClient_etype_Delete(etype->next);

    free(etype->ename);
    free(etype->hname);
    free(etype->unit);

    free(etype);
}

struct etype *ContdaemClient_etype_lst_Add(struct etype *lst, struct etype *new)
{
    if(!new)
	return lst;
	
    new->next = lst;
	
    return new;
}


int ContdaemClient_etype_lst_Length(struct etype *lst)
{
    int cnt = 0;
    
    while(lst){
	cnt++;
	lst = lst->next;
    }
    
    return cnt;
    
}

int ContdaemClient_etype_toString(struct etype *etype,  char *out, size_t maxlen)
{
    const char *access = "none";
    if(etype->readable && etype->writeable)
	access = "readwrite";
    else if(etype->readable)	
	access = "readonly";
    else if(etype->writeable)
	access = "writeonly";

    int len = snprintf(out, maxlen, "%s , \"%s\", \"%s\", %s", etype->ename,
		       etype->hname, etype->unit, access);
    out[maxlen-1] = '\0';
    return len;
}


struct etype *ContdaemClient_List(struct xml_cntn_client* client, const char *filter)
{
    struct xml_item *rxitem = NULL;
    struct xml_item *txitem = xml_item_create(NULL, "lst");

    CD_CLIENT_DEF(client);
    syslog(LOG_DEBUG, "%s dbg: client pointer %p\n", __FUNCTION__, client);

    xml_item_add_attr(NULL, txitem, "same", "same");
    
    rxitem = ContdaemClient_trancive(client, txitem, 10000);

/*
No test for whether rxitem is NULL? Segfaults in xml_item_get_child if so...
*/

    struct xml_item *ptr = NULL;
    struct etype *etypes = NULL;

    while((ptr = xml_item_get_child(rxitem, ptr, "etype"))!=NULL){
	const char *read_str = xml_item_get_attr(ptr, "read");
	const char *write_str = xml_item_get_attr(ptr, "write");
	int read = 0, write = 0;
	if(read_str)
	    if(strcmp(read_str, "true")==0)
		read = 1;
	if(write_str)
	    if(strcmp(write_str, "true")==0)
		write = 1;
	
	if(!read&&!write)
	    continue;

	struct etype *etype = ContdaemClient_etype_Create(xml_item_get_attr(ptr, "ename"),
							  xml_item_get_text(ptr),
							  xml_item_get_attr(ptr, "unit"),
							  read, write);

	etypes = ContdaemClient_etype_lst_Add(etypes, etype);
	
    }

    // out:
    
    xml_item_delete(NULL, txitem);
    xml_item_delete(NULL, rxitem);
    return etypes;
    
    
}


int ContdaemClient_ConvTime(struct timeval *tv, const char *str)
{
    double time;

    if(str == NULL || tv == NULL){
	return -1;
    }
    
    if(sscanf(str, "%lf", &time)!=1){
	return -2;
    }
    double integer;
    double fractional = modf(time, &integer);    

    tv->tv_sec = integer;
    tv->tv_usec = fractional*1000000;
    
    return 0;
    
}


int ContdaemClient_Get(struct xml_cntn_client* client, const char *etype, char *out,  size_t maxlen, struct timeval *tv)
{
    struct xml_item *rxitem = NULL;
    struct xml_item *txitem = xml_item_create(NULL, "rd");
    int retval = -1;
    CD_CLIENT_DEF(client);
    syslog(LOG_DEBUG, "%s dbg: client pointer %p\n", __FUNCTION__, client);

    xml_item_add_attr(NULL, txitem, "ename", etype);
    
    


    rxitem = ContdaemClient_trancive(client, txitem, 10000);

    if (!rxitem) {
		syslog(LOG_WARNING, "Transceive function returned NULL");
		goto out;
	}

    const char *etype_attr = xml_item_get_attr(rxitem, "ename");

    if(!etype_attr){
		syslog(LOG_WARNING, "No etype attribute in : %s\n", xml_item_get_tag(rxitem));
	goto out;
    }
	
    if(strcmp(etype_attr, etype)!=0){
		syslog(LOG_WARNING, "etype mismatch : %s != %s\n", etype_attr, etype);
	goto out;
    }
    
    struct xml_item *data = xml_item_get_child(rxitem, NULL, "data");
    
    if(!data){
		syslog(LOG_WARNING, "no data in  %s\n", etype_attr);
	goto out;
    }

    const char *data_str = xml_item_get_text(data);

    if(!data_str){
		syslog(LOG_WARNING, "no data string in data in %s\n", etype_attr);
	goto out;
    }
    
    syslog(LOG_DEBUG, "Response: %s\n", data_str);
    
    ContdaemClient_ConvTime(tv,  xml_item_get_attr(rxitem, "time"));

    strncpy(out, data_str , maxlen);
	syslog(LOG_DEBUG, "Response (out): %s\n", out);
    retval = 0;
 out:
    
    xml_item_delete(NULL, txitem);
    xml_item_delete(NULL, rxitem);
    return retval;
    


}

int ContdaemClient_Set(struct xml_cntn_client* client,const char *etype, const char *value, char *out,  size_t maxlen)
{
      struct xml_item *rxitem = NULL;
    struct xml_item *txitem = xml_item_create(NULL, "wrt");
    struct xml_item *txdata;
    int retval = -1;
    CD_CLIENT_DEF(client);
    syslog(LOG_DEBUG, "%s dbg: client pointer %p\n", __FUNCTION__, client);

    xml_item_add_attr(NULL, txitem, "ename", etype);
    
    txdata = xml_item_create(NULL, "data");
    xml_item_add_text(NULL, txdata, value, 0);
    xml_item_add_child(NULL, txitem, txdata);   

    rxitem = ContdaemClient_trancive(client, txitem, 10000);

    

    const char *etype_attr = xml_item_get_attr(rxitem, "ename");

    if(!etype_attr){
		syslog(LOG_WARNING, "No etype attribute in : %s\n", xml_item_get_tag(rxitem));
		goto out;
    }
	
    if(strcmp(etype_attr, etype)!=0){
		syslog(LOG_WARNING, "etype mismatch : %s != %s\n", etype_attr, etype);
		goto out;
    }
    
    struct xml_item *data = xml_item_get_child(rxitem, NULL, "data");
    
    if(!data){
		syslog(LOG_WARNING, "no data in  %s\n", etype_attr);
		goto out;
    }

    const char *data_str = xml_item_get_text(data);

    if(!data_str){
		syslog(LOG_WARNING, "no data string in data in %s\n", etype_attr);
		goto out;
    }
    
    syslog(LOG_DEBUG, "Responce: %s\n", data_str);
    
    strncpy(out, data_str , maxlen);
    syslog(LOG_DEBUG, "Responce (out): %s\n", out);
    retval = 0;
 out:
    
    xml_item_delete(NULL, txitem);
    xml_item_delete(NULL, rxitem);
    return retval;
    
    
    
}
