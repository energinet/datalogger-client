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

#include "rpfileutil.h"

#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "md5.h"

#define NUMBER_OF_ITERATIONS (5)
#define MD5_SIGNATURE_SIZE (16)


int rpfile_md5(unsigned char * testBuffer, size_t testLen, char* obuf)
{
  unsigned char signature [MD5_SIGNATURE_SIZE]; 
  int i, ptr = 0; 

  MD5(testBuffer, testLen, signature);

  for (i=0; i < MD5_SIGNATURE_SIZE; i++) { 
    ptr += sprintf(obuf+ptr, "%02x", signature[i]); 
  } 

  ptr += sprintf(obuf+ptr, "\0"); 

  return ptr;

}

    

int rpfile_md5chk(unsigned char * testBuffer, size_t testLen, char* md5sum)
{
    char md5local[32];

    rpfile_md5(testBuffer, testLen, md5local);

    return strcmp(md5sum, md5local);
    

}


int rpfile_read(const char *path, char **buffer, size_t *size, mode_t *mode)
{
    FILE *file = NULL;
    char *data= NULL;
    size_t fsize = 0;
    struct stat statbuf;
    int retval  = 0;

    if(stat(path, &statbuf)==-1){
	fprintf(stderr, "could not stat file \"%s\": %s\n", path, strerror(errno));
	return -errno;
    }

    fsize = statbuf.st_size;
    fprintf(stderr, "fsize of \"%s\" is %d \n", path, fsize);

    file = fopen(path, "r");    
    if(!file){
        fprintf(stderr, "could not open file: \"%s\" %s\n", path, strerror(errno));
	return -errno;
    }  

    data = malloc(fsize);
    
    if(!data){
	fprintf(stderr, "could malloc buffer for file  \"%s\" size %d \n", path, fsize);
	fclose(file);
	return -ENOMEM;
    }

    
    if( (retval=fread(data, fsize,1, file)) != 1 ){
	fprintf(stderr, "Error reading file: \"%s\" %s (%d)\n", path, strerror(ferror(file)), retval);
	retval = -ferror(file);
	free(data);
    } else {
	*buffer = data;
	*size = fsize;
	*mode = statbuf.st_mode;
    }

    fclose(file);

    return retval;

}

int rpfile_write(const char *path, char *buffer, size_t size, mode_t mode)
{
    FILE *file;
    int retval;
    file = fopen(path, "w+");
    if(!file){
	fprintf(stderr, "Error creating file \"%s\": %s\n", path, strerror(errno));
	return -errno;
    }
    
    if(fwrite(buffer, size, 1,file)==-1){
	fprintf(stderr, "Error writing file \"%s\": %s %d\n", path, 
		strerror(ferror(file)), retval);
	fclose(file);
	return -ferror(file);
    }
		
    fclose(file);

    if(chmod(path, mode)==-1){
	fprintf(stderr, "Error setting file mode \"%s\": %s %d\n", path, 
		strerror(errno), retval);
	return -errno;
    } 
    
    
    return 0;
}
