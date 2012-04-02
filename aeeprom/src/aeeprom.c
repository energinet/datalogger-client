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

#include "aeeprom.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s: "str"\n",__FUNCTION__, ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}


FILE *aeeprom_open(const char *path, long offset, const char *mode)
{
    FILE *file;
    file = fopen(path, mode);
    if (file == NULL){
	PRINT_ERR("Could not open file \"%s\": %s",path,strerror(errno));
	return NULL;
    }

    if(fseek(file, offset, SEEK_SET)==-1){
	PRINT_ERR("Could seek file \"%s\": %s",path,strerror(errno));
	fclose(file);
	return NULL;
    }

    return file;
}



char* aeeprom_get_entry(const char *name)
{
    const char *path =  DEFAULT_EE_FILE;
    long offset = DEFAULT_OFFSET;
    FILE *file;
    char buf[1024] ;
    char *retptr = NULL;

    file = aeeprom_open(DEFAULT_EE_FILE, DEFAULT_OFFSET, "r");
    if(!file){
	return NULL;
    }
    
    while (fgets(buf, sizeof(buf), file)) {
	char *val = strchr(buf,'=');

	if(!val)
	    break;

	val[0]='\0';
	
	if(strcmp(name, buf)==0){
	    retptr = strdup(val+1);
	    retptr[strlen(retptr)-1]='\0';
	    break;
	}
	
    }

  out:
    fclose(file);
    
    return retptr;
    
}

int aeeprom_set_entry(const char *name, const char *value)
{
    const char *path =  DEFAULT_EE_FILE;
    long offset = DEFAULT_OFFSET;    
    FILE *file;
    struct stat file_stat;   /*!< The file stat */
    char *obuf = NULL;
    char ibuf[1024] ;
    int len = 0;
    int size = 0;
    int found = 0;
    int retval;

    file = aeeprom_open(DEFAULT_EE_FILE, DEFAULT_OFFSET, "r+");
    if(!file){
	return -EFAULT;
    }

    if(stat(path, &file_stat)==-1){
	PRINT_ERR("Could stat file \"%s\": %s",path,strerror(errno));
	return -EFAULT;
    }
    
    size = file_stat.st_size-offset;
    obuf =  malloc(size);

    while (fgets(ibuf, sizeof(ibuf), file)) {
	char *val = strchr(ibuf,'=');
	char *vname = ibuf;

	if(!val)
	    break;
	val[0]='\0';

	if(strcmp(name, vname)==0){
	    if(value)//Remov if value is NULL
		len += snprintf(obuf+len,size-len,"%s=%s\n", vname, value);
	    found =1;
	} else {
	    len += snprintf(obuf+len,size-len,"%s=%s", vname, val+1);
	}
    }  

    if(!found){
	len += snprintf(obuf+len,size-len,"%s=%s\n", name, value);
    } 

    len += snprintf(obuf+len,size-len,"\n");

    if(fseek(file, offset, SEEK_SET)==-1){
	PRINT_ERR("Could seek file \"%s\": %s",path,strerror(errno));
	fclose(file);
	retval = -errno;
	goto out;
    }

    if(fwrite(obuf,1, len,  file)!=len){
	PRINT_ERR("Could write file \"%s\": %s",path,strerror(ferror(file)));
	retval = -ferror(file);
	goto out;
    }

    fflush(file); 

  out:
    
    fclose(file);
    free(obuf);
    return retval;
}


char* aeeprom_dump(void)
{
    const char *path =  DEFAULT_EE_FILE;
    long offset = DEFAULT_OFFSET;    
    FILE *file;
    struct stat file_stat;   /*!< The file stat */
    char *obuf = NULL;
    char ibuf[1024] ;
    int len = 0;
    int size = 0;
    int found = 0;
    
    file = aeeprom_open(DEFAULT_EE_FILE, DEFAULT_OFFSET, "r");
    if(!file){
	return -EFAULT;
    }

    if(stat(path, &file_stat)==-1){
	PRINT_ERR("Could stat file \"%s\": %s",path,strerror(errno));
	return -EFAULT;
    }
    
    size = file_stat.st_size-offset;
    obuf =  malloc(size);

    while (fgets(ibuf, sizeof(ibuf), file)) {
	char *val = strchr(ibuf,'=');
	
	if(!val)
	    break;

	len += snprintf(obuf+len,size-len,"%s", ibuf);

    }  

  out:
    
    fclose(file);

    return obuf;

}
