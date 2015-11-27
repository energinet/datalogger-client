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

#include "rpclient_cmd.h"
#define _XOPEN_SOURCE
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <logdb.h>
#include <linux/if.h>
#include <sys/ioctl.h>
//#include <linux/in.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <syslog.h>

#include "rpclient_soap.h"
#include "rpclient_db.h"

#include <cmddb.h>

char *dupsprintf(const char *format, ...)
{
    char tmpbuf[256];
    va_list ap;

    va_start(ap, format);
    vsnprintf(tmpbuf, sizeof(tmpbuf), format, ap);
    va_end(ap);

    return strdup(tmpbuf);

}


int boxcmd_relay_set(int num, int value)
{
      FILE *fp;
  char buf[128];

  snprintf(buf, sizeof(buf), "/sys/class/leds/relay_%d/brightness", num);
  
  if((fp = fopen(buf, "w")) != NULL) {
      fprintf(fp, "%d", value ? 1 : 0);
      fflush(fp);
      fclose(fp);
  }

  return 0;
}



int boxcmd_run_system(const char *format, ...)
{
    int status;
    char cmd[256];
    va_list ap;

    va_start(ap, format);
    vsnprintf(cmd, sizeof(cmd), format, ap);
    va_end(ap);

    syslog(LOG_INFO, "executing: %s\n", cmd);

    status = system(cmd); 
    if(status == -1){
        syslog(LOG_ERR, "Error running \"%s\"\n", cmd);
        return -EFAULT;
    } 
    
    return WEXITSTATUS(status);
}


int rp_boxinfo_set(struct rpclient *client_obj, struct boxInfo *boxinfo); //ToDo: header + reorder functions 
int file_get(struct rpclient *client_obj,struct soap *soap, const char *address, char *filename, const char *destpath, const char *modestr);

void boxcmd_set_(struct boxCmd *cmd, const char *id, int sequence, int pseq, 
	       unsigned long trigger_time, int param_cnt, ...)
{
    
    va_list ap;
    int i;

    cmd->name      = strdup(id);
    cmd->sequence  = sequence;
    cmd->pseq      = pseq;
    cmd->params    = malloc(sizeof(void*)*param_cnt);
    cmd->paramsCnt = param_cnt;
    {
        char ttime[32];
        sprintf(ttime, "%ld",  trigger_time);
        cmd->trigtime = strdup(ttime);
    }
    va_start (ap, param_cnt);
    for(i=0;i<param_cnt;i++){
        cmd->params[i] = strdup(va_arg(ap,const char *));
    }
    va_end(ap);
}

void boxcmd_set(struct boxCmd *boxcmd, struct cmddb_cmd *dbcmd){
    boxcmd_set_(boxcmd, dbcmd->name,  dbcmd->cid, 0, dbcmd->ttime, 1, dbcmd->value);
}


int boxcmd_sent_status(struct rpclient *client_obj, struct soap *soap, const char *address, int cid, int status, 
		       struct timeval *timestamp, const char* retstr, struct cmddb_cmd *dbcmd)
{
    int err;
    int retval = 0;
    char time_str[64];
    struct boxCmdUpdate update;
    struct boxInfo *boxinfo = &update.boxInf;
    struct boxCmd *boxcmd = &update.cmd;

    syslog(LOG_DEBUG,"cmdUpdate (cid %d)\n", cid);

    rp_boxinfo_set(client_obj, boxinfo);
    boxcmd_set(boxcmd, dbcmd);

    update.sequence = cid;
    update.status   = status;
    if(retstr)
	update.retval   = strdup(retstr);
    else
	update.retval   = strdup("");
    
    if(timestamp){
	sprintf(time_str, "%lu", timestamp->tv_sec);
    } else {
	struct timeval time;
	gettimeofday(&time, NULL);
	sprintf(time_str, "%lu", time.tv_sec);
    }
    
    update.timestamp = strdup(time_str);

    struct rpclient_soap *rpsoap;
    rpsoap = client_obj->rpsoap;
    int retries = 0;
    retval = -1;
    http_da_restore(soap, rpsoap->dainfo);

