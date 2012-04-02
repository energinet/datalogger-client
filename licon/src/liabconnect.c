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

#include <getopt.h>
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
#include <syslog.h>
#include <assert.h>

#include "liabconnect.h"
#include "licon_if.h"
#include "licon_app.h"
#include "licon_conf.h"
#include "licon_util.h"
#include "licon_server.h"
#include "licon_client.h"
#include "licon_log.h"


#define DEFAULT_PIDFILE   "/var/run/licon.pid"
#define DEFAULT_CONFFILE   "/etc/licon.conf"

const  char *HelpText =
 "liabconnect: Daemon for connection to gw.liab.dk... \n"
 " -i           : Interface list, eg. \"eth0:dhcp,ppp:3g,eth0:static\"\n"
 " -u           : Username\n"
 " -p           : Password\n"
 " -c           : Configuratio file\n"
 " -d           : Debug level\n"
 " -t <tunnel>  : Create tunnel \n" 
 " -h           : Help (this text)\n"
 "Christian Rostgaard, April 2010\n" 
 "";

#define OPTIONFILE   "/tmp/optionsfile"
#define KEEPALVEFILE "/tmp/licon.status"



void set_addon_led(int index, int state)
{
  FILE *fp;
  char buf[128];

  snprintf(buf, sizeof(buf), "/sys/class/leds/addon-led-%d/brightness", index);
//  printf("set addon led: %s\n",buf); 
  if((fp = fopen(buf, "w")) != NULL) {
    fprintf(fp, "%d", state ? 1 : 0);
    fflush(fp);
    fclose(fp);
  }
}



char* libcon_hwa_to_usr(char *hwa)
{
    
    int mac[6];

    if(!hwa)
        return NULL;

    if(sscanf(hwa, "%d:%d:%d:%d:%d:%d", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])==6){
        char user[32];
        free(hwa);
        sprintf(user, "%d%d%d%d%d%d", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return strdup(user);
    } 

    free(hwa);

    return NULL;
}


int licon_default(struct licon *lc_config) 
{ 
     lc_config->conn_if  = NULL; 
     lc_config->sck_port = DEFAULT_PORT;
     return 0; 
} 



int licon_valid_conf(struct licon *lc_config)
{
    if(!lc_config->if_lst)
        return -EFAULT;

    return 0;
}



int licon_connect(struct licon *lc_confi)
{
    struct licon_if* ptr = lc_confi->if_lst;
    int retval = -EFAULT;

    while(ptr){
        int if_state = licon_if_chk(ptr);
        retval = 0;       
      
        if(if_state == IFSTATE_CONN){
	    // set_addon_led(ptr->index+10, 1);
            retval = 0;
            break;
        } 

        if(ptr->state == IFSTATE_CONN && if_state != IFSTATE_CONN){
            /* turn off if */
            printf("disconnecting new state : %s\n", licon_if_state_str(if_state));
            licon_if_disconnect(ptr);
            lc_confi->conn_if = NULL;
            licon_app_stop_all(lc_confi->app_lst);
            if(lc_confi->cmd_down)
                licon_cmd_run(lc_confi->cmd_down);
            retval = 0;
            /* and exit */
            break;
        }

        switch(if_state){
          case IFSTATE_DOWN:
          case IFSTATE_WAIT:
          case IFSTATE_ERR:
            licon_if_set_state(ptr,if_state , -1);
            ptr = ptr->next;
            continue;
          default:
            break;
        }
        //set_addon_led(ptr->index+10, 1);
        retval = licon_if_connect(ptr, lc_confi->d_level);

        if(retval >= 0){
            ptr->fail = 0;
            if(lc_confi->conn_if){
                licon_app_stop_all(lc_confi->app_lst);
                licon_if_disconnect(lc_confi->conn_if);
                lc_confi->conn_if = NULL;
                if(lc_confi->cmd_down)
                    licon_cmd_run(lc_confi->cmd_down);
            }
            lc_confi->conn_if = ptr;
            if(licon_app_start_all(lc_confi->app_lst)<0)
		lc_confi->apperrcnt++;
            if(lc_confi->cmd_up)
                licon_cmd_run(lc_confi->cmd_up);
            break;
        } 

        //set_addon_led(ptr->index+10, 0);
        ptr = ptr->next;
    }

    return retval;

}

static char pidfile[512];

