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
#include "xml_cntn_client.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <syslog.h>

#include <asocket_client.h>

#include "xml_cntn.h"
#include "xml_cntn_trx.h"


struct xml_cntn_client* xml_cntn_client_create(int dbglev)
{
    struct xml_cntn_client* new = malloc(sizeof(*new));
    
    new->fd = -1;
    new->run = 0;
    new->dbglev = dbglev;
    new->doc = xml_doc_create(1024*200, dbglev);
    assert(new->doc); //ToDo: nice error handeling
    new->stack =  xml_stack_create(30,new->doc, dbglev );
    assert(new->stack); //ToDo nice error handeling 

    return new;
}


void xml_cntn_client_delete(struct xml_cntn_client *client)
{
    if(!client)
	return;
    xml_cntn_client_close(client);
    xml_stack_delete(client->stack);
    xml_doc_delete(client->doc);
    free(client);
}

/**
 * Open client conection 
 * @param client The client object.
 * @param host The host address
 * @param port The host port number
 * @param retcnt Retry count
 */
int xml_cntn_client_open(struct xml_cntn_client *client, const char *host, int port , int retcnt)
{
    if(!client){
	PRINT_ERR("No client object");
	return -1;
    }

    struct sockaddr *skaddr;
    int retval = 0;

    if (!host) {
	PRINT_ERR("Unknown host");
	return -1;
    }

    if (port < 0 || port > 0xffff) {
	PRINT_ERR("Invalid port: %d", port);
	return -1;
    }

    client->run = 1;

    skaddr = asocket_addr_create_in(host, port);
    
    if (!skaddr) {
	PRINT_ERR("Invalid address");
	return -1;
    }
    client->fd = asocket_clt_cnt_fd(skaddr,retcnt);

    if (client->fd < 0) {
	PRINT_ERR("Could not connect to modemd\n");
	client->fd = -1;
	retval = -1;
	goto out;
    }

 out:
    asocket_addr_delete(skaddr);  

    return retval;
}


int xml_cntn_client_close(struct xml_cntn_client *client)
{
    assert(client);

    client->run = 0;
    if(client->fd != -1)
	close(client->fd);
    client->fd = -1;
	
    return 0;
}


void xml_cntn_client_seq(struct xml_cntn_client *client, char *seq_str, size_t maxlen)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srandom(tv.tv_usec);
    snprintf(seq_str, maxlen, "%lu", random());   
}

int xml_cntn_client_tx(struct xml_cntn_client *client, struct xml_item *txitem, const char *seq)
{
    assert(client);
  
    if(!client->run)
      return -1;
  
    int retval;
    struct xml_item *txitem_;
    struct xml_doc *doc = client->doc;
    
    doc->first = xml_item_create(doc, "trnf");
    
    txitem_ = xml_item_create_copy(doc,  txitem,1);
    
    if(seq)
        xml_item_add_attr(doc, txitem_, "seq", seq);
    
    xml_item_add_child(doc,doc->first, txitem_);
    
    retval = spxml_cntn_trx_tx(doc, client->fd);
    if(retval < 0)
        PRINT_ERR("ERR >  spxml_cntn_trx_tx returned %d.\n", retval);
    
    return retval;
}


struct xml_item *xml_cntn_client_rx(struct xml_cntn_client *client, int timeout)
{
    assert(client);

    if(!client->run)
	return NULL;

    struct xml_item *rxitem = NULL;
    struct xml_stack *stack = client->stack;
    struct xml_doc *doc = client->doc;

    xml_stack_reset(stack);	
    
    int retval = spxml_cntn_trx_read_doc(client->fd, stack, &client->run,timeout, client->dbglev);
    
    if(retval == -3){
        PRINT_ERR("ERR > spxml_cntn_trx_read_doc returned %d. Exiting \n", retval);
    } else if(retval <-1){
        char outbuf[1024];
        snprintf(outbuf,sizeof(outbuf), "Parse error at line %d:%s",
                 (int)XML_GetCurrentLineNumber( stack->parser),
                 XML_ErrorString(XML_GetErrorCode( stack->parser)));
        syslog(LOG_ERR, "%s", outbuf);
    } else {
        if(doc->first)
            rxitem =  doc->first->childs;       
    } 

    return rxitem;
    

}



struct xml_item *xml_cntn_client_trx(struct xml_cntn_client* client, struct xml_item *txitem , int timeout)
{
    struct xml_item *rxitem = NULL;
    int errcnt = 10;
    char txseq[32];

    //fprintf(stderr, "%s dbg: client pointer %p\n", __FUNCTION__, client);
    //xml_item_printf(txitem);

    xml_cntn_client_seq(client, txseq, sizeof(txseq));
    
    if(xml_cntn_client_tx(client, txitem, txseq)<0){
	//fprintf(stderr, "Error sending message\n");
	syslog(LOG_ERR, "Error sending message\n");
	goto out;
    }

    //    fprintf(stderr, "%s dbg: msg send %p, seq %s\n", __FUNCTION__, txitem, txseq);

    do{
	xml_doc_reset(client->doc);
	
	//	fprintf(stderr, "%s dbg: wating for responce \n", __FUNCTION__);
	struct xml_item *rxitem_ = xml_cntn_client_rx(client,  timeout);
	
	//	fprintf(stderr, "%s dbg: got responce %p\n", __FUNCTION__, rxitem_ );
	if(!rxitem_){
	    //	    fprintf(stderr, "No received item\n");
	    syslog(LOG_ERR, "No received item\n");
	    break;
	}

	const char *rxseq = xml_item_get_attr(rxitem_, "seq");
	//	fprintf(stderr, "%s dbg: got responce seq %s\n", __FUNCTION__, rxseq );
	xml_item_printf(rxitem_);
	if(strcmp(rxseq, txseq)==0)
	    rxitem = xml_item_create_copy(NULL, rxitem_ ,ICOPY_ALL);
	else
	    errcnt--;
    }while(rxitem==NULL&&errcnt<0);
 out:
    return rxitem;
  
}

