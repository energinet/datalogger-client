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


#ifndef SDVP_ITEM_H_
#define SDVP_ITEM_H_


/**
 * Mapped item
 */
struct sdvp_item{
    /**
     * Userdata used for reference */
    const void *userdata;
};

/**
 * Create a mapped item 
 * @param userdata 
 * @return mapped item 
 */
struct sdvp_item * sdvp_item_Create(const void *userdata);

/**
 * Delete a mappd item
 * @param item The item to delete 
 * @note This function will not free userdata
 */
void  sdvp_item_Delete(struct sdvp_item *item);

#endif /* SDVP_ITEM_H_ */
