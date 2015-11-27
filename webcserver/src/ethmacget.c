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

#include "ethmacget.h"


#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


char *platform_conf_get(const char *name)
{
    FILE *file;
    char sstring[128];
    char value[128];
    char line[256];
    int linemax = 100;
    sprintf(sstring, "%s=%%s\\n", name);

    file = fopen("/tmp/optionsfile", "r");
    
    if(file == NULL) {
        fprintf(stderr,"%s(): %d: Error opening file '%s'\n", __FUNCTION__, __LINE__, "/tmp/optionsfile");
        return NULL;
    }

    strcpy(value, "N/A");
    
    while(!feof(file)){

        if(!fgets(line, 256, file))
            break;

        if(sscanf(line, sstring, value)==1)
            break;      
        
        
        if(linemax<=0)
            break;

        linemax--;
    }

    fclose(file);

    return strdup(value);
}


char *ethmac_get(void)
{
    int mac[8];
    int retval;
    char *hwa = platform_conf_get("liabETH");
    char user[32];

    if(hwa == NULL) {
        sprintf(user, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", 0, 0, 0, 0, 0, 0);
        return strdup(user);
    }

    if((retval = sscanf(hwa, "%x:%x:%x:%x:%x:%x", 
                        &mac[0], &mac[1], &mac[2], 
                        &mac[3], &mac[4], &mac[5]))==6){
        free(hwa);
        sprintf(user, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return strdup(user);
    } 

    free(hwa);

    fprintf(stderr, "error reading mac: sscanf returned %d\n", retval);

    return NULL;
}

