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

#include "licon_if.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned int u64;
typedef unsigned char u8;
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <syslog.h>
#include <assert.h>
#include <xml_parser.h>


#include "licon_log.h"
#include "licon_util.h"

#define PIDFILE_DHCP "/var/run/udhcpc.pid"
#define PIDFILE_MODEM "/var/run/ppp%d.pid"

#define DETAULT_PPPAPN "internet"

const char* licon_if_state_str(enum ifstate state)
{   
    switch(state){
      case IFSTATE_INIT:
        return "init";
      case IFSTATE_ERR:
        return "error";
      case IFSTATE_DOWN:
        return "down";
      case IFSTATE_WAIT:
        return "wait";
      case IFSTATE_UP:
        return "up";
      case IFSTATE_TRYCONN:
	return "tryconn";
      case IFSTATE_CONN:
        return "conn.ed";
      default:
        return "unknown";
    }
}

int licon_if_get_name(struct licon_if * if_obj, char* buffer, int maxlen)
{
    switch(if_obj->type){
      case IFTYPE_ETH:
	return snprintf(buffer, maxlen, "%s", if_obj->name);
      case IFTYPE_PPP:
	return snprintf(buffer, maxlen, "%s:%s", if_obj->name, if_obj->ppp.peer);
      default:
	return snprintf(buffer, maxlen, "%s!!", if_obj->name);
    }
}


int licon_if_set_state(struct licon_if * if_obj, int state, int timeout)
{
    struct timeval tv;

    if( timeout >= 0){
        gettimeofday(&tv, NULL);
        if_obj->chk_next = tv.tv_sec + timeout;
    } else if(timeout == USE_WAITS) {
	if_obj->chk_next = licon_time_get_next_chk(if_obj->fail, 0, if_obj->waittime);
    }
    
    if(state != if_obj->state){
		char name[32];
		int uwtime = licon_time_until(if_obj->chk_next, TIME_NOW);
		licon_if_get_name(if_obj, name, sizeof(name));
		PRINT_LOG( LOG_INFO, ">%s: %s--> %s (%lu sec) (errors %d)\n", name, licon_if_state_str(if_obj->state), 
				   licon_if_state_str(state), uwtime, if_obj->fail);
		if(uwtime)
			licon_log_eventv("if", name, licon_if_state_str(state), "(f%d)[%d]", if_obj->fail,uwtime);
		else
			licon_log_eventv("if", name, licon_if_state_str(state), "(f%d)", if_obj->fail);
    }
    
    if_obj->state = state;   
	
    return 0;
}


int licon_if_get_state(struct licon_if * if_obj)
{
     struct timeval tv;

     if(if_obj->state == IFSTATE_WAIT || if_obj->state == IFSTATE_ERR){
         gettimeofday(&tv, NULL);
         if(if_obj->chk_next <= tv.tv_sec)
             return IFSTATE_UP;
     }    

     return if_obj->state;
}



struct licon_if *licon_if_create(const char* name, const char *mode, const char *param1)
{
    
    struct licon_if *new = NULL;
    
    if(!name||!mode){
        fprintf(stderr, "If create: %s %s\n", name, mode);
        goto err_out;
    }

    new = malloc(sizeof(*new));

    if(!new){
        fprintf(stderr, "Could not allocate memory for IF!\n");
        goto err_out;
    }   

    memset(new, 0 , sizeof(*new));
    
