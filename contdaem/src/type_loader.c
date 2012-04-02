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

#include "type_loader.h"
#include <dirent.h> 
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>   /*dlopen*/



/**
 * @ingroup module_type_load Module type loader
 * @{
 */


int module_type_load_file(const char *path, struct module_type **types){

    void *handle;
    struct module_type * (*type_loader)();
    char *error;
    struct module_type *new;

    printf("Loading types from %s\n", path);

    /* load libformatters */
    handle = dlopen (path, /*RTLD_NOW |RTLD_GLOBAL | */RTLD_LAZY);
    
    if(!handle){
        printf("handle == NULL dlerror %s\n", dlerror() );
        return 0;
    }

    type_loader = dlsym(handle, "module_get_type");
    if ((error = dlerror()) != NULL)  {
        printf("%s not found \n","module_get_type" );
        return 0;
    }
    
    new = (*type_loader)();

    if(!new)
        return -1;

    new->dlopen_handel = handle;
    new->subscribe = dlsym(handle, "module_subscribe");
    new->init      = dlsym(handle, "module_init");
    new->start     = dlsym(handle, "module_start");
    new->loop      = dlsym(handle, "module_loop");
    new->deinit    = dlsym(handle, "module_deinit");

    *types = module_type_list_add(*types, new);

    return 0;
}

void module_type_unload_file(struct module_type *type)
{
    char name[32];
    
    strncpy(name, type->name, sizeof(name));

    printf("dlopen returned %d for type %s\n", dlclose( type->dlopen_handel), name);

   
}


int module_type_load_all(const char *path, struct module_type **types){

    DIR           *d;
    struct dirent *dir;
    d = opendir(path);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            int len = strlen(dir->d_name);
            if(len > strlen(FILE_SUFF)){
                if(strcmp(dir->d_name + (len - strlen(FILE_SUFF)), FILE_SUFF)==0){
                    char fpath[512];
                    sprintf(fpath,"%s/%s", path, dir->d_name);
                    if(module_type_load_file(fpath, types)<0)
                        return -1;
                    
                }
            }

        }
        
        closedir(d);
    }
    
    return(0);
    
}

/**
 *@} 
 */
