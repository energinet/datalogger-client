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

#include "module_base.h"
#include "module_value.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/input.h>
#include <assert.h>
#include <termios.h>


#define DEFAULT_BAUD 115200
#define MODULE_NAME "wirlrec"


/**
 * @addtogroup module_xml
 * @{
 * @section wrmodxml Wireless receiver module
 * Module for interfacing the LIAB wireless receiver.
 * <b>Typename: "wirlrec" </b>\n
 * <b> Attributes: </b>
 * - <b> path:</b> Path to the usb tty device\n
 * - <b> baud:</b> Baud rate/communication speed (defailt: 115200)
 * <b> Tags: </b>
 * <ul>
 * <li><b>txunit:</b> Wireless transmitter
 * - <b> adr:</b> Unit adress
 * - <b> type:</b> Transmitter type: temp, temphum...
 * </ul>
 @verbatim 
 <module type="wirlrec" name="wireless" verbose="0">
     <receiver path="/dev/ttyUSB1">
     <transmitter address="
 </module>
@endverbatim
 @}
*/


const char *ttype_str[] = { "tempV1", "tempV1hum" , "unknown",  NULL};

#define SEQ_INITIAL (0xf0000000)

enum ttype{
    TTYPE_TEMPV1,
    TTYPE_TEMPV1HUM,
    TTYPE_UNKNOWN,
};

enum meastype{
    MEASTYPE_TEMP,
    MEASTYPE_HUMI,
	MEASTYPE_BATT,
	MEASTYPE_ENERGY,
	MEASTYPE_BUTTON,
	MEASTYPE_PKGLOST,
};

enum calstype{
	CALCTYPE_NONE,
	CALCTYPE_AVG,
	CALCTYPE_SUM,
};



struct msgmeta{
    int unitaddr;
    int destaddr;
    int seq;
    int len;
    int energy;
};


struct txitem{
    struct evalue *value;
	enum meastype type;
	enum calstype ctype;
	int interval;
	float cvalue;
	int count;
	struct timeval last;
    struct txitem *next;
};


struct txunit{
    enum ttype type;
    const char *name;
    int adr;
    int seq;
    struct txitem *items;
    struct txunit *next;
};

struct wrx_module{
    struct module_base base;
    char *path;
    int baud;
    int fd;
    struct txunit *units;
};



struct wrx_module* module_get_struct(struct module_base *base){
    return (struct wrx_module*) base;
}

/************************************/
/* Bit handeling                    */


struct txitem *txitem_create(struct module_base *base, enum meastype type, const char *name,  
							 const char *subname, const char *unit, const char *hname, unsigned long flags,
							 int interval,	enum calstype ctype)
{
    struct txitem *new =  malloc(sizeof(*new));
    assert(new);
    
    new->value = evalue_create_ext(base, name, subname, NULL , flags, unit, hname);

    char flagstr[128];
    PRINT_MVB(base, "created txitem '%s' at at for %s with flags '%s'", subname , name , 
	      event_type_get_flag_str(new->value->event_type->flags, flagstr) );

    base->event_types = event_type_add(base->event_types, new->value->event_type); // make it avaliable */

	new->type = type;
	new->interval = interval;
	new->ctype = ctype;
	new->cvalue = 0;
	new->count = 0;
	gettimeofday(&new->last, NULL);
    new->next = NULL;
    
    return new;

}

struct txitem *txitem_lst_add(struct txitem *list, struct txitem *new )
{
    struct txitem *ptr = list;
    
    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;
    
    return list;
}

struct txitem *txitem_lst_get_num(struct txitem *list, int index )
{
    struct txitem *ptr = list;
    int count = 0;
	
    while(ptr){
	if(count == index)
	    return ptr;
	count++;
	ptr = ptr->next;
    }

    return NULL;
}

struct txitem *txitem_lst_get_type(struct txitem *list, enum meastype type )
{
    struct txitem *ptr = list;
    int count = 0;
	
    while(ptr){
		if(ptr->type == type)
			return ptr;
		count++;
		ptr = ptr->next;
    }

    return NULL;
}


void txitem_set(struct txitem *item, float value, struct timeval *now ){
	
	if(item==NULL){
		PRINT_ERR("Error item==NULL");
		return;
	}

	if(item->interval==0){
		evalue_setft(item->value, value, now);
		return;
	}

	item->cvalue += value;
	item->count++;
	
} 

