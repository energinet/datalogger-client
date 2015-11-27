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


#ifndef SDVP_MAP_H_
#define SDVP_MAP_H_


#include <stddef.h>


/**
 * Type item
 */
enum sdvp_map_level {
    /**
     * Server */
    SDVPMAP_LEV_SERVER = 0,
    /**
     * Logical Device */
    SDVPMAP_LEV_LDEVICE = 1,
    /**
     * Logical Node */
    SDVPMAP_LEV_LNODE = 2,
    /**
     * Data object */
    SDVPMAP_LEV_DOBJECT = 3,    
};


/**
 * Mapping item
 */
struct sdvp_map{
    /**
     * IEC 61850 name */
    char *name;
    /**
     * IEC 61850 type */
    enum sdvp_map_level level;
    /**
     * Mapped item */
    struct sdvp_item *item;
    /**
     * Childs, mappings containd in this map */
    struct sdvp_map *childs;
    /**
     * Parent map */
    struct sdvp_map *parent;
    /**
     * Next map in a list */
    struct sdvp_map *next;
};


/**
 * Create a map item
 * @param name The name of the mapped item 
 * @param type The type item
 * @param item The mapped item 
 * @return a mapping item
 */
struct sdvp_map *sdvp_map_Create(const char *name, struct sdvp_item *item);

/**
 * Delete a map item
 * @param map The map to delete
 * @note This finction delets all items in a tree
 */
void sdvp_map_Delete(struct sdvp_map *map);

/**
 * Add a map item to a list 
 * @param map The map to add to
 * @param new The new item in the list
 * @return the first item in the list 
 * @note a map set to NULL is an empty list
 */
struct sdvp_map *sdvp_map_AddNext(struct sdvp_map *map, struct sdvp_map *new);

/**
 * Add a child to a map item
 * @param  map The map to add to
 * @param new The new child
 * @return child point (new)
 */
struct sdvp_map *sdvp_map_AddChild(struct sdvp_map *map, struct sdvp_map *new);


/**
 * Get a map item from MMS reference 
 * @parma root The mapping root (The server map item)
 * @param mms The MMS reference 
 * @return the found item or NULL if not found 
 */
struct sdvp_map *sdvp_map_SearchRef(struct sdvp_map *root, const char *mms);

/**
 * Get the MMS reference name of a map item
 * @param The map item 
 * @param buf The buffer to put the string
 * @param max The maximum length of the buffer
 * @return The length of the returned string
 */
int sdvp_item_GetRef(struct sdvp_map *map, char *buf, size_t maxlen);


/**
 * Get next pointer in map tree
 * @param map The last pointer
 * @return the next pointer or NULL if end
 */
struct sdvp_map *sdvp_map_TreeNext(struct sdvp_map *map);


/**
 * Get the next map in a list 
 * @param map The last pointer
 * @return the next pointer or NULL if end
 */
struct sdvp_map *sdvp_map_ListNext(struct sdvp_map *map);

/**
 * Get the size of a list 
 * @param map The map list pointer 
 * @return the list size
 */
int sdvp_map_ListSize(struct sdvp_map *map);


/**
 * Seatch a map list for a named map
 * @param map The map list pointer 
 * @param name The name to search for 
 * @return the pointer or NULL if not found
 */
struct sdvp_map *sdvp_map_ListSearch(struct sdvp_map *map, const char *name );


#endif /* SDVP_MAP_H_ */


