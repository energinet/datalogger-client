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



/**
 * @addtogroup module_xml
 * @{
 * @section sec_readfile_module_xml node read/write module 
 *  
 * @verbatim 
<module type="readfile" name="pt1000"  verbose="0" flags="nolog">
  <file path="/dev/adc00" name="temp1" text="Temperatur 1" unit="°C"
	interval="1" calc="eeprom:adc00_calc#poly1:a0.105b-273" multiread="10"
	max="100" min="-45" />
  <file path="/dev/adc01" name="temp2" text="Temperatur 2" unit="°C"
	interval="1" calc="eeprom:adc01_calc#poly1:a0.105b-273" multiread="10"
	max="100" min="-45" />
</module>
  @endverbatim
  * @}
*/



/**
 * @ingroup modules 
 * @{
 */

/**
 * @defgroup modules_readfile Module for reading /dev nodes
 * @{
 */


#define DEFAULT_EMODE EMODE_ALWAYS
#define DEFAULT_IMODE IMODE_ASCII
#define DEFAULT_INTERVAL (60)  /* sec */
#define DEFAULT_UNIT "l"

pthread_mutex_t rdfile_mutex = PTHREAD_MUTEX_INITIALIZER;

enum{
    EMODE_ALWAYS,
    EMODE_ONCHANG,
};

enum{
    IMODE_ASCII,
    IMODE_INT,
    IMODE_SHORT,
    IMODE_CHAR,
};

struct rdfile{
//    int fd;
    char *filepath;
    int interval;
    int emode;
    int imode;
    int last_value;
    struct module_calc *calc;
    int mread_cnt;
    float bound_max;
    float bound_min;
    float bound_zero;
    int fault_state;
    struct timeval rdtime;
    struct event_type *event_type;
    struct rdfile *next;
};

struct module_object{
    struct module_base base;
    struct rdfile *rdfiles;
    struct event_type *fault_event; 
    int write_enabled;
    int fault_state;
    struct module_tick *tick;
};


unsigned long timeval_diff(struct timeval *now, struct timeval *prev)
{
    int sec = now->tv_sec - prev->tv_sec;
    int usec = now->tv_usec - prev->tv_usec;

    unsigned long diff;

    if(prev->tv_sec == 0)
        return 0x7ffffff;

    if( sec < 0 ) {
        PRINT_ERR("does not support now < prev");
        sec = 0;
    }

    diff = sec*1000;
    diff += usec/1000;

    return diff;
    
        
}

struct rdfile *rdfile_create(const char *path, int interval, const char *emode, const char *imode)
{
    struct rdfile *new = NULL;
    int fd;
    char emodes[][32] = { "always", "change" , ""};
    char imodes[][32] = { "ascii", "int", "short", "char"};

    if(!path){
        PRINT_ERR("no path");
        return NULL;
    }
    
    fd = open(path, O_RDONLY);

    if(fd < 0){
        PRINT_ERR("error opening file %s: %s", path, strerror(errno));
        return NULL;
    }
    
    close(fd);

    new = malloc(sizeof(*new));

    if(!new){
        PRINT_ERR("malloc failed");
        return NULL;
    }

    memset(new, 0, sizeof(*new));
        
//    new->fd = fd;
    new->interval = interval;
    new->filepath = strdup(path);

    new->emode = DEFAULT_EMODE;
    if(emode){
        int i = 0;
        while(emodes[i][0] != '\0'){
            if(strcmp(emodes[i], emode) == 0){
                new->emode = i;
                break;
            }
        }

        if(emodes[i][0] == '\0'){
            PRINT_ERR("mode not supported");
            goto err_out;
        }
    }

    new->imode = DEFAULT_IMODE;
    
    if(imode){
        int i = 0;
        while(imodes[i][0] != '\0'){
            if(strcmp(imodes[i], imode) == 0){
                new->imode = i;
                break;
            }
        }

        if(imodes[i][0] == '\0'){
            PRINT_ERR("imode not supported");
            goto err_out;
        }
    }
    
    return new;

  err_out:
    
    //   close(new->fd);
    
    free(new);

    return NULL;

}

