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
 
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include <netdb.h>

#include "asocket.h"

void asocket_cmd_init(struct skcmd* skcmd, const char *cmd)
{
    assert(skcmd);

    memset(skcmd, 0 , sizeof(*skcmd));

    if(cmd)
        skcmd->cmd = strdup(cmd);

    return;
}

struct skcmd* asocket_cmd_create(const char *cmd)
{
    struct skcmd* new = NULL;

    new = malloc(sizeof(*new));
    
    if(!new){
        fprintf(stderr, "malloc failed in cmd create\n");
        return NULL;
    }

    asocket_cmd_init(new, cmd);

    return new;
    
}


void asocket_cmd_delete(struct skcmd* cmd)
{

    if(!cmd)
        return;

    asocket_param_delete(cmd->param);

    free(cmd->cmd);

    free(cmd);
}




int asocket_cmd_param_count(struct skcmd* cmd)
{
    assert(cmd);
    struct skparam* ptr = cmd->param;
    int cnt = 0;
     
    while(ptr){
        ptr = ptr->next;
        cnt++;
    }

    return cnt;
}

int asocket_cmd_param_block_add(struct skcmd* cmd, char *param)
{
    struct skparam* ptr = NULL;
    struct skparam* new = NULL;
    int cnt = 1;

    if(!cmd||!param)
        return -EFAULT;

    new = malloc(sizeof(*new));
    assert(new);

    memset(new, 0 , sizeof(*new));
    
    new->param = param;

    ptr = cmd->param;

    if(!ptr){
        cmd->param = new;
        return cnt;
    }

    cnt++;
    while(ptr->next){
        ptr = ptr->next;
        cnt++;
    }

    ptr->next = new;

    return cnt;
    
}



int asocket_cmd_param_add(struct skcmd* cmd, const char *param)
{
//    struct skparam* ptr = NULL;
//    struct skparam* new = NULL;
    
    if(!cmd||!param)
        return -EFAULT;

    return asocket_cmd_param_block_add(cmd, strdup(param));

}



char *asocket_cmd_param_get(struct skcmd* cmd, int num)
{
    struct skparam* ptr = cmd->param;
    int cnt = 0;

    while(ptr){
        if(cnt == num)
            return ptr->param;
        cnt++;
        ptr = ptr->next;
    }
    return NULL;

}

void asocket_param_delete(struct skparam *param)
{
    if(!param)
        return;
    
    asocket_param_delete(param->next);

    free(param->param);
    free(param);

}





int asocket_cmd_write(struct skcmd* cmd, char* buf, int maxlen)
{
    int ptr = 0;
    struct skparam* param = cmd->param;

    assert(cmd);

    ptr += snprintf(buf+ptr,maxlen-ptr, "%s(",cmd->cmd);

    while(param){
        
        if(strchr(param->param, ','))
            ptr += snprintf(buf+ptr,maxlen-ptr, "\"%s\"",param->param);
        else 
            ptr += snprintf(buf+ptr,maxlen-ptr, "%s",param->param);
        
        if(param->next)
            ptr += snprintf(buf+ptr,maxlen-ptr, ",");

        param = param->next;
    }

    ptr += snprintf(buf+ptr,maxlen-ptr, ")nnnn");

    return ptr;                    
}

char* strdupadv(char *start, char *end)
{
     char *str = NULL;

     assert(start);

     if(!end)
         return strdup(start);
     
     if(start > end)
         return NULL;

     if((str = malloc(2 + end - start))==NULL){
         fprintf(stderr, "strdupadv malloc fault\n");
         return NULL;
     }

     memcpy(str, start, 1 + end - start);

     str[1 + end - start] = '\0';

     return str;
     
}


int asocket_cmd_read(struct skcmd** cmd_ret, char *buf, int len, char *errbuf)
{
    char *pstart;
    char *pend;
    char *end;
    char *msg;
    struct skcmd *cmd;

    assert(cmd_ret);

    /* check end */
     if(len < strlen("()nnnn")) 
         return 0; 

    if((end = strstr(buf, "nnnn"))==NULL){
        sprintf(errbuf, "no end");
        return 0;
    }

    assert(4+end - buf <= len);

    msg = strdup(buf);

    if((pstart = strchr(msg, '('))==NULL){
        sprintf(errbuf, "no param start");
        free(msg);
        return -EFAULT;
    }
    
    pstart[0] = '\0';
    pstart++;

    if((pend = strrchr(pstart, ')'))==NULL){
        sprintf(errbuf, "no param end");
        free(msg);
        return -EFAULT;
    }

//    cmd->cmd = strdup(msg);
//    cmd->param = NULL;       
    
    cmd = asocket_cmd_create(msg);
    *cmd_ret = cmd;
    if(pstart >= pend){
        goto out;
    }

    pend[0] = '\0';

    while(pstart){
        char *pnext = NULL;

        if(pstart[0] == '"'){
            pnext = strchr(pstart+1, '"');
            if(pnext){
                pnext[0] = '\0';
                pnext +=2;
                pstart++;
            }
        } 
        
        if(!pnext){
            pnext = strchr(pstart, ','); 
            if(pnext){
                pnext[0] = '\0';
                pnext++;
            }      
        }

         
        
        asocket_cmd_param_add(cmd, pstart);   
        pstart = pnext;
    }
    
  out:
    free(msg);
    return 4+(end-buf);
    
}


int asocket_addr_len(struct sockaddr *skaddr)
{
    switch(skaddr->sa_family){
      case AF_UNIX:
        return sizeof(struct sockaddr_un);
      case AF_INET:
        return sizeof(struct sockaddr_in);
      default:
        fprintf(stderr, "Unknown address family %d\n", skaddr->sa_family);
        return -EFAULT;
    }
}


struct sockaddr *asocket_addr_create_un(const char *path)
{
    struct sockaddr_un *skaddr = malloc(sizeof(*skaddr));
    
    if(!skaddr)
        return 0;

    skaddr->sun_family = AF_UNIX;

    strcpy(skaddr->sun_path, path);

    return (struct sockaddr *)skaddr;
    
}



struct sockaddr *asocket_addr_create_in(const char *ip, int port)
{
    struct sockaddr_in *skaddr = malloc(sizeof(*skaddr));
    
    if(!skaddr)
        return 0;

    skaddr->sin_family = AF_INET;

    skaddr->sin_port = htons(port);
    
    if(!ip)
        skaddr->sin_addr.s_addr = INADDR_ANY;
    else {
        struct hostent *he;
        he = gethostbyname(ip);
        if(he){
            skaddr->sin_addr = *((struct in_addr *)he->h_addr);
        } else {
            skaddr->sin_addr.s_addr = inet_addr(ip);
        }
    }

    return (struct sockaddr *)skaddr;
    
}


void asocket_addr_delete(struct sockaddr *skaddr)
{
	free(skaddr);
}
