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
#include "xml_cntn_trx.h"


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <linux/unistd.h>

#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s[%ld]: "str"\n",__FUNCTION__,(long int)syscall(__NR_gettid) , ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s[%ld]: "str"\n",__FUNCTION__,(long int)syscall(__NR_gettid), ## arg);}}

void spxml_cntn_trx_poll_err(struct pollfd *poll_st, const char *fname);


/**
 * Send document from file 
 */
int spxml_cntn_trx_tx(struct xml_doc *doc, int fd)
{
    char buf[1024*100];
    int retval = 0;
    int len;

    len = xml_doc_print(doc, buf, sizeof(buf));

    len += snprintf(buf+len, sizeof(buf)-len, "\r\n");

//	fprintf(stderr, "buf: %s", buf);

    retval = write(fd, buf, len);

    if(len != retval){
		PRINT_ERR("len (%d) != retval (%d)", len, retval);
		return -1;
    }   

    return 0;
    
}




#define POLL_TIMEOUT 100


ssize_t spxml_cntn_trx_read(int fd, void *buf, size_t count, int *run, int timeout, int dbglev)
{
    struct pollfd poll_st;
    int retval = 0;
	int timeused = 0;
	memset(&poll_st, 0, sizeof(poll_st));
    poll_st.fd = fd;
#ifdef POLLRDHUP
    poll_st.events = POLLIN|POLLRDHUP|POLLPRI;
#else
	poll_st.events = POLLIN|POLLPRI;
#endif

	

	while (retval==0 && *run==1){
		//dbglev = 3;
		PRINT_DBG(dbglev >= 8, "polling (dbglev %x)\n",dbglev);

		retval = poll(&poll_st, 1, POLL_TIMEOUT);
		PRINT_DBG(dbglev >= 8, "poll %d\n", retval);

		if(*run!=1)
			return -1;
		if(retval == 0){
			timeused += POLL_TIMEOUT;
			if((timeused > timeout) && (timeout > 0)) 
				return -1;
			PRINT_DBG(dbglev >= 8, "poll_st.revents %x, errno %d, timeout %d, time used, %d",
				  poll_st.revents, errno, timeout , timeused);
			continue;
		}else if(retval<0){
			/* Bug: Per har opdaget at den kan skrive denne fejl ud, selv om det hele køre... */
			PRINT_ERR("poll returned error: %s\n",strerror(errno));
			return -1;
		} else if (*run!=1) {
			if(dbglev){
				PRINT_ERR("poll returned because run = %d\n",*run);
				spxml_cntn_trx_poll_err(&poll_st, "run == 0: Socket rx");
			}
			return 0;
#ifdef POLLRDHUP
		} else if (poll_st.revents & POLLRDHUP){
			return -1;
#endif
		} else if (poll_st.revents & POLLHUP){
		    return -1;
		} else if (poll_st.revents != POLLIN){
			PRINT_ERR("poll_st.revents != POLLIN");
			spxml_cntn_trx_poll_err(&poll_st, "!= POLLIN");
							
			return -1;
		}
    } 
	
	if(*run!=1)
		return -1;
	retval = read(fd, buf, count);
	
    if(retval == 0){
		PRINT_DBG(dbglev >= 1,"EOF");
    } else if(retval < 0){
		PRINT_ERR("socket read error %s\n",strerror(errno));
    } else {
		if(dbglev >= 3){
			char *tmp = (char *)buf;
			tmp[retval] = '\0';
			PRINT_DBG(dbglev >= 3, "(%d) '%s'\n", retval, tmp);
		}
    }
	
    return retval;
}

/**
 * Read document from file 
 */
