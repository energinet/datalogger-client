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
#include "xml_cntn_sub.h"

#define LINKLST_ADD(list, new) \
	({typeof (list)	ptr;\
	if(new)\
	{ptr = new;while(ptr->next){ptr = ptr->next;} ptr->next = list;}	\
	new?new:list;													\
	})

#include <stdlib.h>


struct xmc_cntn_sub *xmc_cntn_sub_create(const void *publisher)
{
	struct xmc_cntn_sub *new = malloc(sizeof(*new));

	new->publisher = publisher;
	new->next = NULL;

	return new;
}

void xmc_cntn_sub_delete(struct xmc_cntn_sub* this)
{
	if(!this)
		return;
	
	xmc_cntn_sub_delete(this->next);
	free(this);
}

struct xmc_cntn_sub *xmc_cntn_sub_list_add(struct xmc_cntn_sub *list, struct xmc_cntn_sub *new)
{
	return LINKLST_ADD(list, new);
}


int xmc_cntn_sub_list_isin(struct xmc_cntn_sub *list, const void *publisher)
{
	struct xmc_cntn_sub *ptr = list;

	while(ptr){
		if(publisher == ptr->publisher)
			return 1;

		ptr = ptr->next;
	}

	return 0;
}