    if(strncmp(name, "eth", 3)==0){
        int ifnum;
        new->type = IFTYPE_ETH;
        if(sscanf(name , "eth%d", &ifnum)!=1){
            fprintf(stderr, "error reading %s\n", name);
            goto err_out_dealloc;
        } else if(ifnum < 0 && ifnum > 255){
            fprintf(stderr, "error eth num aut of bound %s\n", name);
            goto err_out_dealloc;
        }
		if(strcmp(mode, "dhcp")==0){
			new->eth.dhcp = 1;
			new->pidfile = strdup(PIDFILE_DHCP);
		}
        new->name = strdup(name);

    } else if(strncmp(name, "ppp", 3)==0){
		char pidfile[256];
		char pppname[64];
		if(sscanf(name , "ppp%d", &new->ppp.unit)!=1){
			new->ppp.unit=0;
        } else if(new->ppp.unit < 0 && new->ppp.unit > 255){
            fprintf(stderr, "error ppp num out of bound %s\n", name);
            goto err_out_dealloc;
        }

		new->type = IFTYPE_PPP;
		snprintf(pidfile, sizeof(pidfile), "/var/run/ppp%d.pid",  new->ppp.unit);
		new->pidfile = strdup(pidfile);
		new->ppp.apn  = licon_defdup(param1, DETAULT_PPPAPN);
		new->ppp.peer = strdup(mode);
		snprintf(pppname, sizeof(pppname), "ppp%d",  new->ppp.unit);
		new->name = strdup(pppname);
    } else {
        goto err_out_dealloc;
    }
    

	new->maxfail = 10;    

    pthread_mutex_init(&new->mutex , NULL);

    licon_if_set_state(new, IFSTATE_INIT, NO_WAIT);

    return new;

  err_out_dealloc:
    free(new);
  err_out:
    return NULL;
    

}

struct licon_if *licon_if_create_str(const char* if_str)
{
    struct licon_if *new;
    char *name;
    char *mode;

    if(!if_str){
        fprintf(stderr, "No input IF string!\n");
        return NULL;
    }
    

    name = strdup(if_str); 
    mode = strchr(name, ':');

    if(mode){
        mode[0] = '\0';
        mode++;
    }
    
    new = licon_if_create(name, mode, NULL);

    free(name);

    return new;

   

}


void licon_if_print(struct licon_if * if_obj)
{
    if(!if_obj)
        return;

    switch(if_obj->type){
      case IFTYPE_ETH:
	printf("if: %s , mode: %s state: %s\n", 
	       if_obj->name, (if_obj->eth.dhcp)?"dhcp":"static", licon_if_state_str(if_obj->state));
	break;
      case IFTYPE_PPP:
	printf("if: %s , peer: %s, apn %s state: %s\n", 
	       if_obj->name, if_obj->ppp.peer,  if_obj->ppp.apn ,licon_if_state_str(if_obj->state));
	break;
    }
    
    if(if_obj->precmd)
        printf("  precmd:  %s\n", if_obj->precmd);
    else
	printf("  precmd!:  %s\n", if_obj->precmd);

    if(if_obj->postcmd)
        printf("  postcmd: %s\n", if_obj->postcmd);

    licon_if_print(if_obj->next);
}


int licon_if_status(struct licon_if * if_obj, char* buf, int maxlen)
{
    int ret = 0;
    int pid = 0;
    time_t until = 0;
    char name[32];

    if(!if_obj)
        return snprintf(buf+ret, maxlen-ret, "if name             ,%7s ,   pid , fail \n", "state");

    licon_if_get_name(if_obj, name , sizeof(name));

    until = licon_time_until(if_obj->chk_next , 0);

    pid = licon_pidfile_pid(if_obj->pidfile);

    snprintf(buf+ret, maxlen-ret , "%s                   ",  name );

    ret += 20;
    ret += snprintf(buf+ret, maxlen-ret, ",%7s , %5d , %5d ", 
                    licon_if_state_str(if_obj->state), pid, if_obj->fail);


    if(until){
	ret += snprintf(buf+ret, maxlen-ret, "(%lu sec)",   until );
    } 

    if(if_obj->type == IFTYPE_PPP){
	ret += snprintf(buf+ret, maxlen-ret, "(%s%s)",  (if_obj->is_on)?"on":"off", (if_obj->alwayson)?",alwayson":"" );
    }    

    ret += snprintf(buf+ret, maxlen-ret, "\n");

    return ret;
}