struct rdfile *rdfile_add(struct rdfile *first, struct rdfile *new)
{
    struct rdfile *ptr = first;
    
    if(!first)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return first;
}

void rdfile_delete(struct rdfile *rdfile)
{
    if(rdfile->next)
        rdfile_delete(rdfile->next);

    free(rdfile);

    
    
}

float rdfile_readfile(struct rdfile *rdfile);

struct module_event *rdfile_eread(struct event_type *event)
{
     struct rdfile *rdfile = (struct rdfile *)event->objdata;

     float value = rdfile_readfile(rdfile);
     return module_event_create(event, uni_data_create_float(value), NULL);
}

int rdfile_writefile(struct rdfile *rdfile, struct uni_data *data);

/**
 * Receive handler 
 * @note See @ref EVENT_HANDLER_PAR for paramaters
 */

int rdfile_handler_rcv(EVENT_HANDLER_PAR)
{
//    struct module_object *this = module_get_struct(handler->base);
    struct rdfile *rdfile = (struct rdfile *)handler->objdata;
    struct uni_data *data = event->data;
    
    rdfile_writefile(rdfile, data);

    return 0;
}



/**
 * Write event handler
 */
int rdfile_ewrite(struct event_type *event, struct uni_data *data)
{
    struct rdfile *rdfile = (struct rdfile *)event->objdata;
    
    return rdfile_writefile(rdfile, data);
}




void rdfile_send_m_event(struct module_object *module, struct event_type *event_type, struct timeval *time, unsigned long diff, float value)
{
    struct uni_data data;

    memset(&data, 0, sizeof(data));

    data.type = DATA_FLOAT;
    data.value = (float)value;
    data.mtime = diff;
    data.data = &value;

    //memcpy(&data.time, time, sizeof(data.time));
    module_base_send_event(module_event_create(event_type, &data, time));
//    module_base_send_event(&module->base, event_type, &data);
}

void exclude_max(int *values,  int *excludes, int size)
{
    int value = 0;
    int ptr = 0;
    int i;

    for(i = 0 ; i < size; i++){
        if(!excludes[i] && (values[i] > value)){
                value = values[i];
                ptr = i;
        }
    }
    excludes[ptr] = 1;
    
}

void exclude_min(int *values,  int *excludes, int size)
{
    int value = 0x7ffffff;
    int ptr = 0;
    int i;

    for(i = 0 ; i < size; i++){
        if(!excludes[i] && (values[i] < value)){
            value = values[i];
            ptr = i;
        }
    }
    excludes[ptr] = 1;
    
}


float calc_avg(int *values,  int *excludes, int size)
{
    float value = 0;
    int count = 0;
    int i = 0;
//    printf("calc_avg size %d\n", size);
    for(i = 0 ; i < size; i++){
//        printf("value %d %d\n", values[i], excludes[i]);
        if(!excludes[i]){
            value += values[i];
            count++;
        }
    }
//    printf("calc_avg ret cnt %f %d\n", value, count);
    if(count != 0)
        return value/(float)count;
    return value;

}

                
float calc_median(int *values, int size)
{
    int i;
    float value = 0;
    int *excludes = malloc(sizeof(int)*size);
    int excludeno = size/4;

//    printf("calc_median exno %d\n", excludeno);
    memset(excludes, 0, sizeof(int)*size);

    for(i = 0; i < excludeno; i++)
        exclude_max(values,  excludes, size);

    for(i = 0; i < excludeno; i++)
        exclude_min(values,  excludes, size);
    
    
    value =  calc_avg(values, excludes,size);

    free(excludes);
    
//    printf("calc_median ret %f\n", value);
    return value;

}

int rdfile_upd_status(struct rdfile *rdfile)
{
    int status = 0;

    if(rdfile->bound_max != rdfile->bound_min){
	if(rdfile->last_value > rdfile->bound_max)
	    status = 1;
	if(rdfile->last_value < rdfile->bound_min)
	    status = 1;
    }

    if(status != rdfile->fault_state){
		//printf( "rdfile state %d --> %d\n",rdfile->fault_state, status);
		rdfile->fault_state = status;
    }


    return status;
 
}


