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

#include "rpclient_soap.h"

#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#include <errno.h>
#include <assert.h>
#include <syslog.h>

#include "plugin/httpda.h"
#include "rpclient_glob.h"

#include "soapH.h" // obtain the generated stub
#include "liabdatalogger.nsmap" // obtain the namespace mapping table

//#define DEBUG

void rpclient_soap_logfault(struct soap *soap, const char *text)
{
    char errtxt[1024];
    soap_sprint_fault(soap, errtxt, sizeof(errtxt));
    syslog(LOG_ERR, "gSoap fault: %s: %s", text, errtxt);
}


int rpclient_soap_init(struct rpclient_soap *rpsoap)
{
    struct soap *soap= malloc(sizeof(struct soap));
    int dbglev = rpsoap->dbglev;
    char *address = rpsoap->address;

    assert(soap);
    
    struct http_da_info *dainfo = malloc(sizeof(*dainfo));
    memset(dainfo, 0, sizeof(*dainfo));
    rpsoap->dainfo = dainfo;

    syslog(LOG_DEBUG, "Opening soap to '%s'", address);

#ifdef    WITH_OPENSSL
    syslog(LOG_INFO, "SSL enabled");
    if (CRYPTO_thread_setup()){ 
	syslog(LOG_ERR, "Cannot setup SSL thread mutex\n");
        return -EFAULT;
    }
#endif /* WITH_OPENSSL */
    
    soap_init1(soap, SOAP_C_UTFSTRING | SOAP_IO_STORE);

#ifdef DEBUG
    soap_set_recv_logfile(soap, "/root/gsoap_rcv.log"); // append all messages received in /logs/recv/service12.log
    soap_set_sent_logfile(soap, "/root/gsoap_snt.log"); // append all messages sent in /logs/sent/service12.log
    soap_set_test_logfile(soap, "/root/gsoap_tst.log"); // no file name: do not save debug messages  
#endif /* DEBUG */ 

    syslog(LOG_DEBUG , "Register plugin 'http_da'");
    if (soap_register_plugin(soap, http_da)){
	rpclient_soap_logfault(soap, "Faild to register plugin 'http_da'");
    }


#ifdef    WITH_OPENSSL
    syslog(LOG_INFO, "Using SSL");
    if (soap_ssl_client_context(soap,
                                SOAP_SSL_DEFAULT | SOAP_SSL_SKIP_HOST_CHECK,	
                                /* use SOAP_SSL_DEFAULT in production code, */
                                /* we don't want the host name checks since */
                                /* these will change from machine to machine */
                                /* SOAP_SSL_DEFAULT | SOAP_SSL_SKIP_HOST_CHECK, */
                                /* use SOAP_SSL_DEFAULT in production code, we don't */
                                /* want the host name checks since these will change */
                                /* from machine to machine */
								NULL, /* keyfile: required only when client must authenticate */
                                /*to server (see SSL docs on how to obtain this file) */
                                "",		/* password to read the keyfile */
                                CERTDIR"/cacert.pem",	/* optional cacert file to store trusted certificates, */
                                /*use cacerts.pem for all public certificates issued by common CAs */
                                CERTDIR/*NULL*/,		/* optional capath to directory with trusted certificates */
                                NULL		/* if randfile!=NULL: use a file with random data to seed randomness */ 
            )){ 
        soap_print_fault(soap, stderr);
	rpclient_soap_logfault(soap, "Faild to register ssl client");
	return -EFAULT;
    }

    syslog(LOG_DEBUG ,"cacert: %s\n", CERTDIR"/cacert.pem");

#endif /* WITH_OPENSSL */

    rpsoap->soap = soap;
   
    return 0;
}


void rpclient_soap_deinit(struct rpclient_soap *rpsoap)
{
    struct soap *soap= rpsoap->soap;
    rpsoap->soap = NULL;
    
    soap_end(soap); // remove deserialized data and clean up
    soap_done(soap); // detach the gSOAP environment
#ifdef    WITH_OPENSSL
    CRYPTO_thread_cleanup();
#endif /* WITH_OPENSSL */

    free(soap); 
}


int rpclient_soap_hndlerr(struct rpclient_soap *rpsoap,struct soap *soap,  struct http_da_info *info, int retries, const char *funname)
{
    int dbglev = rpsoap->dbglev;

    syslog(LOG_DEBUG,"rpclient_db_handel_err for '%s': soap error %d (retries %d)\n", funname,soap->error , retries);

    if (soap->error == 401) {// challenge: HTTP authentication required 
	if(retries < 2){
	    http_da_save(soap, info, rpsoap->authrealm, rpsoap->username, rpsoap->password);
	} else {
	    http_da_save(soap, info, DEFAULT_REALM ,  DEFAULT_USERNAME, DEFAULT_PASSWORD);
	} 
	syslog(LOG_DEBUG, "Setting login to realm: %s, user: %s, password %s\n", info->authrealm, info->userid, "*******");
    } else {

	char errtxt[1024];
	soap_sprint_fault(soap, errtxt, sizeof(errtxt));
	rpclient_stfile_write("%s fault: %s", funname, errtxt);
	syslog(LOG_ERR , "gSoap error in %s fault: %s\n", funname, errtxt );	
    }


    if(retries > 3)
	return 0; // Maximum number of retries reach, end loop 
    else 
	return 1; // Try again 
    
}

