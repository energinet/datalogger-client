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
 
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <pthread.h>

#include "msgpipe.h"

char regTypes[][16] = {\
    "coils",	       \
    "input",	       \
    "holding",	       \
    "slaveid"	       \
};


char actTypes[][32] = { \
    "read",	       \
    "write",	       \
    "writemultiple"    \
};
    
pthread_mutex_t mutex;

int create_ipc()
{
  key_t ipckey;
  int mq_id;

  pthread_mutex_init(&mutex, NULL);
  
  /* Generate the ipc key */
  ipckey = ftok(KEY_PATH, PROJ_ID);
  if(ipckey < 0)
    printf("error creating key: %s\n", strerror(errno));
  printf("My key is %d\n", ipckey);
  
  /* Set up the message queue */
  mq_id = msgget(ipckey, IPC_CREAT | 0666);
  if (mq_id < 0) {
    printf("error getting identifier: %s\n", strerror(errno));
  }
  printf("Message identifier is %d\n", mq_id);

  return mq_id;
}

int connect_ipc()
{
  key_t ipckey;
  int mq_id;

  /* Generate the ipc key */
  ipckey = ftok(KEY_PATH, PROJ_ID);
  printf("My key is %d\n", ipckey);
  
  /* Set up the message queue */
  mq_id = msgget(ipckey, IPC_CREAT | 0666);
  printf("Message identifier is %d\n", mq_id);
  
  return mq_id;
}

int destroy_ipc(int mq_id)
{  
    pthread_mutex_destroy(&mutex);

  return msgctl(mq_id, IPC_RMID, NULL);
}

bool check_msg_queue(int mq_id, char *message)
{
  struct sIPCMsg mymsg;
  int received;

  if((received = msgrcv(mq_id, &mymsg, sizeof(mymsg), 1, IPC_NOWAIT)) > 0) {
    printf("Got: %s\n", mymsg.text);


    mymsg.type = 2;
    msgsnd(mq_id, &mymsg, sizeof(mymsg), IPC_NOWAIT);
  }

  return true;
}

bool send_recv_cmd(int mq_id, struct sIPCMsg *msg, int timeout_ms)
{
  int received;
  struct sIPCMsg _msg;
  int timeout = 0;
  bool retval = false;

  if(pthread_mutex_lock(&mutex)!=0)
      return false;
  
  //  memset(msg->text, 0, sizeof(msg->text));
  msg->type = 1;

  while(msgsnd(mq_id, msg, sizeof(msg->text),  IPC_NOWAIT) != 0 && timeout < timeout_ms) {
    timeout++;
    usleep(1000);
  }

  if(timeout >= timeout_ms) {
    printf("MODBUS: Timeout queueing message\n");
    goto out;
  }
  
  memset(_msg.text, 0, sizeof(_msg.text));
  timeout = 0;
  while((received = msgrcv(mq_id, &_msg, sizeof(_msg.text), 2, IPC_NOWAIT)) < 0 && timeout < timeout_ms) {
    timeout++;
    usleep(1000);
  }

  if(timeout_ms <= 0) {
    printf("MODBUS: Timeout waiting for reply!\n");
    goto out;
  }
  
  memcpy(msg, &_msg, sizeof(_msg));    
  retval = true;

  out:
  pthread_mutex_unlock(&mutex);

  return retval;
}

int get_no_msg_ipc(int mq_id)
{
  struct msqid_ds ds;

  if(msgctl(mq_id, IPC_STAT, &ds) == 0) {
    printf("Queue status:\n");
    printf("Messages in queue: %d\n", (int)ds.msg_qnum);
    return (int)ds.msg_qnum;
  }

  return -1;
}

