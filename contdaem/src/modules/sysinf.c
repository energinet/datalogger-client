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
#include <assert.h>
#include <sys/statvfs.h> 


/**
 * @addtogroup module_xml
 * @{
 * @section sysinfmodxml System intomation module
 * Module for monitoring system performace.
 * <b>Typename: "ssyinf" </b>\n
 * <b> Tags: </b>
 * <ul>
 * <li><b>info:</b> The output device path \n  
 *   Information about the system memory usage.
 * - <b> type:</b>   The type of reading:\n
 *    -- @p memfree : the amount of free memory\n
 *    -- @p vmused : the amount of virtual memory that is allocated\n
 *    -- @p p_mrss : the amount of memory that is used by an application \n
 *    -- @p p_mvsz : the amount of virtual memory that is allocated by an application 
 * - <b> app:</b>   The name of the application. Used in the types: p_mrss and p_mvsz\n
 * - <b> pidfile:</b>   The path to the pidtile of the application. Used in the types: p_mrss and p_mvsz\n
 * - <b> interval:</b>  The interval to log in seconds\n
 * <li><b>filesize:</b> Read the size of a file \n  
 * - <b> path:</b> The path of the file\n
 * - <b> interval:</b>  The interval to log in seconds\n
 * <li><b>partfree:</b> Read the amount of free space on a pattition \n  
 * - <b> path:</b>  The path of the partition\n
 * - <b> interval:</b>  The interval to log in seconds\n
 * </ul>
 * Example from a setup:
 @verbatim 
 <module type="sysinf" name="system" flags="hide">
    <info type="memfree" interval="3600"/>
    <info type="vmused" interval="3600"/>
    <info type="p_mrss" app="contdaem" pidfile="/var/run/contdaem.pid" 
          interval="3600"/>
    <info type="p_mvsz" app="contdaem" pidfile="/var/run/contdaem.pid" 
          interval="3600"/>
    <info type="p_mrss" app="licon" pidfile="/var/run/licon.pid" 
          interval="3600"/>
    <info type="p_mvsz" app="licon" pidfile="/var/run/licon.pid" 
          interval="3600"/>
    <filesize name="bigdb" path="/jffs2/bigdb.sql" 
          interval="3600"/>
    <partfree name="root" path="/" interval="3600"/>
    <partfree name="jffs2" path="/jffs2" interval="3600"/>
  </module>
@endverbatim
 @}
*/



/**
 * @ingroup modules 
 * @{
 */

/**
 * @defgroup modules_sysinf Module for system information
 * @{
 */


#define DEFAULT_EMODE EMODE_ALWAYS
#define DEFAULT_IMODE IMODE_ASCII
#define DEFAULT_INTERVAL (60)  /* sec */
#define DEFAULT_UNIT "l"


enum{
    EMODE_ALWAYS,
    EMODE_ONCHANG,
};

enum{
    SYSI_MEM_FREE,
    SYSI_MEM_VMUSED,
    SYSI_PROG_MRSS,
    SYSI_PROG_MVSZ,
    SYSI_PARTI_FREE,
    SYSI_FILE_SIZE,
};

enum{
    SYSSTA_INIT=-1,
    SYSSTA_OK=0,
    SYSSTA_THRH,
    SYSSTA_FAULT,
};

struct sys_info{
    int type;
    int interval;
    char *name;
    char *path;
    int thrh;
    int pid;
    int state;
    struct event_type *event_type;
    struct sys_info *next;
};

struct module_object{
    struct module_base base;
    int fault_state;
    struct event_type *state_event;
    struct sys_info *sysitems;
};

int licon_pidfile_pid(const char *pidfile)
{
    FILE *file;
    int pid;

    if(!pidfile)
        return -1;

    file = fopen(pidfile, "r");

    if(!file)
        return -errno;

    if(fscanf(file, "%d", &pid)!=1){
        pid =  -EFAULT;
    }
    
    fclose(file);

    return pid;

}


int sys_info_parti_free(const char *path)
{
    struct statvfs buf;
    if(statvfs(path, &buf))
        return -1;

//    printf("block size %d, free blocks %d\n", buf.f_bsize, buf.f_bfree);

//    printf("returning %d\n", (buf.f_bsize*buf.f_bfree)/1024);
    
    return (buf.f_bsize*buf.f_bfree)/1024;
}


int sys_info_file_size(const char *path)
{
    struct stat buf;
    
    if(stat(path, &buf))
        return -1;
      
    //printf("file size %d\n", buf.st_size);

    return  buf.st_size/1024;
}