    do{
	err = soap_call_liabdatalogger__cmdUpdate(soap, address, NULL, &update, &retval);
	if(err==SOAP_OK){
	    syslog(LOG_DEBUG,"cmdUpdate success (returned %d) for cid %d\n", retval, cid);
	    if(retval > 0 && cid < 0)
		cmd_cid_change(cid, retval); 
	    retval = 0;
	    break;
	}
    }while(rpclient_soap_hndlerr(rpsoap,soap, rpsoap->dainfo, retries++, __FUNCTION__));

    return retval;
}


void boxcmd_print(int priority, struct boxCmd *cmd) 
 { 
     int n; 

     syslog(priority, "cmd %d \"%s\"\n", cmd->sequence, cmd->name); 
     for(n = 0; n <  cmd->paramsCnt ; n++){ 
	 syslog(priority, "   param %d \"%s\"\n", n, cmd->params[n]); 
     } 
 } 

int dbcmd_setfile(CMD_HND_PARAM)
{
	int retval_ = 0;
	char *file = NULL;
    char *descrip = NULL;
	struct cmdhnd_data *data = (struct cmdhnd_data *)sessdata;

	if(!cmd->value){
		*retstr = strdup("error paramater");
		return CMDSTA_EXECERR;
    }


	
	file = strdup(cmd->value);
    descrip = strchr(file, ',');
    if(!descrip){
		descrip = file;
    } else {
		descrip[0]= '\0';
		descrip++;
	}

	retval_ = file_set(data->client_obj, data->soap, data->address, 
					   descrip,file);
	
	if(retval_ < 0){
		char tmpbuf[256];
		sprintf(tmpbuf, "error writing file: %s", strerror(-retval_));
		*retstr = strdup(tmpbuf);
		return CMDSTA_EXECERR;
    }

	return CMDSTA_EXECUTED;    
}

int dbcmd_getfile(CMD_HND_PARAM)
{
    int retval_ = 0;
    char *file = NULL;
    char *dest = NULL;
    char *mode = NULL;
    struct cmdhnd_data *data = (struct cmdhnd_data *)sessdata;
    
   
    if(!cmd->value){
		*retstr = strdup("error paramater");
		return CMDSTA_EXECERR;
    }

    file = strdup(cmd->value);
    dest = strchr(file, ',');
    if(!dest){
		*retstr = strdup("error paramater");
		return CMDSTA_EXECERR;	
    }
    
    dest[0] = '\0';
    dest++;

    mode =  strchr(dest, ',');
    if(mode){
	mode[0]='\0';
	mode++;
    }
    
    retval_ = file_get(data->client_obj, data->soap, data->address, file, dest, mode);

    if(retval_ < 0){
	char tmpbuf[256];
	sprintf(tmpbuf, "error writing file: %s", strerror(-retval_));
	*retstr = strdup(tmpbuf);
	return CMDSTA_EXECERR;
    }

    return CMDSTA_EXECUTED;    
}



int rpclient_db_getconfs(struct cmdhnd_data *data, const char *confdir)
{
    char *licon_file = "licon.conf";
    char licon_path[256];
    char *contdaem_file = "contdaem.conf";
    char contdaem_path[256];
    int retval = 0;
    
    sprintf(licon_path, "%s/%s", confdir, licon_file);
    sprintf(contdaem_path, "%s/%s", confdir, contdaem_file);

    retval = boxcmd_run_system("/bin/mkdir -p %s", confdir);
    if(retval != 0){
	return -1;
    }

    retval = file_get(data->client_obj, data->soap, data->address, "_licon.conf", "/tmp/conf/licon.conf", "0666");
    if(retval < 0){
	return -1;
    }
    
    retval = file_get(data->client_obj, data->soap, data->address, "_contdaem.conf", "/tmp/conf/contdaem.conf", "0666");
    if(retval < 0){
	return -1;
    }

    return 0;
}