void txitem_calc(struct txitem *item, struct timeval *now)
{
	if(item->interval==0)
		return;

	if(item->last.tv_sec == now->tv_sec)
		return;
	
	if((now->tv_sec%item->interval) && ((item->last.tv_sec + item->interval) > now->tv_sec))
		return;
	
	float value = 0;

	fprintf(stderr, "last  %ld, now %ld %ld %ld\n",
			item->last.tv_sec,  now->tv_sec,
			(now->tv_sec%item->interval), 
			(item->last.tv_sec + item->interval/2));

	fprintf(stderr, "cvalue %f, count %d \n", item->cvalue, item->count);

	item->last.tv_sec =  now->tv_sec;	
	item->last.tv_usec =  now->tv_usec;	

	if(item->count==0)
		return;

	switch(item->ctype){
	case CALCTYPE_AVG:
		value = item->cvalue/item->count;
		break;
	case CALCTYPE_SUM:
		value = item->cvalue;
		break;
	case CALCTYPE_NONE:
	default:
		value = 0;
	}

	item->cvalue = 0;
	item->count = 0;

	evalue_setft(item->value, value, now);


}

void txitem_lst_set_type(struct txitem *list, enum meastype type, float value, struct timeval *now  )
{
	struct txitem *item = txitem_lst_get_type(list, type );
	
	txitem_set(item, value, now );
	
}


void txitem_delete(struct txitem *item)
{
    /* ToDo: Delete evalue */
    
    if(!item)
	txitem_delete(item->next);
    
    free(item);
}

int txunit_pkg_lost(struct txunit *unit,  short seq)
{
	int lost = 0;
	if(unit->seq !=  SEQ_INITIAL){
		short next = unit->seq +1;
		lost = seq-next;
		
	}
	unit->seq = seq; 	
	return lost;	

}



struct txunit *txunit_create(struct module_base *base, const char *el,
			     const char **attr )
{
    struct txunit* unit = malloc(sizeof(*unit));
    const char * name =  get_attr_str_ptr(attr, "name");
    const char * text =  get_attr_str_ptr(attr, "text");
    int statint =  get_attr_int(attr, "status_interval", 3600);

    struct txitem *item;
    unsigned long flags = event_type_get_flags(get_attr_str_ptr(attr, "flags"), base->flags);
    unit->items = NULL;
    unit->adr = get_attr_int(attr, "adr", 0xffff);
    
    unit->type = modutil_get_listitem(el ,TTYPE_UNKNOWN , ttype_str);

    switch(unit->type){
    case TTYPE_TEMPV1:
		item = txitem_create(base, MEASTYPE_TEMP, name, "temp", "°C", text, flags, 0, CALCTYPE_NONE);
		unit->items = txitem_lst_add(unit->items, item);
		item = txitem_create(base, MEASTYPE_BATT, name,  "batt",  "V", text,DEFAULT_FFEVT, statint, CALCTYPE_AVG);
		unit->items = txitem_lst_add(unit->items, item);
		break;
    case TTYPE_TEMPV1HUM:
		item = txitem_create(base, MEASTYPE_TEMP, name, "temp", "°C", text, flags, 0, CALCTYPE_NONE);
		unit->items = txitem_lst_add(unit->items, item);
		item = txitem_create(base, MEASTYPE_HUMI, name, "humi", "%RH", text, flags, 0, CALCTYPE_NONE);
		unit->items = txitem_lst_add(unit->items, item);
		item = txitem_create(base, MEASTYPE_BATT, name, "batt", "V", text, DEFAULT_FFEVT, statint, CALCTYPE_AVG);
		unit->items = txitem_lst_add(unit->items, item);
		break;
    default:
		return NULL;
    }
   
	item = txitem_create(base, MEASTYPE_ENERGY, name, "energy", "energy", text, DEFAULT_FFEVT, statint, CALCTYPE_AVG);
	unit->items = txitem_lst_add(unit->items, item);

	item = txitem_create(base, MEASTYPE_PKGLOST, name, "pkglost", "", text, DEFAULT_FFEVT, statint, CALCTYPE_SUM);
	unit->items = txitem_lst_add(unit->items, item);

   	unit->name = strdup(name);
	unit->seq = SEQ_INITIAL;

    unit->next = NULL;

    char flagstr[128];
    PRINT_MVB(base, "created txunit '%s' at at adr %x with flags '%s'", name , unit->adr, 
	      event_type_get_flag_str(flags, flagstr) );


    return unit;
}

struct txunit *txunit_lst_add(struct txunit *list, struct txunit *new )
{
    struct txunit *ptr = list;
    
    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;
    
    return list;
}



void txunit_delete(struct txunit *unit)
{
    /* ToDo: Delete evalue */
    
    if(!unit)
	txunit_delete(unit->next);
    
    free(unit);
}




