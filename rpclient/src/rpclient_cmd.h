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

#ifndef RPCLIENT_CMD_H_
#define RPCLIENT_CMD_H_
#include "soapH.h" // obtain the generated stub
#include "rpserver.h"
//#include "boxcmdq.h"
#include "rpclient_db.h"
#include <cmddb.h>


#define CMDFLA_KEEPCURR   1 /**< Keep current value in db */
#define CMDFLA_NODELAY    2 /**< The commans/value is delayable */
#define CMDFLA_SETTING    CMDFLA_KEEPCURR  /**< Keep current value in db */
#define CMDFLA_ACTION     0 /**< Keep current value in db */
#define CMDFLA_CONNECTED  4 /**< The operation is done connected */ 



#define RETURN_DO_REBOOT (1238547)

struct cmdhnd_data{
    struct rpclient *client_obj;
    struct soap *soap;
    char *address;
    int reboot;
};


/* struct cmd_handler{ */
/*     char *name; */
/*     int (*handler)( struct boxvcmd *cmd, struct cmdhnd_data *data, char** retval); */
/*     unsigned long flags; */
/*     void *data; */
/* }; */


int boxcmd_sent_status(struct rpclient *client_obj, struct soap *soap, char *address, int cid, int status, 
		       struct timeval *timestamp, const char* retstr, struct cmddb_cmd *dbcmd);

//int boxcmd_run(struct boxvcmd *cmd, struct cmdhnd_data *data, char** retval);

//struct cmd_handler *boxcmd_get_hndl(const char *name);
//unsigned long boxcmd_get_flags(const char *name);


void boxcmd_print(struct boxCmd *cmd);

int rpclient_cmd_init(const char *db_path, int debug_level);

int rpclient_cmd_deinit(void);

int rpclient_cmd_run(struct cmdhnd_data *data);


#endif /* RPCLIENT_CMD_H_ */