int spxml_cntn_trx_read_doc(int fd, struct xml_stack *rx_stack,int *run,int timeout, int dbglev)
{
	int retval = 0;
	char rx_buffer[1024];
	memset(rx_buffer, 0, sizeof(rx_buffer));

	char *newline = NULL;
	int trnf_size = 0, trnf_size_parse = 0, ptr = 0;
    int rd_ptr = 0, wrd_ptr = 0, size = 0;
	
	while(retval >= 0){
			
		if(rd_ptr == wrd_ptr){
			size = spxml_cntn_trx_read(fd, rx_buffer+wrd_ptr, /*sizeof(rx_buffer)-wrd_ptr*/1, run,timeout,  dbglev);
			PRINT_DBG(dbglev >= 2, "spxml_cntn_trx_read returned  %d\n", size);
		
			if(size <= 0){
				
				retval = -3;
				break;
			}
			

				wrd_ptr += size;
		}
		
		
		rx_buffer[wrd_ptr] = '\0';
		
		newline = strchr(rx_buffer+rd_ptr, '\r');
		if(newline){
			newline[0] = '\0';
			trnf_size = newline-(rx_buffer+rd_ptr)+1;
			trnf_size_parse = trnf_size-1;
			if(newline[1]=='\n'){
				trnf_size++;
			}
			
		} else {
			trnf_size = wrd_ptr-rd_ptr;
			trnf_size_parse = trnf_size;
		}
		ptr = rd_ptr;
		
		PRINT_DBG(dbglev >= 2, "Sending string to parser len %d >'%s'<\n", trnf_size_parse, rx_buffer+rd_ptr);
		retval = xml_stack_parse_step(rx_stack, rx_buffer+rd_ptr, trnf_size_parse);

		if(retval <0){
			rd_ptr += trnf_size;
			if(rd_ptr == wrd_ptr){
				rd_ptr = 0;
				wrd_ptr = 0;
			}
			break;
		}
		rd_ptr = 0;
		wrd_ptr = 0;
	}
	
	if((retval < -1) && (retval != -3))
	  PRINT_ERR("ERR >--inbuf(%d) len %zd >'%s'<\n", retval, strlen(rx_buffer+ptr), rx_buffer+ptr);
		
	return retval;
	

}


void spxml_cntn_trx_poll_err(struct pollfd *poll_st, const char *fname)
{
	int found = 0;
	PRINT_ERR( "%s Error ret=0x%X", fname, poll_st->revents);
	if(poll_st->revents&POLLPRI){
		PRINT_ERR("%s Error: POLLPRI", fname);
		found++;
	}

	if(poll_st->revents&POLLOUT){
		PRINT_ERR("%s Error: POLLOUT", fname);
		found++;
	}
#ifdef POLLRDHUP
	if(poll_st->revents&POLLRDHUP){
		PRINT_ERR("%s Error: POLLRDHUP", fname);
		found++;
	}
#endif
	if(poll_st->revents&POLLERR){
		PRINT_ERR("%s Error: POLLERR", fname);
		found++;
	}
	if(poll_st->revents&POLLHUP){
		PRINT_ERR("%s Error: POLLHUP", fname);
		found++;
	}
	if(poll_st->revents&POLLNVAL){
		PRINT_ERR("%s Error: POLLNVAL", fname);
		found++;
	}
#ifdef POLLRDNORM
	if(poll_st->revents&POLLRDNORM){
		PRINT_ERR("%s Error: POLLRDNORM", fname);
		found++;
	}
#endif
#ifdef POLLRDBAND
	if(poll_st->revents&POLLRDBAND){
		PRINT_ERR("%s Error: POLLRDBAND", fname);
		found++;
	}
#endif
#ifdef POLLWRNORM
	if(poll_st->revents&POLLWRNORM){
		PRINT_ERR("%s Error: POLLWRNORM", fname);
		found++;
	}
#endif
#ifdef POLLWRBAND
	if(poll_st->revents&POLLWRBAND){
		PRINT_ERR("%s Error: POLLWRBAND", fname);
		found++;
	}
#endif

	if(!found){
		PRINT_ERR("%s Error: Unknown---", fname);
	}
}
