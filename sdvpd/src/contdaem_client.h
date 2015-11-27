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

#ifndef CONTDAEM_CLIENT_H_
#define CONTDAEM_CLIENT_H_

#include <xml_cntn_client.h>
#include <sys/time.h>

#define DEFAULT_CONTDAEM_PORT 6527


struct etype{
    char *ename;
    char *hname;
    char *unit;
    int writeable;
    int readable;
    struct etype *next;
};


int ContdaemClient_Init(struct xml_cntn_client** client_ret);

void ContdaemClient_Deinit(struct xml_cntn_client* client);

int ContdaemClient_Get(struct xml_cntn_client* client, const char *etype, char *out,  size_t maxlen, struct timeval *tv);

int ContdaemClient_Set(struct xml_cntn_client* client,const char *etype, const char *value,  char *out, size_t maxlen);

struct etype *ContdaemClient_List(struct xml_cntn_client* client, const char *filter);



struct etype *ContdaemClient_etype_Create(const char *ename, const char *hname, const char *unit, int readable, int writeable);



void ContdaemClient_etype_Delete(struct etype *etype);

int ContdaemClient_etype_toString(struct etype *etype,  char *out, size_t maxlen);

struct etype *ContdaemClient_etype_lst_Add(struct etype *lst, struct etype *new);



int ContdaemClient_etype_lst_Length(struct etype *lst);





#endif /* CONTDAEM_CLIENT_H_ */