int sys_info_read_file(const char *path, const char *search)
{
    
    int retval = -1;
    char *retptr = NULL;
    FILE *file;
	int len = strlen(search);
    char buf[1024];

    if(!path)
        return -1;

    file = fopen(path, "r");

    if(!file)
        return -errno;
//    printf("search \"%s\"\n", search);
    while(!feof(file)){
        retptr = fgets(buf, sizeof(buf), file);
        //      printf("retptr \"%s\"\n", retptr);

        if(!retptr){
            PRINT_ERR("!retptr");
            break;
        }
        if(strncmp(buf, search, len) == 0){
            while(buf[len] == ' ')
                len++;
            //  printf("value \"%s\"\n", buf+len);
            sscanf(buf+len, "%d", &retval);
            break;
        }
    }

//    printf("returning %d\n", retval);

    fclose(file);

    return retval;
}



struct sys_info *sys_info_create(const char *typestr, const char *name, const char *path, int interval)
{
    struct sys_info *new = NULL;
    int type = -1;
    int i;
    char sitypes[][32] = { "memfree" , "vmused","p_mrss", "p_mvsz","parti_free","file_size", ""};


    if(!typestr){
        PRINT_ERR("no sys item type");
        return NULL;
    }


    i = 0;
    while(sitypes[i][0] != '\0'){
        if(strcmp(sitypes[i], typestr) == 0){
            type = i;
            break;
        }
        i++;
    }

    if(type < 0){
        PRINT_ERR("sys info type unknown");
        return NULL;
    }
    
    new = malloc(sizeof(*new));

    assert(new);

    memset(new, 0, sizeof(*new));

    new->type  = type;
    new->interval = interval;

    switch(type){
      case SYSI_MEM_FREE:
      case SYSI_MEM_VMUSED:
        break;
      case SYSI_PROG_MRSS:
      case SYSI_PROG_MVSZ:
        new->pid = licon_pidfile_pid(path);
        if(new->pid < 0){
            new->pid = -1;
            PRINT_ERR("error reading pidfile"); 
/*              goto err_out; */
        }
        /* do continue */
      case SYSI_PARTI_FREE:
      case SYSI_FILE_SIZE:
        if(!name){
            PRINT_ERR("no app specified");
            goto err_out;
        }
        if(!path){
            PRINT_ERR("no pidfile specified");
            goto err_out;
        }
        
        new->name = strdup(name);
        new->path = strdup(path);//pidfile
       
        break;
    }

    return new;

  err_out:
    
    //   close(new->fd);
    free(new->name);
    free(new->path);
    free(new);

    return NULL;

}

void sys_info_delete(struct sys_info *sysinf)
{
    if(sysinf->next)
        sys_info_delete(sysinf->next);

    free(sysinf->name);
    free(sysinf->path);
    free(sysinf);

}


struct sys_info *sys_info_add(struct sys_info *first, struct sys_info *new)
{
    struct sys_info *ptr = first;
    
    if(!first)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return first;
}


void sys_info_send_m_event(struct module_object *module, struct event_type *event_type, struct timeval *time, unsigned long diff, float value){
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


int sys_info_thrh(struct sys_info *sysinfo, int value)
{
    if(sysinfo->thrh==0)
	return 0;

    switch(sysinfo->type){
      case SYSI_MEM_FREE:
      case SYSI_PARTI_FREE:
	if(value < sysinfo->thrh)
	    return 1;
	break;
      case SYSI_PROG_MVSZ:
      case SYSI_MEM_VMUSED:
      case SYSI_PROG_MRSS:
      case SYSI_FILE_SIZE:
        if(value > sysinfo->thrh)
	    return 1;
        break;
      default:
        break;
    }

    return 0;
    
}


int sys_info_read(struct module_object *module, struct sys_info *sysinfo, struct timeval *time, int do_log)
{
    int retval;
    float value;
	char file[64];

    sysinfo->state = SYSSTA_OK;

    switch(sysinfo->type){
      case SYSI_MEM_FREE:
        retval = sys_info_read_file("/proc/meminfo", "MemFree:");  
	break;
      case SYSI_MEM_VMUSED:
        retval = sys_info_read_file("/proc/meminfo", "VmallocUsed:");
        break;
      case SYSI_PROG_MRSS:
        sysinfo->pid = licon_pidfile_pid(sysinfo->path);
        if(sysinfo->pid<0){
            retval = 0;
	    sysinfo->state = SYSSTA_FAULT;
        } else {
            sprintf(file, "/proc/%d/status", sysinfo->pid);
            retval = sys_info_read_file(file, "VmRSS:");
        }
        break;
      case SYSI_PROG_MVSZ:
        sysinfo->pid = licon_pidfile_pid(sysinfo->path);
        if(sysinfo->pid<0){
            retval = 0;
	    sysinfo->state = SYSSTA_FAULT;
        } else {
            sprintf(file, "/proc/%d/status", sysinfo->pid);
            retval = sys_info_read_file(file, "VmSize:");
        }
        break;
      case SYSI_PARTI_FREE:
         retval = sys_info_parti_free(sysinfo->path);
        break;
      case SYSI_FILE_SIZE:
        retval = sys_info_file_size(sysinfo->path);
        break;
      default:
	retval = -EFAULT;
	sysinfo->state = SYSSTA_FAULT;
        break;
	        
    }
    

    if(retval < 0){
        PRINT_ERR( "Read failed for %d '%s'", sysinfo->type, sysinfo->name );
	sysinfo->state = SYSSTA_FAULT;
        return -1;
    }

    if(sysinfo->state ==   SYSSTA_OK){
	if(sys_info_thrh(sysinfo, retval)){
	    sysinfo->state = SYSSTA_THRH;
	}
    }

    value = retval/((float)1024);

    PRINT_MVB(&module->base, "Value read %f", value);
    if(do_log)
	sys_info_send_m_event(module, sysinfo->event_type, time, 1, value);
    
    return 0;

}


struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}