int pid_file_create(const char *pidfile_name)
{
  int ret;
  struct stat s; 
  FILE *fp;
  int pid_val;
  int fd;

  assert(pidfile_name != NULL);

  /* Does a pid-file already exist? */
  snprintf(pidfile, sizeof(pidfile), "%s", pidfile_name);
  ret = stat(pidfile, &s);

  if(ret == 0) {
    if((fp = fopen(pidfile, "r")) != NULL) {
      /* Read pid */          
      if(!fscanf(fp, "%d[^\n]", &pid_val)) {
        fclose(fp);        
        return (-1);
      } else {
        fclose(fp);
        fprintf(stdout, "found pid in file: %d\n", pid_val);
        /* If so, try to kill the process */
        if(kill(pid_val, SIGTERM) == 0 || errno == ESRCH) {
          fprintf(stdout, "Killed process pid %d\n", pid_val);
          fprintf(stdout, "deleting %s",pidfile);
          /* Delete PID file */
          if(unlink(pidfile) != 0) {
            fprintf(stderr, "unlink() failed: %s", strerror(errno));
            return (-1);
          } else {
            goto create;
          }
        } else {
          fprintf(stderr, "kill() failed: %s", strerror(errno));
          return (-1);
        }
      }        
    } else {
      return (-1);
    }
  } else if(ret == -1 && errno == ENOENT) {
    /* Try to create a PID file */
    fprintf(stdout, "No process running... creating PID file");
  create:
    fd = open(pidfile, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0) {
      fprintf(stderr, "open() failed: %s", strerror(errno));
      return (-1);
    } else {
      if((fp = fdopen(fd, "w")) != NULL) {
          fprintf(stdout, "pidfile with PID: %d", (unsigned int)getpid());
        fprintf(fp, "%ld\n", (long) getpid());
        fclose(fp);
        close(fd);
        return (0);
      } else {
        close(fd);
        return (-1);
      }        
    }
  } else {
    return (-1);
  }
}

int pid_file_remove(void)
{
  if (unlink(pidfile) != 0)
    return -1;

  return 0;
}


int pidfile_pidwrite(const char *pidfile)
{
    FILE *fp;

    if((fp = fopen(pidfile, "w")) == NULL) {
        fprintf(stderr, "Error: could not open pidfile for writing: %s\n", strerror(errno));
        return errno;
    }
    
    fprintf(fp, "%ld\n", (long) getpid());
    
    fclose(fp);

    return 0;
}



int pidfile_create(const char *pidfile)
{
    int ret;   
    struct stat s; 
    FILE *fp;
    int filepid;
    
    ret = stat(pidfile, &s);

    if(ret == 0){
        /* file exits: open file */
        if((fp = fopen(pidfile, "r")) == NULL) {
             fprintf(stderr, "Error: could not open pidfile: %s\n", strerror(errno));
             return -errno;
        }
        
        /* read pid */
        ret = fscanf(fp, "%d[^\n]", &filepid);
        fclose(fp);
        
        if(ret !=1){
            fprintf(stderr, "Error: reading pidfile: %s\n", strerror(errno));
            return -errno;
        }

        printf("pid in file is  %d\n", filepid);

        if (kill (filepid, 0) == 0){
             printf("pid is running... \n");
             return 0;
        }
    }

    pidfile_pidwrite(pidfile);

    return 1;

}


int pidfile_close(const char *path)
{
    if (unlink(path) != 0)
        return -1;
    
    return 0;
  
}

int run = 1;
int hupped = 0;

void signal_handler(int sig)
{

  switch(sig) {
  case SIGHUP:
    hupped = 1;
    break;
  case SIGINT:
    hupped = 1;
    run = 0;
    break;
  default:
    break;
  }

}


int licon_get_status(struct licon *lc_config)
{
    
    int appsta;

    if(!lc_config->conn_if)
	return LICON_IF_DOWN;
    
    appsta = licon_app_status_all(lc_config->app_lst);

    if(appsta>=APPSTA_RUN)
	return LICON_APP_RUN;

    if(appsta>=APPSTA_STARTING)
	return LICON_APP_START;
    
    return LICON_IF_UP;
	
}

void licon_print_status(struct licon *lc_config)
{
    char rx_buf[1024*10];
    char rx_len = 0;

    if(lc_config->d_level) printf("licon is allready running status:\n");
    licon_client_connect(lc_config->sck_port );
    if(lc_config->d_level) printf("connected to socket (port %d)\n", lc_config->sck_port);
    rx_len = licon_client_get_status(rx_buf, sizeof(rx_buf));
    if(lc_config->d_level) printf("status received\n");
    licon_client_disconnect();

    if(lc_config->d_level) printf("%s\n", rx_buf);
    
}

