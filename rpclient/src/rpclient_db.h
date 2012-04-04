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

#ifndef RPCLIENT_DB_H_
#define RPCLIENT_DB_H_
#include <logdb.h>
#include "soapH.h" // obtain the generated stub
#include "rpclient.h"

#define DEFAULT_DB_FILE "/jffs2/bigdb.sql"


int rp_boxinfo_set(struct rpclient *client_obj,struct boxInfo *boxinfo);
int cmd_run(struct rpclient *client_obj, struct soap *soap, char *address, time_t time);

char *platform_user_get(void);
int send_metadata(struct rpclient *client_obj, struct soap *soap,  char *address, struct logdb *db  );
int send_measurments(struct rpclient *client_obj,struct soap *soap,  char *address, struct logdb *db  );


int file_set(struct rpclient *client_obj, struct soap *soap, char *address,  
			 char *serverfile,char *filepath);

int file_get(struct rpclient *client_obj, struct soap *soap, char *address, char *filename, const char *destpath, const char *modestr);


#endif /* RPCLIENT_DB_H_ */