//int event_handler_all(struct module_event *event, void* userdata)
int event_handler_all(EVENT_HANDLER_PAR)
{
    struct module_object* this = module_get_struct(handler->base);
    printf("Module type %s received event\n", this->base.name);
    return 0;
}

struct event_handler handlers[] = { {.name = "all", .function = event_handler_all} ,{.name = ""}};




int start_sysinfo(XML_START_PAR)
{
    char name[32];
    char unit[32];
    char text[64];
    const char *text_alt;
    struct module_object* this = module_get_struct(ele->parent->data);
    struct sys_info *new = sys_info_create(get_attr_str_ptr(attr, "type"), 
                                           get_attr_str_ptr(attr, "app"), 
                                           get_attr_str_ptr(attr, "pidfile"), 
                                           get_attr_int(attr, "interval" , 600 ));

    if(!new)
        return -1;

    switch(new->type){
      case SYSI_MEM_FREE:
        strcpy(name,"memfree");
        strcpy(unit,"MB");
        strcpy(text,"Free Memory");
        break;
      case SYSI_MEM_VMUSED:
        strcpy(name,"vmused");
        strcpy(unit,"MB");
        strcpy(text,"VM Used");
        break;
      case SYSI_PROG_MRSS:
        sprintf(name, "%s_mrss", new->name);
        strcpy(unit,"MB");
        sprintf(text, "Memory RSS (%s)", new->name);
        break;
      case SYSI_PROG_MVSZ:
        sprintf(name, "%s_mvsz", new->name);
        strcpy(unit,"MB");
        sprintf(text, "VM Size (%s)", new->name);
        break;
      case SYSI_PARTI_FREE:
        sprintf(name, "%s_free", new->name);
        strcpy(unit,"MB");
        sprintf(text, "Free Space (%s)", new->name);
        break;
      case SYSI_FILE_SIZE:
        sprintf(name, "%s_size", new->name);
        strcpy(unit,"MB");
        sprintf(text, "File Size (%s)", new->name);
        break;
      default:
        break;
        return -1;
    }

    text_alt = get_attr_str_ptr(attr, "text");

    if(text_alt)
        strcpy(text,text_alt);
    
    new->event_type = event_type_create(&this->base, NULL, name, unit,text,this->base.flags );

//event_type_create( name, unit,text);
    
    if(!new->event_type){
        PRINT_ERR("event_type is NULL");
        return -1;
    }    

    this->base.event_types = event_type_add(this->base.event_types, new->event_type); // make it avaliable


    this->sysitems = sys_info_add(this->sysitems, new); // Add rdfile
    
    PRINT_MVB(&this->base, "sysinf added: %s %d", name ,new->pid );

    return 0;
}