struct licon_if *licon_if_add(struct licon_if * list, struct licon_if * new)
{
    struct licon_if *ptr = list;
    int index = 0;
    if(!list){
        new->index = index;
        return new;
    }
    index++;

    while(ptr->next){
        index++;
        ptr = ptr->next;
    }
    
    ptr->next = new;
    new->index = index;
    
    return list;
}

struct licon_if *licon_if_get(struct licon_if *list, const char *name)
{
    struct licon_if *ptr = list;

    while(ptr){
	if(strcmp(ptr->name, name)==0)
	    return ptr;
	ptr = ptr->next;
    }
    
    return NULL;

}



int licon_if_chk(struct licon_if* if_obj)
{
    int state = licon_if_get_state(if_obj);

    if(state == IFSTATE_DOWN)
        state = IFSTATE_UP;

    /* cjeck link */
    switch(if_obj->type){
      case IFTYPE_ETH:
        if(licon_eth_link_sta(if_obj->name) == IFSTATE_DOWN){
            state = IFSTATE_DOWN;
        }
      case IFTYPE_PPP:
      default:
        break;
    }

    /* check pidfile */
    if(state == IFSTATE_CONN && if_obj->pidfile){
        if(licon_pidfile_check(if_obj->pidfile)){
            printf("pidfile error\n");
            state = IFSTATE_DOWN;
        }
    } 

    return state;
    
    
}



/**********
 * ETH
 */
static int skfd = -1;

int licon_eth_link_sta(const char *ifname)
{
    struct ifreq ifr;
    struct ethtool_value edata;
    
    if(skfd == -1){ 
        if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ){
            printf("socket error: %s\n",strerror(errno) );
            skfd = -1;
            return -EFAULT;        
        }
        printf("Opening socket to  %s\n", ifname );
    }
    
    edata.cmd = ETHTOOL_GLINK;
        
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
    ifr.ifr_data = (char *) &edata;
    
    
    if (ioctl(skfd, SIOCETHTOOL, &ifr) == -1) {
        printf("Link state failed: %s\n", strerror(errno));
        close(skfd);
        skfd = -1;
        return IFSTATE_DOWN;
    }
    
//    close(skfd);
//    printf("%s link state: %s\n",ifname,edata.data ? "Link" : "No link"  );
    return (edata.data ? IFSTATE_UP : IFSTATE_DOWN);
}


int licon_eth_connect(struct licon_if* if_st)
{
    char cmd[512];
    int retval;
    
    if(if_st->state == IFSTATE_CONN){
        printf("IF is allready connected\n");
        return 0;
    }

    printf("connecting %s\n", if_st->name);
    /* test link */ 
    if(licon_eth_link_sta( if_st->name)==IFSTATE_DOWN){ 
        licon_if_set_state(if_st, IFSTATE_DOWN, NO_WAIT);
        return -EFAULT;
    } 
    
    // start eth
    sprintf(cmd, "ifconfig %s up", if_st->name);
    retval = system(cmd);
    if(retval == -1){
	
        fprintf(stderr, "could not run \"%s\"\n", cmd);
        licon_if_set_state(if_st, IFSTATE_ERR, USE_WAITS);
        return -EFAULT;
    } else {
        if(WEXITSTATUS(retval)){
            fprintf(stderr, "\"%s\" returned %d\n", cmd, WEXITSTATUS(retval));
            licon_if_set_state(if_st, IFSTATE_ERR, USE_WAITS);
            return -EFAULT;
        }
    }

    if(if_st->eth.dhcp){
        printf("starting %s with DHCP\n", if_st->name);
        /* start dhcp client */
        sprintf(cmd, "/sbin/udhcpc --now --pidfile="PIDFILE_DHCP"\n");
        retval = system(cmd); 
        if(retval == -1){
            fprintf(stderr, "could not run \"%s\"\n", cmd);
            licon_if_set_state(if_st, IFSTATE_ERR, USE_WAITS);
            return -EFAULT;
        } else {
            if(WEXITSTATUS(retval)){
                fprintf(stderr, "\"%s\" returned %d\n", cmd, WEXITSTATUS(retval));
                licon_if_set_state(if_st, IFSTATE_WAIT, USE_WAITS);
                return -EFAULT;
            }          
        }
//        if_st->pidfile = strdup(PIDFILE_DHCP);
    } else {
        licon_pidfile_stop(PIDFILE_DHCP); //Måske ikke nødvendig
        printf("starting %s with STATIC ip\n",if_st->name );
    }
    licon_if_set_state(if_st, IFSTATE_CONN, NO_WAIT);
    printf("connected %s\n", if_st->name);
    return 0;
}


