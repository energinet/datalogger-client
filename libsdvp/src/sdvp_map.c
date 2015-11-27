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
#include "sdvp_map.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define SDVPMAP_MAX_DEPTH 10

/**
 * Create a map item
 * @param name The name of the mapped item 
 * @param type The type item
 * @param item The mapped item 
 * @return a mapping item
 */
struct sdvp_map *sdvp_map_Create(const char *name, struct sdvp_item *item)
{
    struct sdvp_map *new = malloc(sizeof(*new));
    assert(new);
    
    new->name = (name)?strdup(name):NULL;

    new->level = SDVPMAP_LEV_SERVER;  
    new->item = item;
    new->next = NULL;
    new->parent = NULL;
    new->childs = NULL;
    
    return new;
}

/*
 * Delete a map item
 */
void sdvp_map_Delete(struct sdvp_map *map)
{
    if(!map)
	return;

    sdvp_map_Delete(map->childs);
    sdvp_map_Delete(map->next);

    free(map->name);
    free(map);
}

/**
 * Add a map item to a list 
 * @param map The map to add to
 * @param new The new item in the list
 * @return the first item in the list 
 * @note a map set to NULL is an empty list
 */
struct sdvp_map *sdvp_map_AddNext(struct sdvp_map *map, struct sdvp_map *new)
{
    if(!map)
	return new;

    struct sdvp_map *ptr = map;

    while(ptr->next){
	ptr = ptr->next;
    }

    ptr->next = new;

    return map;
    
}

/*
 * Add a child to a map item
 */
struct sdvp_map *sdvp_map_AddChild(struct sdvp_map *map, struct sdvp_map *new)
{
    map->childs = sdvp_map_AddNext(map->childs, new);
    new->parent = map;
    new->level = map->level++;
    return new;
}

const char *sdbp_item_GetPreStr(int level)
{
    switch(level){
    case 0:
	return "";
    case 1:
	return "";
    case 2:
	return "/";
    case 3:
	return ".";
    default:
	return ".";
    }
}

char sdbp_item_GetPreChar(int level)
{
    const char *buf = sdbp_item_GetPreStr(level);

    return buf[0];
}

/*
 * Get a map item from MMS reference 
 */
struct sdvp_map *sdvp_map_SearchRef(struct sdvp_map *root, const char *mms)
{
    int level = 1;
    char name[64];
    struct sdvp_map *mptr = root;
    const char *sptr = mms;
    
    if(!mms)
	return root;

    if(strcmp(mms, "domain")==0||strcmp(mms, "/")==0||strlen(mms)<1)
	return root;
    
    mptr = root->childs;

    
    while(mptr && sptr){
	char divchr = sdbp_item_GetPreChar(++level);
	const char *snext = NULL;
	if(divchr != '\0')
	    snext = strchr(sptr, divchr);

	if(snext){
	    memcpy(name, sptr, snext-sptr);
	    name[snext-sptr] = '\0';
	    sptr = snext+1;
	    syslog(LOG_DEBUG, "name %s\n", name);
	    mptr =  sdvp_map_ListSearch(mptr, name );
	} else {
	    syslog(LOG_DEBUG, "name %s\n", sptr);
	    return  sdvp_map_ListSearch(mptr, sptr );
	}	

	if(mptr)
	    mptr = mptr->childs;
	
    }

    return NULL;
}

struct sdvp_map *sdvp_map_GetRoot(struct sdvp_map *map)
{
    struct sdvp_map *ptr = map;

    if(!map)
	return NULL;

    while(ptr->parent){
	ptr = ptr->parent;
    }
    
    return ptr;
}


/**
 * Get the MMS reference name of a map item
 * @param The map item 
 * @param buf The buffer to put the string
 * @param max The maximum length of the buffer
 * @return The length of the returned string
 */
int sdvp_item_GetRef(struct sdvp_map *map, char *buf, size_t maxlen)
{
    int depth = 0;
    int level = 0;
    int len = 0;
    struct sdvp_map *ptr = map;
    struct sdvp_map *path[SDVPMAP_MAX_DEPTH];
    

    while((ptr) && (depth < SDVPMAP_MAX_DEPTH)){
	path[depth] = ptr;
	depth++;
	ptr = ptr->parent;
    }

    while(depth){
	len += snprintf(buf+len, maxlen-len-1,
		       "%s%s", sdbp_item_GetPreStr(level++), 
		       path[depth-1]->name);
	depth--;
    }
    buf[len] = '\0';
    return len;

    
}

struct sdvp_map *sdvp_map_TreeNext(struct sdvp_map *map)
{
    if(!map)
	return NULL;

    if(map->childs)
	return map->childs;

    if(map->next)
	return map->next;

    while(map->parent){
	if(map->parent->next)
	    return map->parent->next;
	map = map->parent;
    }

    return NULL;    
}

int sdvp_map_TreeSize(struct sdvp_map *map)
{
    int count = 0;
    
    while(map){
	count++;
	map = sdvp_map_TreeNext(map);
    }

    return count;
}



struct sdvp_map *sdvp_map_ListNext(struct sdvp_map *map)
{
    if(!map)
	return NULL;

    return map->next;
}


int sdvp_map_ListSize(struct sdvp_map *map)
{
    int count = 0;
    
    while(map){
	count++;
	map = sdvp_map_ListNext(map);
    }

    return count;
}

struct sdvp_map *sdvp_map_ListSearch(struct sdvp_map *map, const char *name )
{
    syslog(LOG_INFO, "List search '%s' in %p\n", name, map);

    while(map){
		syslog(LOG_DEBUG, "List search '%s' == '%s' ??\n", name, map->name);
		if(strcmp(map->name, name )==0)
			return map;
		map = sdvp_map_ListNext(map);
    }

    return NULL;
}


