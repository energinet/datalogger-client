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

#include <errno.h>
#include <assert.h>

#include "soapH.h" // obtain the generated stub
#include "liabdatalogger.nsmap" // obtain the namespace mapping table

#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s: "str"\n",__FUNCTION__, ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}

#define DEFAULT_REALM "Mikkels realm"

#ifdef    WITH_OPENSSL
int CRYPTO_thread_setup();
void CRYPTO_thread_cleanup();
#endif /* WITH_OPENSSL */



int rpclient_soap_init(struct rpclient_soap *rpsoap)
{
    struct soap *soap= malloc(sizeof(struct soap));
    int dbglev = rpsoap->dbglev;
    char *address = rpsoap->address;

    assert(soap);

    PRINT_DBG(dbglev, "Open soap to %s\n", address);

#ifdef    WITH_OPENSSL
    PRINT_DBG(dbglev, "SSL enabled\n");
    if (CRYPTO_thread_setup()){ 
	PRINT_ERR("Cannot setup thread mutex\n");
        return -EFAULT;
    }
#endif /* WITH_OPENSSL */
    
    soap_init1(soap, SOAP_C_UTFSTRING | SOAP_IO_STORE);

#ifdef DEBUG
    soap_set_recv_logfile(soap, "/root/gsoap_rcv.log"); // append all messages received in /logs/recv/service12.log
    soap_set_sent_logfile(soap, "/root/gsoap_snt.log"); // append all messages sent in /logs/sent/service12.log
    soap_set_test_logfile(soap, "/root/gsoap_tst.log"); // no file name: do not save debug messages  
#endif /* DEBUG */ 


#ifdef    WITH_OPENSSL
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
        PRINT_ERR("soap_print_fault\n");
	return -EFAULT;
    }

    PRINT_DBG(dbglev,"cacert: %s\n", CERTDIR"/cacert.pem");

#endif /* WITH_OPENSSL */

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


/******************************************************************************\
 *
 *	OpenSSL
 *
\******************************************************************************/

#ifdef WITH_OPENSSL

#if defined(WIN32)
# define MUTEX_TYPE		HANDLE
# define MUTEX_SETUP(x)		(x) = CreateMutex(NULL, FALSE, NULL)
# define MUTEX_CLEANUP(x)	CloseHandle(x)
# define MUTEX_LOCK(x)		WaitForSingleObject((x), INFINITE)
# define MUTEX_UNLOCK(x)	ReleaseMutex(x)
# define THREAD_ID		GetCurrentThreadId()
#elif defined(_POSIX_THREADS) || defined(_SC_THREADS)
# define MUTEX_TYPE		pthread_mutex_t
# define MUTEX_SETUP(x)		pthread_mutex_init(&(x), NULL)
# define MUTEX_CLEANUP(x)	pthread_mutex_destroy(&(x))
# define MUTEX_LOCK(x)		pthread_mutex_lock(&(x))
# define MUTEX_UNLOCK(x)	pthread_mutex_unlock(&(x))
# define THREAD_ID		pthread_self()
#else
# error "You must define mutex operations appropriate for your platform"
# error	"See OpenSSL /threads/th-lock.c on how to implement mutex on your platform"
#endif

struct CRYPTO_dynlock_value
{ MUTEX_TYPE mutex;
};

static MUTEX_TYPE *mutex_buf;

static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line)
{ struct CRYPTO_dynlock_value *value;
  value = (struct CRYPTO_dynlock_value*)malloc(sizeof(struct CRYPTO_dynlock_value));
  if (value)
    MUTEX_SETUP(value->mutex);
  return value;
}

static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l, const char *file, int line)
{ if (mode & CRYPTO_LOCK)
    MUTEX_LOCK(l->mutex);
  else
    MUTEX_UNLOCK(l->mutex);
}

static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *file, int line)
{ MUTEX_CLEANUP(l->mutex);
  free(l);
}

void locking_function(int mode, int n, const char *file, int line)
{ if (mode & CRYPTO_LOCK)
    MUTEX_LOCK(mutex_buf[n]);
  else
    MUTEX_UNLOCK(mutex_buf[n]);
}

unsigned long id_function()
{ return (unsigned long)THREAD_ID;
}

int CRYPTO_thread_setup()
{ int i;
  mutex_buf = (MUTEX_TYPE*)malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
  if (!mutex_buf)
    return SOAP_EOM;
  for (i = 0; i < CRYPTO_num_locks(); i++)
    MUTEX_SETUP(mutex_buf[i]);
  CRYPTO_set_id_callback(id_function);
  CRYPTO_set_locking_callback(locking_function);
  CRYPTO_set_dynlock_create_callback(dyn_create_function);
  CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
  CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);
  return SOAP_OK;
}

void CRYPTO_thread_cleanup()
{ int i;
  if (!mutex_buf)
    return;
  CRYPTO_set_id_callback(NULL);
  CRYPTO_set_locking_callback(NULL);
  CRYPTO_set_dynlock_create_callback(NULL);
  CRYPTO_set_dynlock_lock_callback(NULL);
  CRYPTO_set_dynlock_destroy_callback(NULL);
  for (i = 0; i < CRYPTO_num_locks(); i++)
    MUTEX_CLEANUP(mutex_buf[i]);
  free(mutex_buf);
  mutex_buf = NULL;
}

#endif