int licon_eth_down(struct licon_if* if_obj)
{      

    return 0;
}

/***
 * PPP
 */

int licon_ppp_connect(struct licon_if* if_obj, int dbglev)
{
    char cmd[512];
    int retval = 0;
    int sysret = 0;
    time_t t_begin, t_end;
    PRINT_LOG(LOG_INFO, "Running ppp connect for %s\n",if_obj->name);
    printf("starting ppp %s\n", if_obj->ppp.peer);

    sprintf(cmd, "pon %s updetach maxfail 1 unit %d %s\n", if_obj->ppp.peer, if_obj->ppp.unit, (dbglev)?"debug":"" );

    if(if_obj->ppp.apn)
	setenv("PPPAPN", if_obj->ppp.apn,1 );

    t_begin = time(NULL);

    PRINT_LOG(LOG_INFO,"cmd: %s", cmd);
    sysret = system(cmd); 
    PRINT_LOG(LOG_INFO, "%s returned %x\n",cmd, sysret);

    t_end = time(NULL);

    if(sysret == -1){
        PRINT_LOG(LOG_ERR, "could not run \"%s\"\n", cmd);
	retval = -EFAULT;
    } else {
	if(!WIFEXITED(sysret)){
	    if(WIFSIGNALED(sysret))
		PRINT_LOG(LOG_ERR, "\"%s\" received signal %d\n", cmd, WTERMSIG(sysret));
	    else
		PRINT_LOG(LOG_ERR, "\"%s\" exited unusial %d\n", cmd, sysret);
	    retval = -EFAULT;
	} else {
	    if(WEXITSTATUS(sysret)){
		PRINT_LOG(LOG_ERR, "\"%s\" returned %d\n", cmd, WEXITSTATUS(sysret));
		retval = -EFAULT;
	    } 
	}
	    
    }

    if(retval<0){
	licon_if_set_state(if_obj, IFSTATE_ERR, USE_WAITS);
	if((t_end - t_begin)>300){
	    retval = -100;
	}
    } else {
	licon_if_set_state(if_obj, IFSTATE_CONN, NO_WAIT);
	PRINT_LOG(LOG_INFO, "%s connect success (%d sec) \n",if_obj->name, t_end - t_begin);
    }

    return retval;
}


/**
 * Turn the in on and off with the pre- and post-cmd 
 * @param if_obj The interface object 
 * @param seton The setvalue: 1 on, 0 off, and -1 force off
 * @return 0 on sucess and negative value if error
 */

int licon_if_onoff(struct licon_if* if_obj, int seton)
{
    const char *cmd;

    PRINT_LOG(LOG_INFO, "Set %s on/off %d\n", if_obj->name, seton);

    if(if_obj->is_on == seton){
	PRINT_LOG(LOG_INFO, "Is allready set\n");
	return 0;
    }

    if(seton == 0 && (
	   (if_obj->alwayson) || 
	   (if_obj->state>=IFSTATE_TRYCONN)
	   )){
	PRINT_LOG(LOG_INFO, "Cannot turn off %s: %s state %s\n", if_obj->name,
		  (if_obj->alwayson)?"(alwayson)":"",  licon_if_state_str(if_obj->state ));
	return 0;
    }
    
    seton = (seton>0)?1:0;

    if(seton){
	cmd = if_obj->precmd;
    } else {
	cmd = if_obj->postcmd;
    }
       
    if(cmd){
	PRINT_LOG(LOG_INFO, "Running %s\n", cmd);
	if(licon_cmd_run(cmd)){
	    PRINT_LOG(LOG_ERR, "Command %s failed\n", cmd);
	    return -EFAULT;
	} 
    }
       
    
    if_obj->is_on = seton;
    
    PRINT_LOG(LOG_INFO, "On/off state for %s changed (%s)\n", if_obj->name, (seton)?"on":"off");
    return 0;
}