int dbcmd_jffsupd(CMD_HND_PARAM)
{
    struct cmdhnd_data *data = (struct cmdhnd_data *)sessdata;
    char *upddir = "/jffs2/update";
    char *destdir = "/jffs2";
    char *fallbackdir = "/jffs2/fallback";
    char *localfile = "/jffs2/update/upd.tgz";
    char *remotefile = cmd->value;
    char *upd_sh  =  "do_update.sh";
    int retval = 0;

    if(!cmd->value){
	*retstr = strdup("error paramater");
	return CMDSTA_EXECERR;
    }

    rpclient_db_getconfs(data, "/tmp/conf");

    /* make update dir*/
    retval = boxcmd_run_system("/bin/mkdir -p %s",upddir);
    if(retval != 0){
	*retstr = dupsprintf("error creating dir \"%s\": %d",upddir, retval);
	return CMDSTA_EXECERR;
    }

    /* get file */
    retval = file_get(data->client_obj, data->soap, data->address, remotefile, localfile, "0666");
    if(retval < 0){
	*retstr = dupsprintf("error writing file: %s", strerror(-retval));
	return CMDSTA_EXECERR;
    }  

    /* unpack update tar */
    retval = boxcmd_run_system("/bin/tar xzvf %s -C %s",localfile, upddir);
    if(retval != 0){
	*retstr = dupsprintf("error unpacking file \"%s\": %d",localfile, retval);
	return CMDSTA_EXECERR;
    }

    /* remove update tar */
    if(unlink(localfile))
	fprintf(stderr, "Error deleting \"%s\": %s\n",localfile, strerror(errno)); 
    
    /* execute update */
    retval = boxcmd_run_system("%s/%s \"%s\" \"%s\" \"%s\" ",upddir, upd_sh, destdir, fallbackdir , upddir);

    if(retval == 126){
	*retstr = dupsprintf("restarting");
	boxcmd_run_system("/bin/cp %s/* %s", "/tmp/conf", "/jffs2");
	data->reboot  = RETURN_DO_REBOOT;
    } else if(retval < 0){
	*retstr = dupsprintf("error executing %s (%d)", retval);
	return CMDSTA_EXECERR;
    } else {
	*retstr = dupsprintf("retval %d", retval);
    }     


    return CMDSTA_EXECUTED;
}



int dbcmd_confset(CMD_HND_PARAM)
{
    struct cmdhnd_data *data = (struct cmdhnd_data *)sessdata;
    char *process = NULL;
    char *conffile = NULL;
    char *upddir = "/jffs2/update";
    char localfile[64];
    char remotefile[64];
    int retval = 0;

    if(!cmd->value){
	*retstr = strdup("error paramater 1");
	return CMDSTA_EXECERR;
    }
    
    process = strdup(cmd->value);
    conffile = strchr(process, ',');
    if(!conffile){
	*retstr = strdup("error paramater 2");
	free(process);
	return CMDSTA_EXECERR;	
    }
    
    conffile[0] = '\0';
    conffile++;

    sprintf(localfile , "%s/%s", upddir, conffile);
    sprintf(remotefile , "_%s",  conffile);
    
    /* make update dir*/
    retval = boxcmd_run_system("/bin/mkdir -p %s",upddir);
    if(retval != 0){
	*retstr = dupsprintf("error creating dir \"%s\": %d",upddir, retval);
	return CMDSTA_EXECERR;
    }


    /* get file */
    retval = file_get(data->client_obj, data->soap, data->address, remotefile, localfile, "0666");
    if(retval < 0){
	*retstr = dupsprintf("error writing file: %s", strerror(-retval));
	free(process);
	return CMDSTA_EXECERR;
    }   
    

    retval = boxcmd_run_system("/usr/bin/chconfig.sh \"%s\" \"%s\" \"%s\" ", process, conffile, upddir);

    if(retval != 0){
	free(process);
	*retstr = dupsprintf("error in script : %d", retval);
	return CMDSTA_EXECERR;
    }
    
    return CMDSTA_EXECUTED;

}