int main(int narg, char *argp[])
{
    struct licon lc_config;
    int optc;
    int retval;
    int daemonize_flag = 1;
    int detach = DETACH_NOW;
    int socket_run = 1;
    int pidfile_fd = -1;
    int default_config = 1;
    int if_error = 0;

    struct licon_if *last_if = NULL;
    struct licon_app *last_app = NULL;
    memset(&lc_config, 0 , sizeof(lc_config));

    licon_default(&lc_config);

    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);

    while ((optc = getopt(narg, argp, "hd:i:u:p:c:lU:D:fFnt:s::v")) > 0) {
        switch (optc){ 
          case 'c':
	    default_config = 0;
            if((retval = licon_conf_read_file(&lc_config, optarg)) < 0){
                fprintf(stderr, "Error reading config file\n");
                return retval;
            }
            break;
          case 's':
	    if(optarg)
		lc_config.sck_port = atoi(optarg);

	    if(lc_config.sck_port == 0)
		socket_run = 0;
            break;
          case 'i':
	    default_config = 0;
            last_if = licon_if_create_str(optarg);
            lc_config.if_lst = licon_if_add(lc_config.if_lst,  last_if );
            break;
          case 'U':
            if(!last_if)
                lc_config.cmd_up = strdup(optarg);
            else 
                last_if->precmd = strdup(optarg);
            break;
          case 'D':
            if(!last_if)
                lc_config.cmd_down = strdup(optarg);
            else
                last_if->postcmd = strdup(optarg);
            break;
          case 'R':
            if(!last_if){
                fprintf(stderr, "Error: no interface added!!!\n");
                exit(0);
            }
            last_if->repcmd = strdup(optarg);
            break;
          case 'u': 
            lc_config.t_user = strdup(optarg);
            break;
          case 'p':
            lc_config.t_passwd = strdup(optarg);
            break;
          case 'd':
            lc_config.d_level = atoi(optarg);
            break;
          case 'l':
            retval = licon_eth_link_sta("eth0");
            printf("Link status is %d\n", retval);
            return retval;
          case 'f':
            detach = DETACH_NONE;
            break;
          case 'n':
            detach = DETACH_NOW;
            break;
	  case 'F':
            detach = DETACH_UP;
            break;
          case 't':
            last_app = licon_tun_create(optarg);
            lc_config.conn_tun = last_app;
            lc_config.app_lst = licon_app_add(lc_config.app_lst,  last_app);
            break;
          case 'v':
            last_app = licon_app_testvpp_create();
            lc_config.app_lst = licon_app_add(lc_config.app_lst,  last_app);
            break;
          case 'h':
          default:
            fprintf(stderr, "%s", HelpText);
            exit(0);
            break;
        }
    }



    if(default_config){
	if((retval = licon_conf_read_file(&lc_config, DEFAULT_CONFFILE)) < 0){
	    fprintf(stderr, "Error reading config file\n");
	    return retval;
	}
    }

    /* create/open pidfile */
    pidfile_fd = pidfile_create(DEFAULT_PIDFILE);

    if( pidfile_fd== 0){
	fprintf(stderr, "licon is allready running...\n");
//        licon_print_status(&lc_config);
        exit(0);
    } else if( pidfile_fd < 0){
        exit(1);
    }

    if(lc_config.d_level){
	licon_if_print(lc_config.if_lst);
	licon_app_print(lc_config.app_lst);
    }

    if((retval = licon_valid_conf(&lc_config))<0){ 
         fprintf(stderr, "config not valid\n"); 
         return retval; 
    } 
    
    if(detach == DETACH_NOW){
        if(daemon(0,0))
            fprintf(stderr, "Daemon failed: %s\n", strerror(errno));
        pidfile_pidwrite(DEFAULT_PIDFILE);
	licon_sysdbg_set(2);
    }

    if(socket_run&&detach != DETACH_UP)
        licon_server_start(&lc_config);


    while(run){
        hupped = 0;
	if_error = 0;
	licon_check_reset(lc_config.net_check);
        while(!hupped&&!if_error){
            licon_connect(&lc_config);
            if(lc_config.conn_if){
		if(licon_check_do(lc_config.net_check)<0){
		    if_error = 1;
		} 
		
		switch(licon_app_check_all(lc_config.app_lst)){
		  case APPSTA_FATAL:
		    if_error = 1;
		    break;
		  case APPSTA_ERR:
		    lc_config.apperrcnt++;
		    break;
		  default:
		    if_error = 0;
		    break;
		}
		
	    }
            sleep(1);
            

            if(detach == DETACH_UP){
                detach = DETACH_NONE;
                if(daemon(0,0))
                    fprintf(stderr, "Daemon failed: %s\n", strerror(errno));
                pidfile_pidwrite(DEFAULT_PIDFILE);
		licon_sysdbg_set(2);
                if(socket_run)
                    licon_server_start(&lc_config);
            }


        }
	if(if_error)
	    fprintf(stderr, "if error: disconnecting\n"); 
	else
	    fprintf(stderr, "Signal: disconnecting\n"); 
	 
        if(lc_config.conn_if){
            licon_app_stop_all(lc_config.app_lst);
            licon_if_disconnect(lc_config.conn_if);
	    if(if_error){
		licon_if_set_state(lc_config.conn_if,IFSTATE_ERR , USE_WAITS);
	    }
            lc_config.conn_if = NULL;
            if(lc_config.cmd_down)
                licon_cmd_run(lc_config.cmd_down);
        }
    }

    if(socket_run)
        licon_server_stop();

//    pid_file_remove();

    pidfile_close(DEFAULT_PIDFILE);

    return 0;
    
    
}