int licon_if_alwayson(struct licon_if* if_obj, int seton)
{
    int retval;
    PRINT_LOG(LOG_INFO, "Set always on: %d\n", seton);
    pthread_mutex_lock(&if_obj->mutex); 
   
    if_obj->alwayson = seton;
    
    retval = licon_if_onoff(if_obj, seton);
   
    pthread_mutex_unlock(&if_obj->mutex); 
    PRINT_LOG(LOG_INFO, "Set always on completed: %d\n", seton);
    return retval;
}

int licon_if_connect(struct licon_if* if_obj, int dbglev)
{
    int retval;

    licon_if_set_state(if_obj,IFSTATE_TRYCONN , NO_WAIT);
    PRINT_LOG(LOG_INFO, "Try to connect %s\n",if_obj->name );
    pthread_mutex_lock(&if_obj->mutex); 

    if(if_obj->pidfile){
		if(!licon_pidfile_check(if_obj->pidfile)){
			fprintf(stderr, "\"%s\" id running \n", if_obj->pidfile);
			licon_pidfile_stop(if_obj->pidfile);
		}
    }


    if(licon_if_onoff(if_obj, 2)){
		retval = -EFAULT;
		licon_if_set_state(if_obj,IFSTATE_ERR , USE_WAITS);
		goto out;
    }


    switch(if_obj->type){
      case IFTYPE_ETH:
        retval = licon_eth_connect(if_obj);
        break;
      case IFTYPE_PPP:
        retval = licon_ppp_connect(if_obj, dbglev);
        break;
      default:
        licon_if_set_state(if_obj,IFSTATE_ERR , 300);
        retval = -EFAULT;
        break;
    }

  out:
    if(retval < 0){
        if_obj->fail++;
        if((if_obj->fail!=0) && ((if_obj->fail%if_obj->maxfail)==0) && (if_obj->maxfail>0)){
			if(if_obj->repcmd){
				licon_cmd_run(if_obj->repcmd);
			} else {
				licon_if_onoff(if_obj, -1);
				if(if_obj->alwayson)
					licon_if_onoff(if_obj, 1);
			}

            licon_if_set_state(if_obj,IFSTATE_ERR , 3); 
        }
    } else {
        if_obj->fail = 0;
    }

    pthread_mutex_unlock(&if_obj->mutex); 
    PRINT_LOG(LOG_INFO, "Try to connect %s returned %d\n",if_obj->name, retval );
    return retval;
    
}


int licon_if_disconnect(struct licon_if* if_obj)
{      
    
    PRINT_LOG(LOG_INFO, "disconnecting %s\n",if_obj->name);
    //licon_if_set_state(if_obj,IFSTATE_TRYCONN , NO_WAIT);

    switch(if_obj->type){
      case IFTYPE_ETH:
        licon_eth_down(if_obj);
        break;
      case IFTYPE_PPP:
      default:
        break;
    }
    licon_pidfile_stop(if_obj->pidfile);
    licon_if_set_state(if_obj, IFSTATE_DOWN, NO_WAIT);

    sleep(1);

    licon_if_onoff(if_obj, 0);
    /* if(if_st->postcmd) */
    /*     licon_cmd_run(if_st->postcmd); */

    pthread_mutex_unlock(&if_obj->mutex); 
    PRINT_LOG(LOG_INFO, "%s disconnected\n",if_obj->name);
    return 0;
}
