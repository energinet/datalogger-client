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

#ifndef XMLCONTDAEM_H_
#define XMLCONTDAEM_H_

#include <xml_serialize.h>
#include <module_base.h>

int module_util_xml_to_time(struct timeval *time, const char *str);

struct xml_item *uni_data_xml_item(struct uni_data *data, struct xml_doc *doc, struct convunit *flunit);

struct uni_data *uni_data_create_from_xmli(struct xml_item *item);

struct xml_item *module_event_xml_upd(struct  module_event* event, struct xml_doc *doc);

struct xml_item *event_type_xml(struct event_type *etype, struct xml_doc *doc);




#endif /* XMLCONTDAEM_H_ */