void flush_msg_ipc(int mq_id)
{
  int no;
  int i;
  struct sIPCMsg mymsg;
  no = get_no_msg_ipc(mq_id);

  if(no > 0) {
    printf("Flushing message queue %d\n", no);
    for(i=0; i<no; i++) {
      if(msgrcv(mq_id, &mymsg, sizeof(mymsg), 1, IPC_NOWAIT) > 0) {
        //printf("Flushed %d in %d\n", i, 1);
      }
    }

    for(i=0; i<no; i++) {
      if(msgrcv(mq_id, &mymsg, sizeof(mymsg), 2, IPC_NOWAIT) > 0) {
        //printf("Flushed %d in %d\n", i, 2);
      }
    }
  }
}





int prep_msg(char *ptr, int max, int addr, enum eRegTypes type, enum eActionTypes action, int reg, int mult, int value, enum eValueTypes val_type)
{
    int len = 0;

    len += snprintf(ptr+len, max-len, "ADDR:0x%2.2x", addr);

    len += snprintf(ptr+len, max-len, "|REG:%d", reg);

    len += snprintf(ptr+len, max-len, "|TYPE:%s", regTypes[type]);
    

    len += snprintf(ptr+len, max-len, "|ACTION:%s", actTypes[action] );

    if(action == MA_TYPE_READ){
	int cnt = 1;
	if(val_type == MV_TYPE_LONG || val_type == MV_TYPE_ULONG)
	    cnt = 2;
	len += snprintf(ptr+len, max-len, ":%d", cnt );
    }else
	len += snprintf(ptr+len, max-len, ":0x%4.4x", value);
    
    if(action == MA_TYPE_WRITE_MULT)
	len += snprintf(ptr+len, max-len, "|NOREG:%i",  mult);


    return len;
    
}


int read_result(char *ptr, enum eValueTypes val_type, int *valuep)
{
    char* result;
    int value = 0;
    
    result = strstr(ptr, "RESULT:");

    if(!result){
	return 0;
    }

    result += strlen("RESULT:");
    
    sscanf(result, "%d", &value);

    if(value&0x8000&&(val_type==MV_TYPE_SHORT)){
	value = value-0x10000;
    }

    *valuep = value;
	
    return 1;

    
 
}


int read_cmd(int mq_id, int addr, enum eRegTypes type, int reg_no,enum eValueTypes val_type, int *value, int dbglev)
{
    struct sIPCMsg msg;
    int retval;
    int timeout = 500;

    prep_msg(msg.text ,sizeof(msg.text) ,addr ,type ,MA_TYPE_READ, reg_no , 2, 0, val_type);

    if(dbglev)
	fprintf(stderr, "Sending to modbusd: %s, type %d\n", msg.text, type);

    retval = send_recv_cmd(mq_id, &msg, timeout);
    
    if(dbglev)
	fprintf(stderr, "Received from modbusd: %s (retval %d)\n", msg.text, retval);        

    if(strstr(msg.text, "ERROR")) {
	if(dbglev)
	    fprintf(stderr, "%s\n", msg.text);        
	return -1;
    }

    
    return read_result(msg.text, val_type, value);

}


int write_cmd(int mq_id,  int addr, enum eRegTypes type, int reg_no, int mult, int value, int dbglev)
{
    struct sIPCMsg msg;
    int retval;
    int timeout = 500;

    int action = MA_TYPE_WRITE;

    if(mult > 1)
	action = MA_TYPE_WRITE_MULT;
       
    prep_msg(msg.text ,sizeof(msg.text) ,addr ,type ,action, reg_no , mult , value, MV_TYPE_SHORT);

    if(dbglev)
	fprintf(stderr, "Sending to modbusd: %s type %d\n", msg.text, type);

    retval = send_recv_cmd(mq_id, &msg, timeout);

    if(dbglev)
	fprintf(stderr, "Received from modbusd: %s (retval %d)\n", msg.text, retval);  

    if(strstr(msg.text, "ERROR")) {
	if(dbglev)
	    fprintf(stderr, "%s\n", msg.text);        
	return -1;
    }
    
    
    
    return 0;
   
}
