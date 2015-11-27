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

#ifndef ASOCKET_H_
#define ASOCKET_H_

#include <sys/un.h>
#include <arpa/inet.h>
#include <pthread.h>

int asocket_addr_len(struct sockaddr *skaddr);

#define ASOCKET_FUNC_PARAM struct asocket_con* sk_con, struct asocket_cmd *cmd, struct skcmd* rx_msg



struct skparam{
    char *param;
    struct skparam *next;
};

struct skcmd{
    char *cmd;
    struct skparam *param;
};


struct asocket_cmd;


struct asocket_con{
    void *appdata;
    int dbglev;
    int run;
    int fd;
    struct asocket_cmd *cmds;
    struct sockaddr *skaddr;
    pthread_mutex_t tx_mutex;
};

struct asocket_cmd{
    const char *cmd;
    int (*function) (ASOCKET_FUNC_PARAM);
    const char *help;
};


/* struct asocket_con* asocket_con_create(int skfd, struct asocket_cmd *cmds , void* appdata , int dbglev); */

/* void asocket_con_remove(struct asocket_con* sk_con); */


struct skcmd* asocket_cmd_create(const char *cmd);
void asocket_cmd_init(struct skcmd* skcmd, const char *cmd);

int asocket_cmd_param_count(struct skcmd* cmd);


int asocket_cmd_param_block_add(struct skcmd* cmd, char *param);
int asocket_cmd_param_add(struct skcmd* cmd, const char *param);
void asocket_param_delete(struct skparam *param);

char *asocket_cmd_param_get(struct skcmd* cmd, int num);

void asocket_cmd_delete(struct skcmd* cmd);

int asocket_cmd_write(struct skcmd* cmd, char* buf, int maxlen);
int asocket_cmd_read(struct skcmd** cmd_ret, char *buf, int len, char *errbuf);

struct sockaddr *asocket_addr_create_un(const char *path);

struct sockaddr *asocket_addr_create_in(const char *ip, int port);

void asocket_addr_delete(struct sockaddr *addr);



#endif /* ASOCKET_H_ */


