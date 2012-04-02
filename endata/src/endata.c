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

#include "endata.h"

#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <stdlib.h> 
#include <assert.h> 

#include "stdsoap2.h"
#include "soapH.h" // obtain the generated stub
#include "SmartGridWebServiceBinding.nsmap" // obtain the namespace mapping table
//#include "endata_soap.h"


const char *endatatypestr[] = {"All","Temperature","Wind speed","Wind direction","Humidity","Indirect radiation","Direct radiation", "Spotprice", "Consumption prognosis", "Thermal production prognosis", "Vindpower prognosis", "Import prognosis", "Imbalaceprices", "CO2", "Grid Tarif", "Trader", "Altitude", "Azimuth", NULL};
                   
const char *endatalang[] = { "DK", "UK" , NULL};

int endata_get_listitem(const char *text, int def, const char** items)
{
    int i = 0;
    if(!text)
	return def;
    
    while(items[i]){
	if(strcmp(text, items[i])==0)
	    return i;
	i++;
    }

    return def;
}

enum endata_lang endata_lang_get(const char *str)
{
	return endata_get_listitem(str, ENDATA_LANG_UK, endatalang);
}

const char *endata_get_liststr(int num, const char** items, int last_ele)
{
	if(num>last_ele)
		return "";

	return items[num];
}


/*******************************/
/* endata_bitem                */
/*******************************/

struct endata_bitem *endata_bitem_create(float value, time_t i_time)
{
	struct endata_bitem *new =  malloc(sizeof(*new));
	assert(new);

	new->value = value;
	new->time  = i_time;
	new->next  = NULL;

	return new;

}

struct endata_bitem *endata_bitem_add(struct endata_bitem *list, 
									  struct endata_bitem *new)
{
	struct endata_bitem *ptr = list;

    if(!list)
        /* first module in the list */
        return new;
	
    /* find the last module in the list */
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;

}

struct endata_bitem *endata_bitem_rem(struct endata_bitem *list, 
									  struct endata_bitem *rem)
{
	struct endata_bitem *ptr = list;
	struct endata_bitem *prev = NULL;

    /* find the last module in the list */
    while(ptr){
        if(rem == ptr){
            if(prev)
                prev->next = ptr->next;
            else
                list = ptr->next;
            
            ptr->next = NULL;
            break;
        }
        prev = ptr;
        ptr = ptr->next;
    }

    return list;
}



void endata_bitem_delete(struct endata_bitem *item)
{
	if(!item)
		return;
	
	endata_bitem_delete(item->next);

	free(item);

}

struct endata_bitem **endata_bitem_value_sort_sub(struct endata_bitem **list, enum endata_sort stype)
{
	struct endata_bitem **prevp = list;
	struct endata_bitem **selcp = NULL;
	struct endata_bitem *ptr = *list;
	float cmpval;

	/* intentionaly inverted */
	switch(stype){
	  case ENDATA_SORT_MINFIRST:
		cmpval = -1000000;
		break;
	  case ENDATA_SORT_MAXFIRST:
		cmpval = 1000000;
		break;
	  default:
		return NULL;
	}
	
//	fprintf(stderr, "list %p ptr %p, \n", list, ptr);

	int stopcount = 30;

	while(ptr){

		if((( ptr->value < cmpval)&&(stype==ENDATA_SORT_MAXFIRST))||
		   (( ptr->value > cmpval)&&(stype==ENDATA_SORT_MINFIRST))){
			selcp = prevp;
			cmpval = ptr->value;
		}
//		fprintf(stderr, "ptr %p ptr->value %f minval %f stopcount %d ptr->next %p\n", ptr, ptr->value,  minval, stopcount,ptr->next );		
		prevp = &ptr->next;
		ptr = ptr->next;

		if(stopcount-- < 0){
			exit(0);
			break;
		}
	}
	
	return selcp;
}


struct endata_bitem *endata_bitem_value_sort(struct endata_bitem *list_old, enum endata_sort stype)
{
	struct endata_bitem *new_list = NULL;
	struct endata_bitem **prevp;
	
//	int stopcount = 240;
//	fprintf(stderr, "list_old %p &list_old %p,stopcount %d \n", list_old, &list_old, stopcount);

	while((prevp = endata_bitem_value_sort_sub(&list_old, stype))){
		struct endata_bitem *ptr  = *prevp;
//		fprintf(stderr, "---%p, *prevp %p, %p, ptr %p list_old %p ptr->next %p\n", prevp, *prevp, &list_old, ptr, list_old, ptr->next);
		
		*prevp = ptr->next;
		
//		fprintf(stderr, "--+%p, *prevp %p, %p, ptr %p list_old %p ptr->next %p\n", prevp, *prevp, &list_old, ptr, list_old, ptr->next);
		ptr->next = new_list;
		new_list = ptr;
//		fprintf(stderr, "-->%p, *prevp %p, %p, ptr %p list_old %p ptr->next %p\n", prevp, *prevp, &list_old, ptr, list_old, ptr->next);
//		if(stopcount-- < 0)
//			break;
	}


	return new_list;

}