void txunit_receive(struct module_base *base, struct txunit *unit, struct msgmeta *meta, const char* str)
{
    float value,hum;
    int batt1, batt2, batt3;
    str++;

	struct timeval now_;
	struct timeval *now = &now_;

	gettimeofday(now, NULL);
	    
    if(strncmp("1.2T",str,4) == 0) {
        if(sscanf(str, "1.2T%fH%fB%d", &value, &hum, &batt1 )!=3){
            PRINT_ERR("1Error reading temperature '%s'", str);
            return;
        }
		value /= 100;
		hum /= 100;

		txitem_lst_set_type(unit->items, MEASTYPE_TEMP, value, now );
		txitem_lst_set_type(unit->items, MEASTYPE_HUMI, hum, now );
		txitem_lst_set_type(unit->items, MEASTYPE_BATT, ((float)batt1)/100, now );

    } else {
	    if(sscanf(str, "%f", &value)!=1){
	        PRINT_ERR("Error reading temperature '%s'", str);
	        return;
	    }
	    
        value /= 100;

        PRINT_MVB(base, "Temperature 0x%x sendt %f", meta->unitaddr, value);

		txitem_lst_set_type(unit->items, MEASTYPE_TEMP, value, now );

        const char *battptr = strchr(str, ' ');

        if(!battptr)
	        return;
    
        battptr++;

        if(sscanf(battptr, "%d %d %d", &batt1, &batt2, &batt3)!=3){
	        PRINT_ERR("No batery in '%s'",  battptr);
	        return;
        }
		
		txitem_lst_set_type(unit->items, MEASTYPE_BATT, ((float)batt2)/100, now );

    }

	txitem_lst_set_type(unit->items, MEASTYPE_ENERGY, meta->energy, now );    

	int lost = txunit_pkg_lost(unit,  meta->seq);
	txitem_lst_set_type(unit->items, MEASTYPE_PKGLOST, lost, now );    
}



int xml_start_txunit(XML_START_PAR)
{
    struct module_base* base = ele->parent->data;
    struct wrx_module *this = module_get_struct(base);
    struct txunit *new = txunit_create(base,el,attr);	

    if(!new){
	return -1;
    }
    
    this->units = txunit_lst_add(this->units, new);
    

		
    return 0;
}


struct xml_tag module_tags[] = {
    { "tempV1" , "module" , xml_start_txunit, NULL, NULL},
    { "tempV1hum" , "module" , xml_start_txunit, NULL, NULL},
    { "" , ""  , NULL, NULL, NULL }
};
  
int module_wirlrec_tty_speed(int speed)
{
	switch(speed){
	  case 115200:
		return B115200;
	  case 38400:
		return B38400;
	  case 9600:
		return B9600;
	  default:
		return speed;
	}	
	
}

int module_wirlrec_tty_attribs(int fd, int speed_, int parity)
{
    struct termios tty;
	int speed = module_wirlrec_tty_speed(speed_);
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // ignore break signal
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                printf ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}


void module_wirlrec_tty_err(struct pollfd *poll_st, const char *fname)
{
    PRINT_ERR("TTY %s Error ret=0x%X", fname, poll_st->revents);
    if(poll_st->revents&POLLPRI)
	PRINT_ERR("TTY %s Error: POLLPRI", fname);
    if(poll_st->revents&POLLOUT)
	PRINT_ERR("TTY %s Error: POLLOUT", fname);
#ifdef POLLRDHUP
    if(poll_st->revents&POLLRDHUP)
	PRINT_ERR("TTY %s Error: POLLRDHUP", fname);
#endif
    if(poll_st->revents&POLLERR)
	PRINT_ERR("TTY %s Error: POLLERR", fname);
    if(poll_st->revents&POLLHUP)
	PRINT_ERR("TTY %s Error: POLLHUP", fname);
    if(poll_st->revents&POLLNVAL)
	PRINT_ERR("TTY %s Error: POLLNVAL", fname);
#ifdef POLLRDNORM
    if(poll_st->revents&POLLRDNORM)
	PRINT_ERR("TTY %s Error: POLLRDNORM", fname);
#endif
#ifdef POLLRDBAND
    if(poll_st->revents&POLLRDBAND)
	PRINT_ERR("TTY %s Error: POLLRDBAND", fname);
#endif
#ifdef POLLWRNORM
    if(poll_st->revents&POLLWRNORM)
	PRINT_ERR("TTY %s Error: POLLWRNORM", fname);
#endif
#ifdef POLLWRBAND
    if(poll_st->revents&POLLWRBAND)
	PRINT_ERR("TTY %s Error: POLLWRBAND", fname);
#endif
}



