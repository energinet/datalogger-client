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
 
#ifndef SEQUENCE_IO_H_
#define SEQUENCE_IO_H_

#include "module_value.h" 

struct seq_io {
    struct evalue *evalue;
    char *name;
    struct seq_io *next;
};



struct seq_io *seq_io_create_input( struct module_base *base, const char *event);


struct seq_io *seq_io_create_attr( struct module_base *base, const char **attr);


struct seq_io *seq_io_lst_add(struct seq_io *list, struct seq_io *new);


void seq_io_delete(struct seq_io *io);


const char *seq_io_get_name(struct seq_io *io);


struct seq_io *seq_io_get_by_name(struct seq_io *list, const char *name);


void seq_io_setf(struct seq_io *io, float value);

int seq_io_wait(struct seq_io *io, float value);

#endif /* SEQUENCE_IO_H_ */