struct endata_bitem *endata_bitem_value_get(struct endata_bitem *item, float maxval)
{

	struct endata_bitem *found = NULL;
	float cur_max = 0;

	while(item){
		if((item->value < maxval)&&(item->value > cur_max)){
			cur_max = item->value;
			found = item;
		}
		item = item->next;
	}

	return found;
}
	
void endata_bitem_period(struct endata_bitem *item, time_t *t_begin_, time_t *t_end_, time_t *t_interval_)
{
	time_t t_begin    = 0;
	time_t t_end      = 0;
	time_t t_interval = 0;
	int count = 0;

	struct endata_bitem * ptr = item;
	
	while(ptr){
		if((!t_begin) || (ptr->time<t_begin)){
			t_begin = ptr->time;

		}
		if((!t_end) || (ptr->time>t_end)){
			t_end = ptr->time;

		}

		count++;
		ptr = ptr->next;
	}
	
	if(count >= 2)
		t_interval = (t_end-t_begin)/(count-1);
	
	 t_end+= t_interval;

	*t_begin_ = t_begin;
	*t_end_ = t_end;
	*t_interval_ = t_interval;

//	fprintf(stderr, "t_begin %ld, t_end %ld, t_interval %ld\n", t_begin, t_end, t_interval);
	
	return;

}

int endata_bitem_subsequent(struct endata_bitem *item, struct endata_bitem *list, time_t t_interval)
{
	time_t searchmin = item->time-t_interval;
	time_t searchmax = item->time+(t_interval*2);

	while(list){
		if((list->time >= searchmin)&&
		   (list->time < searchmax)){
			return 1;
		}
		
		list = list->next;
	}

	return 0;
	

}

void endata_bitem_print(struct endata_bitem *item)
{
	time_t t_begin;

	if(!item){
		printf("N/A\n");
		return;
	}
	
	t_begin = item->time;
	
	printf("%ld::",t_begin);

	while(item){
		printf("%2.2ld:%5.2f, ",(item->time-t_begin)/3600, item->value );
		item = item->next;
	}

	printf("\n");
	
	return;
}


/*******************************/
/* endata_block                */
/*******************************/

struct endata_block *endata_block_create(enum endata_type type, const char *unit)
{
	
	struct endata_block *new =  malloc(sizeof(*new));
	assert(new);

	new->type = type;
	if(unit)
		new->unit  = strdup(unit);
	else
		new->unit = NULL;
	new->list  = NULL;
	new->next  = NULL;

	return new;
}

struct endata_block *endata_block_add(struct endata_block *list, struct endata_block *new)
{
	struct endata_block *ptr = list;

    if(!list)
        /* first module in the list */
        return new;
	
    /* find the last module in the list */
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}

struct endata_block *endata_block_rem(struct endata_block *list, struct endata_block *rem)
{
	struct endata_block *ptr = list;
	struct endata_block *prev = NULL;

    /* find the last module in the list */
    while(ptr){
        if(rem == ptr){
            if(prev)
                prev->next = ptr->next;
            else
                list = ptr->next;
            
            ptr->next = NULL;
            break;
        }
        prev = ptr;
        ptr = ptr->next;
    }

    return list;
}

void endata_block_delete(struct endata_block *block)
{
	if(!block)
		return;
	
	endata_block_delete(block->next);

	endata_bitem_delete(block->list);

	free(block);

}


struct endata_block *endata_block_gettype(struct endata_block *list,enum endata_type type )
{
	while(list){
		if(list->type == type)
			return list;
		list = list->next;
	}

	return NULL;
}

void endata_block_print(struct endata_block *block)
{

	if(!block){
		printf("N/A\n");
		return;
	}

	fprintf(stderr, "t_begin %ld, t_end %ld, t_interval %ld\n", block->t_begin, block->t_end, block->t_interval);

	while(block){
		if(block->unit)
			printf("%15s[%4s]->", endatatypestr[block->type], block->unit);
		else
			printf("%21s->", endatatypestr[block->type]);			
		endata_bitem_print(block->list);
		block = block->next;
	}
	
}




#define DEFAULT_REALM "endata"


struct rpclient_soap {
    char *address;
    char *username;
    char *password;
    char *authrealm;
    struct soap *soap;
    int dbglev;
};

#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s: "str"\n",__FUNCTION__, ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}

int endata_soap_init(struct rpclient_soap *rpsoap)
{
    struct soap *soap= malloc(sizeof(struct soap));
    int dbglev = rpsoap->dbglev;
    char *address = rpsoap->address;

    assert(soap);

    PRINT_DBG(dbglev, "Open soap to %s\n", address);

    soap_init1(soap, SOAP_C_UTFSTRING | SOAP_IO_STORE);

#ifdef DEBUG
    soap_set_recv_logfile(soap, "/root/gsoap_rcv.log"); // append all messages received in /logs/recv/service12.log
    soap_set_sent_logfile(soap, "/root/gsoap_snt.log"); // append all messages sent in /logs/sent/service12.log
    soap_set_test_logfile(soap, "/root/gsoap_tst.log"); // no file name: do not save debug messages  
#endif /* DEBUG */ 

    if(rpsoap->username && rpsoap->password) {
        soap->userid = rpsoap->username;
        soap->passwd = rpsoap->password;
	if(rpsoap->authrealm)
	    soap->authrealm = rpsoap->authrealm;
	else
	    soap->authrealm = DEFAULT_REALM;
        printf("Setting login information %s:%s:%s\n", soap->userid, soap->passwd, soap->authrealm);
    }


    
    rpsoap->soap = soap;

    
    return 0;
}





