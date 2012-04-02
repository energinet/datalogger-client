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

#define  _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#include "msgpipe.h"

#define NO_RUN 100000


ulong time_diff(struct timeval *start, struct timeval *end)
{
  ulong res;

  if (end->tv_usec < start->tv_usec) {
    int nsec = (start->tv_usec - end->tv_usec) / 1000000 + 1;
    start->tv_usec -= 1000000 * nsec;
    start->tv_sec += nsec;
  }
  if (end->tv_usec - start->tv_usec > 1000000) {
    int nsec = (end->tv_usec - start->tv_usec) / 1000000;
    start->tv_usec += 1000000 * nsec;
    start->tv_sec -= nsec;
  }

  res = (end->tv_sec - start->tv_sec)*1000000 + (end->tv_usec - start->tv_usec);
  
  return res;
}

int main(int argc, char *argv[])
{
  int mq_id;
  struct sIPCMsg msg;
  int on=1;
  mq_id = connect_ipc();
  int i, reg;

  int err = 0;
  struct timeval start, end;

  flush_msg_ipc(mq_id);

#ifdef undef
  printf("Clicking relays %d times\n", NO_RUN);
  for(i=0; i< NO_RUN; i++) {
    if(on == 1) {
      snprintf(msg.text, sizeof(msg.text), 
               "ADDR:0x10|REG:1000|TYPE:coils|ACTION:writemultiple:0xff|NOREG:8");
      on = 0;
    } else {
      snprintf(msg.text, sizeof(msg.text), 
               "ADDR:0x10|REG:1000|TYPE:coils|ACTION:writemultiple:0x00|NOREG:8");
      on = 1;
    }

    printf("Frame: %s\n",msg.text );
    send_recv_cmd(mq_id, &msg, 500);

    if(strstr(msg.text, "ERROR")) {
      exit(0);
    }


    usleep(10000);
  }
#endif
  
  i=0;

  for(;;) {


    for(reg=248; reg<=248; reg++) {
      snprintf(msg.text, sizeof(msg.text), 
               "ADDR:0x10|REG:%d|TYPE:input|ACTION:read:1", reg);
      
      if(!send_recv_cmd(mq_id, &msg, 100)) {
        printf("TIMEOUT\n");
      }

      printf("Telegram / Error   %d / %d  : ", i, err);      
      if(strstr(msg.text, "ERROR")) {
        err++;
        printf("Error!\n");
      } else {
        printf("%s\n", msg.text);
      }

      
      //      usleep(1000);
      i++;
    }
    printf("\n");
    fflush(stdout);
  }

  return EXIT_SUCCESS;
}
