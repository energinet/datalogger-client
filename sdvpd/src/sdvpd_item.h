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
#ifndef SDVPD_ITEM_H_
#define SDVPD_ITEM_H_

enum sdvpd_item_type {
    CT_DIRECT,
    CT_CMD,
    CT_CMD_CANCEL,
};

struct sdvpd_item {
    enum sdvpd_item_type type;
    char *name;
};

struct sdvpd_item *sdvpd_item_Init( const char *name, enum sdvpd_item_type type);
  
void sdvpd_item_Delete(struct sdvpd_item *item);

#endif /* SDVPD_ITEM_H_ */