void endata_soap_deinit(struct rpclient_soap *rpsoap)
{
    struct soap *soap= rpsoap->soap;
    rpsoap->soap = NULL;
    
    soap_end(soap); // remove deserialized data and clean up
    soap_done(soap); // detach the gSOAP environment

    free(soap);

    

    
}

char *endata_strdup(const char *string)
{
	if(string)
		return strdup(string);
	else
		return  strdup("");
}

char *endata_intdup(int value)
{
	char string[32];

	sprintf(string, "%d", value);

	return strdup(string);
}


struct endata_block * endata_get(int inst_id, int day , enum endata_lang language, enum endata_type select,
								 const char *address, int dbglev)
{
	struct rpclient_soap rpsoap;
	memset(&rpsoap, 0, sizeof(rpsoap));

	if(address)
		rpsoap.address = strdup(address);
	else 
		rpsoap.address = strdup("http://data.styrdinvarmepumpe.dk/server2.php");
	
	rpsoap.dbglev = dbglev;

	endata_soap_init(&rpsoap);

/*getSmartGrid_Data	*/

	int err;
	struct endata_block *block_list = NULL; 

	struct ns1__getSmartGrid_USCOREDataRequestType  ns1__getSmartGrid_USCOREData;
	struct ns1__getSmartGrid_USCOREDataResponseType ns1__getSmartGrid_USCOREDataResponse;

	memset(&ns1__getSmartGrid_USCOREData, 0, sizeof(ns1__getSmartGrid_USCOREData));

	ns1__getSmartGrid_USCOREData.instID = endata_intdup(inst_id);
	ns1__getSmartGrid_USCOREData.day = endata_intdup(day);;
	ns1__getSmartGrid_USCOREData.locale = endata_strdup(endata_get_liststr(language, endatalang, ENDATA_LANG_UK));
	ns1__getSmartGrid_USCOREData.param = endata_intdup(select);

	err = soap_call___ns1__getSmartGrid_USCOREData(rpsoap.soap, address, NULL, 
												   &ns1__getSmartGrid_USCOREData, 
												   &ns1__getSmartGrid_USCOREDataResponse);


	if(err!=SOAP_OK){
		fprintf(stderr, "test 3\n");
		soap_print_fault(rpsoap.soap, stderr);
        fprintf(stderr,"getSmartGrid_Data fault\n");
		goto out;
	}

	if(ns1__getSmartGrid_USCOREDataResponse.ret){
		int i;
		for(i =0 ; i < ns1__getSmartGrid_USCOREDataResponse.ret->__sizeitem ; i ++){

			struct ns1__SG_USCOREStruct* item  = ns1__getSmartGrid_USCOREDataResponse.ret->item+i;
			int type = atoi(item->paramID);
			struct endata_block *block = endata_block_gettype(block_list,type ); 
			if(!block){
				block = endata_block_create(type, item->Unit);
				block_list = endata_block_add(block_list, block );
			}
			

			block->list = endata_bitem_add(
				block->list ,
				endata_bitem_create( item->Value,item->UTCTime));

			/* printf("%p %ld Name '%s', Value '%lf', Unit '%s', Align '%s', paramID '%d'\n",  */
			/* 	   item,  */
			/* 	   item->UTCTime,  */
			/* 	   item->Name, */
			/* 	   item->Value, */
			/* 	   item->Unit, */
			/* 	   item->Align, */
			/* 	   type */
			/* 	); */

		}
	}

	struct endata_block *block = block_list; 

	while(block){
		endata_bitem_period(block->list, &block->t_begin, &block->t_end, &block->t_interval);
		block = block->next;
	}

  out:
	endata_soap_deinit(&rpsoap);

	return block_list;
}






struct endata_block * endata_get_test(int inst_id)
{
	int i;
	time_t now = time(NULL);
	
	struct endata_block *block_list = NULL;
	struct endata_block *block_cur = NULL;	

	block_cur = endata_block_create(ENDATA_TEMPERATURE, "°C");
	for(i=0; i < 24; i++){
		struct endata_bitem *item = endata_bitem_create(3.5+i, now+(i*3600));
		block_cur->list = endata_bitem_add(block_cur->list , item);
	}
		
	block_list = endata_block_add(block_list, block_cur );
	block_cur = endata_block_create(ENDATA_WINDSPEED, "°m/s");
	for(i=0; i < 24; i++){
		struct endata_bitem *item = endata_bitem_create(3.5+i,now+(i*3600));
		block_cur->list = endata_bitem_add(block_cur->list , item);
	}

	block_list = endata_block_add(block_list, block_cur);

	return block_list;
}