int module_wirlrec_tty_readline(int fd, char *buf, size_t maxlen, int timeout, int *run)
{
    int ret, len = 0;
    struct pollfd poll_st;
    poll_st.fd        = fd;
    poll_st.events    = POLLIN;
    
    do{
	ret = poll(&poll_st, 1, timeout);
	
	if(ret == 0){
	    return -ETIME;
	} else if(ret < 0){
	    PRINT_ERR("TTY RX Error in poll: %s (%d)", strerror(errno), errno);
	    return -errno;
	}
		
	if(poll_st.revents == POLLIN){
	    ret = read(fd, buf+len, 1);
	    if(ret > 0){
		len += ret;
	    }else if(ret == 0)
		return -ENETDOWN;
	    else
		return ret;	
	} else {
	    module_wirlrec_tty_err(&poll_st, "RX");
	    return -EFAULT;
	}
	
    }while((len < maxlen) && !(buf[len-1]=='\n') && (*run));

    if(buf[len-1]=='\n')
	len--;

    buf[len] = '\0';

    return len;
}



int module_wirlrec_msg_cnv(struct msgmeta *meta, const char *str){
    assert(meta);
    assert(str);
    int unitaddr;
    int destaddr;
    int seq;
    int len;
    int energy;

    if(sscanf(str, "Received %x->%x %d %d %d", &unitaddr, &destaddr, &seq, &len, &energy)!=5){
	PRINT_ERR("Error parsing message string: '%s'", str);
	return -1;
    }
	
    meta->unitaddr = unitaddr;
    meta->destaddr = destaddr;
    meta->seq = seq;
    meta->len = len;
    meta->energy  = energy;
    
    char *ptr = strchr(str, ':');
    if(ptr == NULL){
	PRINT_ERR("Error parsing message string at ':' : '%s'", str);
	return -1;
    }

    return ptr-str+1;
}


void* module_loop(void *parm)
{
    struct wrx_module *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;
    
    char buf[1024];
    
    base->run = 1;

    if(this->fd < 0)
	return NULL;
    
    while(base->run){
        if(this->fd < 0) {
            /* Try reopening */
            close(this->fd);
            this->fd = open(this->path, O_RDWR | O_NOCTTY | O_SYNC);
            if(this->fd < 0){
	            PRINT_ERR("error %d opening %s: %s", errno, this->path, strerror(errno));
	            continue;
	        }
        }
     
		struct txunit *unit = this->units;
		struct timeval now;
		gettimeofday(&now, NULL);
		while(unit){
			struct txitem *item = unit->items;
			while(item){
				txitem_calc(item, &now);
				item = item->next;
			}
			unit = unit->next;
		}
			

	memset(buf, 0 , sizeof(buf));
	int ret = module_wirlrec_tty_readline(this->fd, buf, sizeof(buf), 1000, &base->run);
	if(ret == -ETIME || ret <= 1) {
	    if(ret == -EFAULT)
	        this->fd = -1;
	    continue;
	}

	PRINT_MVB(base, "Received %d: '%s'", ret, buf);

	struct msgmeta meta;

	int len = module_wirlrec_msg_cnv(&meta, buf);
	if(len <= 0)
	    continue;

	struct txunit *ptr = this->units;
	while(ptr){
	    PRINT_MVB(base, "Test address 0x%x == 0x%x", meta.unitaddr, ptr->adr);
	    if(meta.unitaddr == ptr->adr){
		txunit_receive(base, ptr, &meta, buf+len);
		break;
	    }
	    ptr = ptr->next;
	}

	if(!ptr)	PRINT_MVB(base, "Discarted address 0x%x", meta.unitaddr);
	
    }

    return NULL;

}


int module_init(struct module_base *base, const char **attr)
{
    int retval = 0;
    struct  wrx_module *this = module_get_struct(base);
    const char *path = get_attr_str_ptr(attr, "path");

    if(!path){
	PRINT_ERR("Path must be defined in %s of type "MODULE_NAME, 
		  get_attr_str_ptr(attr, "name"));
	return -1;
    }
	
    this->path = strdup(path);
    this->baud =  get_attr_int(attr, "baud", DEFAULT_BAUD);
    this->units = NULL;

    this->fd = open (this->path, O_RDWR | O_NOCTTY | O_SYNC);
	
    if(this->fd < 0){
	PRINT_ERR("error %d opening %s: %s", errno, this->path, strerror(errno));
	return 0;
	//return -errno;
    }    

    retval = module_wirlrec_tty_attribs(this->fd, this->baud, 0 /* parity */);

    if(retval){
	PRINT_ERR("error %d opening %s: %s", errno, this->path, strerror(errno));
	return -errno;
    }

    return 0;

}

void module_deinit(struct module_base *module)
{
    struct wrx_module* this = module_get_struct(module);

    close(this->fd);
    free(this->path);

    return;
}


struct module_type module_type = {    
    .name       = MODULE_NAME,
    .xml_tags   = module_tags,                      
    .type_struct_size = sizeof(struct wrx_module), 
};


struct module_type *module_get_type()
{
    return &module_type;
}

