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
#include "sdvpd_item.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#ifdef MEMWATCH
#include "memwatch.h"
#endif

struct sdvpd_item *sdvpd_item_Init( const char *name, enum sdvpd_item_type type)
{

    struct sdvpd_item *new = malloc(sizeof(*new));
    assert(new);

    new->type = type;
    assert(name);
    new->name = strdup(name);

    return new;
}
  
void sdvpd_item_Delete(struct sdvpd_item *item)
{
    if(!item)
	return;

    free(item->name);
    free(item);
}
