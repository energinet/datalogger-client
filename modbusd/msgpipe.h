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

#ifndef _MSGPIPE_H
#define _MSGPIPE_H

#include <stdbool.h>

#define KEY_PATH "/etc/hosts"
#define PROJ_ID 43

struct sIPCMsg { 
  long type; 
  char text[100]; 
};

enum eRegTypes {
    MB_TYPE_UNKNOWN=-1, 
    MB_TYPE_COILS, 
    MB_TYPE_INPUT, 
    MB_TYPE_HOLDING, 
    MB_TYPE_SLAVEID
};


enum eValueTypes  {
    MV_TYPE_SHORT,
    MV_TYPE_USHORT,
    MV_TYPE_LONG,
    MV_TYPE_ULONG,

};

enum eActionTypes {
    MA_TYPE_READ,
    MA_TYPE_WRITE,
    MA_TYPE_WRITE_MULT
};


int create_ipc();
int connect_ipc();
int destroy_ipc(int mq_id);
bool check_msg_queue(int mq_id, char *message);
bool send_recv_cmd(int mq_id, struct sIPCMsg *msg, int timeout_ms);






void flush_msg_ipc(int mq_id);

int read_cmd(int mq_id, int addr, enum eRegTypes type, int reg_no,enum eValueTypes val_type, int *value, int dbglev);

int write_cmd(int mq_id,int addr, enum eRegTypes type, int reg_mo, int mult, int value, int dbglev);

#endif