int start_partfree(XML_START_PAR)
{
    char name[32];
    char unit[32];
    char text[64];
    const char *text_alt;
    struct module_object* this = module_get_struct(ele->parent->data);
    struct sys_info *new = sys_info_create("parti_free", 
                                           get_attr_str_ptr(attr, "name"), 
                                           get_attr_str_ptr(attr, "path"), 
                                           get_attr_int(attr, "interval" , 600 ));

    if(!new)
        return -1;

    sprintf(name, "%s_free", new->name);
    strcpy(unit,"MB");
    sprintf(text, "Free Space (%s)", new->name);
    
    text_alt = get_attr_str_ptr(attr, "text");

    if(text_alt)
        strcpy(text,text_alt);
    
    new->event_type = event_type_create(&this->base, NULL, name, unit,text,this->base.flags );
//event_type_create( name, unit,text);
    
    if(!new->event_type){
        PRINT_ERR("event_type is NULL");
        return -1;
    }    

    this->base.event_types = event_type_add(this->base.event_types, new->event_type); // make it avaliable


    this->sysitems = sys_info_add(this->sysitems, new); // Add rdfile
    
    PRINT_MVB(&this->base, "sysinf added: %s %d", name ,new->pid );

    return 0;
}

int start_filesize(XML_START_PAR)
{
    char name[32];
    char unit[32];
    char text[64];
    const char *text_alt;
    struct module_object* this = module_get_struct(ele->parent->data);
    struct sys_info *new = sys_info_create("file_size", 
                                           get_attr_str_ptr(attr, "name"), 
                                           get_attr_str_ptr(attr, "path"), 
                                           get_attr_int(attr, "interval" , 600 ));

    if(!new)
        return -1;

    sprintf(name, "%s_size", new->name);
    strcpy(unit,"MB");
    sprintf(text, "File Size (%s)", new->name);
    
    text_alt = get_attr_str_ptr(attr, "text");

    if(text_alt)
        strcpy(text,text_alt);
    
    new->event_type = event_type_create(&this->base, NULL, name, unit,text,this->base.flags );
    
    if(!new->event_type){
        PRINT_ERR("event_type is NULL");
        return -1;
    }    

    this->base.event_types = event_type_add(this->base.event_types, new->event_type); // make it avaliable


    this->sysitems = sys_info_add(this->sysitems, new); // Add rdfile
    
    PRINT_MVB(&this->base, "sysinf added: %s %d", name ,new->pid );

    return 0;
}




struct xml_tag module_tags[] = {                         \
    { "info" , "module" , start_sysinfo, NULL, NULL},   \
    { "filesize" , "module" , start_filesize, NULL, NULL},  \
    { "partfree" , "module" , start_partfree, NULL, NULL},  \
    { "" , "" , NULL, NULL, NULL }                       \
};
    

int sys_info_state_upd(struct module_object *module)
{
    struct sys_info *list = module->sysitems;

    int new_state = SYSSTA_OK;
    while(list){

	if(list->state > new_state)
	    new_state = list->state;
	list = list->next;
    }

    if(module->fault_state != new_state){
	struct uni_data data;
	struct timeval time;

	PRINT_MVB(&module->base, "Fault state changed %d --> %d", module->fault_state,  new_state);
	
	gettimeofday(&time, NULL);

	memset(&data, 0, sizeof(data));

	data.type = DATA_INT;
	data.value = new_state ;
	data.mtime = 0;
	
//	memcpy(&data.time, &time, sizeof(data.time));
	

	module_base_send_event(module_event_create(module->state_event, &data, &time));
	module->fault_state = new_state;
    }

    return new_state;

}


int module_init(struct module_base *module, const char **attr)
{
    struct module_object* this = module_get_struct(module);

    this->fault_state = SYSSTA_INIT;

    this->state_event = event_type_create(module, NULL, "state", "","Application State", DEFAULT_FFEVT );

//event_type_create("state", "", "Application State"); // create 

    if(! this->state_event){
        PRINT_ERR("event is NULL");
        return -1;
    }    

    module->event_types = event_type_add(module->event_types,this->state_event); // make it avaliable

    return 0;
}


void module_deinit(struct module_base *module)
{
    struct module_object *this = module_get_struct(module);
    sys_info_delete(this->sysitems);

}

void* module_loop(void *parm)
{
    struct module_object *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;
//	time_t prev_time;

    base->run = 1;
    
    while(base->run){ 
        struct sys_info *sysinf = this->sysitems;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        while(sysinf){
	    sys_info_read(this, sysinf, &tv,(tv.tv_sec%sysinf->interval)==0);
            sysinf = sysinf->next;
        }
	sys_info_state_upd(this);
    //    prev_time = tv.tv_sec;
        sleep(1);
    } 
    
    PRINT_MVB(base, "loop returned");
    
    return NULL;

}

struct module_type module_type = {                  \
    .name       = "sysinf",                       \
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