int rdfile_writefile(struct rdfile *rdfile, struct uni_data *data)
{
    const char *path = rdfile->filepath;
    FILE *file = NULL;
    char buffer[256];

    if(!path){
        PRINT_ERR("no device");
        return -EFAULT;
    }
    
    file = fopen(path, "w");

    if(!file){
        PRINT_ERR("error opening %s: %s", path, strerror(errno));
        return -errno;
    }
    
    uni_data_get_txt_value(data, buffer, sizeof(buffer));

//    fprintf(file,"%s", buffer);  
    
    fclose(file);
    
    return 0;
}


float rdfile_readfile(struct rdfile *rdfile)
{
    int value = 0;
    float fvalue = 0;
    float calc_value = 0;
    char buf[32];
    int retval = 0;
    int i;
    int *values = NULL;
    int fd = open(rdfile->filepath, O_RDONLY);

    if(fd < 0){
        PRINT_ERR("error opening file %s: %s", rdfile->filepath, strerror(errno));
        return -1;
    };

    values = malloc(sizeof(int)*rdfile->mread_cnt);

    for(i = 0; i < rdfile->mread_cnt; i++){
		
		memset(buf, 0, sizeof(buf));

        retval = read(fd, buf, sizeof(buf));   

        if(retval <= 0){
            PRINT_ERR("read error");
			calc_value = -1;
			goto out;
		}

        switch(rdfile->imode){
          default:
          case IMODE_ASCII:
            value = atoi(buf);
            break;
          case IMODE_INT:
            value =  buf[0]&0xff;
            value += (buf[1]&0xff)<<8;
			value += (buf[2]&0xff)<<16;
			value += (buf[3]&0xff)<<24;
            break;
          /* case IMODE_SHORT: */
          /*   value = (short)buf; */
          /*   break; */
          /* case IMODE_CHAR: */
          /*   value = (char)buf; */
          /*   break; */
        }

        values[i] = value;
    }
	
    fvalue = calc_median(values, rdfile->mread_cnt);
    value = (int) fvalue;
   
    calc_value = module_calc_calc(rdfile->calc, fvalue);
    //PRINT_DBG(1, "read: %f, calc_value: %f\n ", fvalue, calc_value);

  out:
	free(values);
	close(fd);
    return calc_value;

}


int rdfile_read(struct module_object *module, struct rdfile *rdfile, struct timeval *time)
{
    float calc_value = 0;

    calc_value = rdfile_readfile(rdfile);
//    PRINT_DBG(1, "read: %f, calc_value: %f\n ", fvalue, calc_value);
	memcpy(&rdfile->rdtime, time, sizeof(struct timeval));
    rdfile->last_value = calc_value;
    if(rdfile_upd_status(rdfile)==0)
	rdfile_send_m_event(module, rdfile->event_type, time, 1, calc_value);
    
    /* close(fd); */

    return 0;

}

struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}


//struct event_handler handlers[] = { {.name = "read", .function = handler_read} ,{.name = ""}};
struct event_handler handlers[] = { {.name = ""}};


int start_rdfile(XML_START_PAR)
{
    struct modules *modules = (struct modules*)data;
    struct module_object* this = module_get_struct(ele->parent->data);
        struct rdfile *rdfile = rdfile_create(get_attr_str_ptr(attr, "path"),
                  get_attr_int(attr, "interval", 60),
                  get_attr_str_ptr(attr, "event_mode"),
                  get_attr_str_ptr(attr, "input"));

    if(!rdfile)
        return -1;

    rdfile->event_type =  event_type_create_attr(&this->base,(void*)rdfile,attr);

    rdfile->event_type->read  = rdfile_eread;
    
    if(get_attr_int(attr, "write", this->write_enabled))
	rdfile->event_type->write = rdfile_ewrite;

    rdfile->calc = module_calc_create(get_attr_str_ptr(attr, "calc"),this->base.verbose);  

    rdfile->mread_cnt = get_attr_int(attr, "multiread", 1);
    
    if(!rdfile->event_type){
        PRINT_ERR("event_type is NULL");
        return -1;
    }    

    this->base.event_types = event_type_add(this->base.event_types, rdfile->event_type); // make it avaliable


    this->rdfiles = rdfile_add(this->rdfiles, rdfile); // Add rdfile
    
    
    
    rdfile->bound_max = get_attr_float(attr, "max", 0);
    rdfile->bound_min = get_attr_float(attr, "min", 0);
    rdfile->bound_zero = get_attr_float(attr, "bzero", 0);
    
    if( get_attr_str_ptr(attr, "event")){
	/* create event handler */
	struct event_handler *event_handler = event_handler_create(get_attr_str_ptr(attr, "name")
								   ,  rdfile_handler_rcv ,&this->base, (void*)rdfile);
	
	this->base.event_handlers = event_handler_list_add(this->base.event_handlers, event_handler);

	return module_base_subscribe_event(&this->base, modules->list, get_attr_str_ptr(attr, "event"), FLAG_ALL, 
				event_handler, attr);

    }


    PRINT_MVB(&this->base, "max %f, min %f, bzero %f %s\n",  rdfile->bound_max, 
	      rdfile->bound_min, rdfile->bound_zero, "test");

    return 0;
}

