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

// Contents of file "rpfchart.h":
//gsoap ns service name: rpfchart
//gsoap ns service style: rpc
//gsoap ns service encoding: encoded
//gsoap ns service port: cgi-bin/rpfchart.cgi
//gsoap ns service namespace: urn:rpfchart
//gsoap ns service method-action: add ""

typedef char* xsd__string;
typedef int xsd__int;
typedef double xsd__double;

struct ns__event{
    @xsd__string value;
    @xsd__string time;
    @xsd__int eid;
};



struct ns__events{
    struct ns__event  *__ptr;
    int __size;
    int __offset;
};

struct ns__events_ret{
    char *name;
    char *begin;
    char *end;
    struct ns__events events;
    int timeCache;
    int timeQuery;
    int count;
    int eid;
    char *complete;
};


struct ns__etype{
    @xsd__string name;
    @xsd__string unit;
    @xsd__string description;  
    @xsd__int eid;
    @xsd__string type;
};

struct ns__etypes{
    struct ns__etype *__ptr;
    int __size;
    int __offset;
};

int ns__getETypes(char *box, struct ns__etypes *result);
int ns__getEvents(char *box, char *ename, char *begin, char *end, struct ns__events_ret *etypes);