int dbcmd_reboot(CMD_HND_PARAM)
{
    struct cmdhnd_data *data = (struct cmdhnd_data *)sessdata;

    /* get file */
    if(strcmp(cmd->value, "magic number")!=0)
	return CMDSTA_EXECERR;

    /* execute update */

    data->reboot  = RETURN_DO_REBOOT;

    return CMDSTA_EXECUTED;
}



int dbcmd_cmdnum(CMD_HND_PARAM)
{
    int value = 0;
    const char *format = (const char *)userdata;

    if(sscanf(cmd->value, "%d", &value )!=1){
		*retstr = dupsprintf("error param");
		return CMDSTA_EXECERR;
    }
	
	
	int retval = boxcmd_run_system(format, value);

    *retstr = dupsprintf("retval = %d", retval);
    return CMDSTA_EXECUTED;
}


int dbcmd_cmdstr(CMD_HND_PARAM)
{
	const char *format = (const char *)userdata;
	char value[64];

	if(sscanf(cmd->value, "%[a-zA-Z0-9_*/-.]", value )!=1){
		*retstr = dupsprintf("error param");
		return CMDSTA_EXECERR;
    }

	int retval = boxcmd_run_system(format, value);

	*retstr = dupsprintf("retval = %d", retval);
    return CMDSTA_EXECUTED;

}

int dbcmd_cmd(CMD_HND_PARAM)
{
	struct cmdhnd_data *data = (struct cmdhnd_data *)sessdata;
	const char *format = (const char *)userdata;

	int retval = boxcmd_run_system(format);

	*retstr = dupsprintf("retval = %d", retval);

	if(strcmp(cmd->value, "reboot")==0){
		data->reboot  = RETURN_DO_REBOOT;
	}

    return CMDSTA_EXECUTED;
	

}



struct cmddb_desc *cmd_desc_list = NULL;

int rpclient_cmd_init(const char *db_path, int debug_level)
{
    
	cmd_desc_list = cmddb_desc_add(cmd_desc_list,  cmddb_desc_create("setfile", dbcmd_setfile, NULL, CMDFLA_ACTION));
    cmd_desc_list = cmddb_desc_add(cmd_desc_list,  cmddb_desc_create("getfile", dbcmd_getfile, NULL, CMDFLA_ACTION));
    cmd_desc_list = cmddb_desc_add(cmd_desc_list,  cmddb_desc_create("jffsupd", dbcmd_jffsupd, NULL, CMDFLA_ACTION));
    cmd_desc_list = cmddb_desc_add(cmd_desc_list,  cmddb_desc_create("confset", dbcmd_confset, NULL, CMDFLA_ACTION));
    cmd_desc_list = cmddb_desc_add(cmd_desc_list,  cmddb_desc_create("reboot" , dbcmd_reboot , NULL, CMDFLA_ACTION));

    cmd_desc_list = cmddb_desc_add(cmd_desc_list,  cmddb_desc_create("liabtunnel", dbcmd_cmdnum, 
																	 "/usr/bin/liconutil -a \"ppp tunnel\" enabled=%d" , 
																	 CMDFLA_ACTION));

	cmd_desc_list = cmddb_desc_add(cmd_desc_list,  cmddb_desc_create("logdbetypdel", dbcmd_cmdstr, 
																	 "/usr/bin/logdbutil -R %s" , 
																	 CMDFLA_ACTION));

	cmd_desc_list = cmddb_desc_add(cmd_desc_list,  cmddb_desc_create("logdbdel", dbcmd_cmd, 
																	 "rm /jffs2/bigdb.sql" , 
																	 CMDFLA_ACTION));


    cmddb_db_open(db_path, debug_level);
    
    return 0;
    
}


int rpclient_cmd_deinit(void)
{
     cmddb_db_close();
     
     cmddb_desc_delete(cmd_desc_list);
     cmd_desc_list = NULL;


     return 0;
}


int rpclient_cmd_run(struct cmdhnd_data *data)
{
    
    return cmddb_exec_list(cmd_desc_list, (void*)data , time(NULL));

}
