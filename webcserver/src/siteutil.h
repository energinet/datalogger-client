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

#ifndef SITEUTIL_H_
#define SITEUTIL_H_
#include <qDecoder.h>

struct sitereq{
    Q_ENTRY *req;
    char *boxid;
    char *localid;
    char *subtitle;
    char *sname;
    int simpel;
    struct asocket_con* socket_rp;
};

struct sitemenu_item{
    char *name;
    char *link;
    char *sname;
};


int siteutil_init(struct sitereq *site, char *subtitle, char *sname);

void siteutil_deinit(struct sitereq *site);

void siteutil_top(struct sitereq *site,  int refresh);

void siteutil_bot(struct sitereq *site);


#endif /* SITEUTIL_H_ */