struct xml_tag module_tags[] = {                         \
    { "file" , "module" , start_rdfile, NULL, NULL},   \
    { "" , "" , NULL, NULL, NULL }                       \
};

int module_init(struct module_base *base, const char **attr)
{
    struct module_object* this = module_get_struct(base);
    
    this->fault_state = -1;
    base->flags |= FLAG_LOG;

    this->fault_event = event_type_create(base, NULL, "fault", "","Readfile Fault Event", DEFAULT_FFEVT );
    
    base->event_types = event_type_add(base->event_types, this->fault_event);

    this->write_enabled = get_attr_int(attr, "write", 0);

    this->tick = module_tick_create(base->tick_master,  base, get_attr_float(attr, "interval", 0), TICK_FLAG_SECALGN1);

    return 0;

}

void module_deinit(struct module_base *module)
{
    struct module_object* this = module_get_struct(module);

    rdfile_delete(this->rdfiles);

    module_tick_delete(this->tick);

    return;
}

int module_status(struct module_object* this, struct timeval *time)
{
     struct module_base *base = &this->base;
    
    int new_state = 0;
    struct rdfile *ptr = this->rdfiles;

    while(ptr){
	if(ptr->fault_state > new_state)
	    new_state = ptr->fault_state;
	ptr = ptr->next;
    }
    
    if(new_state != this->fault_state){
	struct uni_data data;
	PRINT_MVB(base, "fault state changed %d --> %d", this->fault_state ,new_state);
	this->fault_state = new_state;
	memset(&data, 0, sizeof(data)); 
	data.type = DATA_INT;
	data.value = new_state;
	data.mtime = 1; 
	//memcpy(&data.time, time, sizeof(data.time));
	module_base_send_event(module_event_create(this->fault_event, &data, time));
    }

    return 0;
    
}




void* module_loop(void *parm)
{
    struct module_object *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;
    time_t prev_time;

    base->run = 1;
    
    while(base->run){ 
        struct rdfile *rdfile = this->rdfiles;
        struct timeval tv;

		module_tick_wait(this->tick, &tv);
		
		if(pthread_mutex_lock(&rdfile_mutex)){
			PRINT_ERR("error locking mutex");
			return NULL; 
		}
	   		
		while(rdfile){
            if((tv.tv_sec%rdfile->interval)==0){
				usleep(5000); // To make the ADC regain voltage (tested value by CRA) 
				PRINT_MVB(base, "Read %s.%s ------------------", base->name, rdfile->event_type->name);
				rdfile_read(this, rdfile, &tv);
            }
            rdfile = rdfile->next;
        }
		pthread_mutex_unlock(&rdfile_mutex); 
		
        prev_time = tv.tv_sec;
		module_status(this, &tv);

    } 
    
    PRINT_MVB(base, "loop returned %ld",  prev_time );
    
    return NULL;

}

struct module_type module_type = {                  \
    .name       = "readfile",                       \
    .xml_tags   = module_tags,                      \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct module_object), \
};


struct module_type *module_get_type()
{
    return &module_type;
}




/**
 * @} 
 */

/**
 * @} 
 */
