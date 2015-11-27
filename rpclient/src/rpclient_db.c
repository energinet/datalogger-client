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

#include "rpclient_db.h"

#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <logdb.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <plugin/httpda.h>


//#include <linux/in.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>

//#include "boxcmdq.h"
#include "rpserver.h"
#include "rpclient_cmd.h"
#include "rpclient_stfile.h"
#include "rpclient_glob.h"
#include "rpfileutil.h"
#include "rpclient_soap.h"
#include <cmddb.h>
#include <assert.h>
#include "version.h"



char *device_name = NULL;


char *platform_user_get(void);
float platform_get_uptime(void);
char* date_to_str(time_t ttime);
int get_iface_info(int type, const char *ifname, char *result, int len);





void rpcutil_reboot(void){
    char *cmd = "reboot -f";
    syslog(LOG_WARNING , "rpclient rebooting\n");
    system(cmd);    
}



void rpcutil_time_from_str(struct timeval *dest, const char *timestr)
{
    dest->tv_sec = 0;
    sscanf(timestr, "%lu", &dest->tv_sec);
    dest->tv_usec = 0;
}
  
char *rpcutil_time_to_str(struct timeval *time)
{
    char buf[64];

    snprintf(buf, sizeof(buf), "%lu", time->tv_sec);

    return strdup(buf);
}
  

int cmd_send_status(struct rpclient *client_obj, struct soap *soap, const char *address) 
{
     struct timeval stime;
     struct cmddb_cmd *stacmd = NULL, *ptr= NULL;
     
     memset(&stime, 0 , sizeof(stime));

     stacmd =  cmddb_get_updates();
     ptr = stacmd;
     syslog(LOG_DEBUG, "status first cmd %p", ptr);
     while(ptr){
		 syslog(LOG_DEBUG,"status cmd: cid %d, status %d",  ptr->cid, ptr->status);
		 stime.tv_sec = ptr->stime;
		 boxcmd_sent_status(client_obj, soap, address, ptr->cid, ptr->status, &stime, ptr->retval, ptr);
		 cmddb_sent_mark(ptr->cid, ptr->status);
		 //	 boxvcmd_set_sent(ptr->cid, ptr->status, boxcmd_get_flags(ptr->name)&CMDFLA_KEEPCURR);
		 ptr = ptr->next;
     }
     
     cmddb_sent_clean();

     cmddb_cmd_delete(stacmd);
     
     syslog(LOG_DEBUG, "status cmd ended");
     
     return 0;
}




int cmd_run(struct rpclient *client_obj, struct soap *soap, const char *address, time_t time)
{

    struct cmdhnd_data data;

    memset(&data, 0 , sizeof(data));
    data.soap = soap;
    data.address = address;
    data.reboot  = 0;
    data.client_obj = client_obj;

    rpclient_cmd_run(&data);
   
    if(data.reboot == RETURN_DO_REBOOT)
	return data.reboot;

    return 0;

}


int file_set(struct rpclient *client_obj, struct soap *soap, const char *address,  
	     char *serverfile,char *filepath)
{
    struct boxInfo boxinfo;
    struct filetransf filetf;
    mode_t mode = 0666;
    char md5sum[64]= "N/A";
    size_t size  = 0;
    char *data= NULL;
    char fileid[64];
    int retval, err;
    rpclient_stfile_write("fileSet...");
    syslog(LOG_DEBUG, "running %s", __FUNCTION__);
    rp_boxinfo_set(client_obj, &boxinfo);

    sprintf(fileid, "<%s@datalogger.liab.dk>", serverfile);

    if(rpfile_read(filepath, &data , &size, &mode)<=0){ 
	return SOAP_ERR; 
    } 

    memset(&filetf, 0, sizeof(filetf));

    filetf.file_mode = mode; 
    rpfile_md5((unsigned char*)data, size, md5sum); 
    filetf.checksum  = strdup(md5sum);
    filetf.mineid    = strdup(fileid); 

    soap_set_mime(soap, NULL, NULL); // enable MIME
	
    if (soap_set_mime_attachment(soap, data, size, SOAP_MIME_BINARY, "application/x-gzip", fileid, filepath, serverfile)) {
	soap_clr_mime(soap); // don't want fault with attachments
	free(data);
	syslog(LOG_ERR, "ERROR sending mine attachment");
	rpclient_stfile_write("fileSet fault: set mine");
	return soap->error;
    }
    
    syslog(LOG_DEBUG, "before gsoap in %s", __FUNCTION__);

    struct rpclient_soap *rpsoap;
    rpsoap = client_obj->rpsoap;
    http_da_restore(soap, rpsoap->dainfo);
    int retries = 0;
    retval = -1;

    do{
	err = soap_call_liabdatalogger__fileSet(soap, address, NULL,  &boxinfo, &filetf, &retval);
	if(err == SOAP_OK){
	    break;
	}
    }while(rpclient_soap_hndlerr(rpsoap,soap, rpsoap->dainfo, retries++, __FUNCTION__));
   
    syslog(LOG_DEBUG, "after gsoap in %s", __FUNCTION__);

    soap_clr_mime(soap); // don't want fault with attachments
    free(data);

    return 0;
}

int file_get(struct rpclient *client_obj, struct soap *soap, const char *address, char *filename, const char *destpath, const char *modestr){

    int retval = 0;
    int err;
    struct filetransf filetf;
    struct boxInfo boxinfo;

    syslog(LOG_DEBUG, "running %s: filname '%s', destpath '%s'", __FUNCTION__, filename, destpath);
    rpclient_stfile_write("fileGet...");
	
    rp_boxinfo_set(client_obj, &boxinfo);

    struct rpclient_soap *rpsoap;
    rpsoap = client_obj->rpsoap;
    
    int retries = 0;
    retval = -EBADRQC;
    http_da_restore(soap, rpsoap->dainfo);
    
    do{
	err = soap_call_liabdatalogger__fileGet(soap, address, NULL, filename, &boxinfo, &filetf);
	if(err == SOAP_OK){
	    syslog(LOG_DEBUG,"Received file (%d) id: %s", retval, filetf.mineid);
	    retval = 0;
	    struct soap_multipart *attachment;
	    
	    for (attachment = soap->mime.list; attachment; attachment = attachment->next){
		syslog(LOG_DEBUG,"MIME attachment:");
		syslog(LOG_DEBUG,"Memory=%p", (*attachment).ptr);
		syslog(LOG_DEBUG,"Size=%ul", (*attachment).size);
		syslog(LOG_DEBUG,"Encoding=%d", (int)(*attachment).encoding);
		syslog(LOG_DEBUG,"Type=%s", (*attachment).type?(*attachment).type:"null");
		syslog(LOG_DEBUG,"ID=%s", (*attachment).id?(*attachment).id:"null");
		syslog(LOG_DEBUG,"Location=%s", (*attachment).location?(*attachment).location:"null");
		syslog(LOG_DEBUG,"Description=%s", (*attachment).description?(*attachment).description:"null");
		
		if(strcmp(attachment->id, filetf.mineid)==0){
		    char md5sum[32];
		    mode_t mode = (mode_t)filetf.file_mode;
		    
		    if(modestr){
			retval = sscanf(modestr, "%o", &mode);
			syslog(LOG_DEBUG, "scanf %s, result %d %o\n", modestr, retval, mode);
		    }
		    
		    rpfile_md5((unsigned char*) attachment->ptr, attachment->size, md5sum);
		    
		    syslog(LOG_DEBUG, "(test)md5local %s md5remote %s\n", md5sum, filetf.checksum);
		    
		    if(rpfile_md5chk((unsigned char*)attachment->ptr, attachment->size, filetf.checksum)!=0){
			syslog(LOG_ERR, "check sum FAULT in get file");
			rpclient_stfile_write("fileGet crc fault");
			retval = -EILSEQ;
		    } else {
			syslog(LOG_DEBUG, "check sum OK in get file\n");
			rpclient_stfile_write("fileGet crc ok");
			rpfile_write(destpath, attachment->ptr, attachment->size, mode);
			retval = 0;
		    }
		}
		
	    }
	    break;
	}
	
    }while(rpclient_soap_hndlerr(rpsoap,soap, rpsoap->dainfo, retries++, __FUNCTION__));

    return retval;
}

int hostret_handle(struct rpclient *client_obj, struct soap *soap, const char *address, struct hostReturn *hostRet)
{
    int i, retval;

    time_t stime = time(NULL);
   
    if(cmd_db_lock()){
	retval = -1;
	goto out;
    }

    for(i = 0; i < hostRet->cmdCnt; i++){
	struct timeval ttime;
        struct boxCmd *cmd = hostRet->cmds + i;

	rpcutil_time_from_str(&ttime, cmd->trigtime);
		
	boxcmd_print(LOG_DEBUG, cmd);
		
	if(cmd->status == CMDSTA_DELETIG)
	    retval = cmddb_mark_remove(cmd->sequence);
	else 
	    retval = cmddb_insert(cmd->sequence,  cmd->name, cmd->params[0], 
				  ttime.tv_sec, cmd->pseq, stime);
    }

    cmddb_db_print();

    retval = cmd_run(client_obj, soap, address, stime);

    cmd_send_status(client_obj, soap, address);

    if(retval == RETURN_DO_REBOOT)
	rpcutil_reboot();

    retval = 0;
	
 out:

    cmd_db_unlock();

    return retval;
}





int rp_boxinfo_set(struct rpclient *client_obj,struct boxInfo *boxinfo)
{
    char result[20];
    if(get_iface_info(IF_IP, "eth0", result, sizeof(result))<0)
        strcpy(result, "unknown");

    boxinfo->type     = strdup("test");
    boxinfo->name     = strdup(platform_get_name());
    if(client_obj->localid)
	boxinfo->localid    = strdup(client_obj->localid);
    else
	boxinfo->localid    = strdup("");
    boxinfo->rpversion = strdup(BUILD" "COMPILE_TIME );
    boxinfo->conn.if_name = strdup("eth0");
    boxinfo->conn.ip = strdup(result);
    boxinfo->conn.tx_byte = 0;
    boxinfo->conn.rx_byte = 0;
    boxinfo->conn.signal = 100;
    boxinfo->status.uptime = platform_get_uptime();

    return 0;
}


void rp_boxinfo_free(struct boxInfo *boxinfo)
{
    free(boxinfo->type);
    free(boxinfo->name);
    free(boxinfo->conn.if_name);
}


void measurments_free(struct measurments *meas)
{
    int i, n;
    rp_boxinfo_free(&meas->boxInf);

    for(i = 0; i < meas->seriesCnt; i++){
        struct eventSeries *series =  meas->series + i;
        
        free(series->ename);
        free(series->time_start);
        free(series->time_end);

        for(n = 0; n < series->measCnt; n++){
            struct eventItem *item = series->meas + n;
            free(item->time);
            free(item->value);
        }

        free(series->meas);
    }

    free(meas->series);
}


int send_metadata(struct rpclient *client_obj,struct soap *soap,  const char *address, struct logdb *db )
{
    static int update_sent = 0;

    struct measMeta metaData;
    struct boxInfo *boxinfo = &metaData.boxInf;
    struct hostReturn hostRet;
    int err;
    int i;
    int retval = -1;
    char etype_srv[32];
    char etype_cli[32];


    sqlite3_stmt * ppStmt = NULL; //logdb_etype_list_prep(db);
    rpclient_stfile_write("sendMetadata checking");
    if(logdb_setting_get(db, "etype_srv_updated", etype_srv, 32)&&logdb_setting_get(db, "etype_updated",etype_cli, 32)){
        if(strcmp(etype_cli, etype_srv)==0){
            /* allready updated */
            syslog(LOG_DEBUG,"Etype is up to date\n");
	    if(update_sent){
		retval = 0;
		goto out;
	    }
        }
    }
    
    rpclient_stfile_write("sendMetadata sending");

    update_sent++;
    
    ppStmt = logdb_etype_list_prep(db);

    rp_boxinfo_set(client_obj, boxinfo);

    
    metaData.etypeCnt = logdb_etype_count(db);
    metaData.etypes = malloc(sizeof(struct measMeta)* metaData.etypeCnt);
    
    syslog(LOG_DEBUG ,"sendMetadata cnt %d %p...\n", metaData.etypeCnt, metaData.etypes);        

    for(i = 0; i < metaData.etypeCnt; i++){
        int eid;
        char *ename, *hname, *unit, *type;
        struct eTypeMeta *etype = metaData.etypes +i;
        retval = logdb_etype_list_next(ppStmt, &eid, &ename, &hname, 
                                       &unit, &type);
        if(retval != 1)
            break;
        
        etype->eid = eid;
        etype->ename = strdup(ename);
        etype->hname = strdup(hname);

        if(unit)
            etype->unit = strdup(unit);
        else 
            etype->unit = strdup("");

        if(type)
            etype->type  = strdup(type);
        else
            etype->type = strdup("");
    }


    logdb_list_end(ppStmt);  

    syslog(LOG_DEBUG, "ready to send\n");

    struct rpclient_soap *rpsoap;
    rpsoap = client_obj->rpsoap;
    int retries = 0;
    retval = -1;

    http_da_restore(soap, rpsoap->dainfo);

    do{
	err = soap_call_liabdatalogger__sendMetadata(soap, address, NULL, &metaData, &hostRet);
	
	if(err == SOAP_OK){
	    syslog(LOG_DEBUG, "sendMetadata success\n");
	    hostret_handle(client_obj, soap,address,&hostRet);
	    logdb_setting_set(db, "etype_srv_updated",  logdb_setting_get(db, "etype_updated",etype_cli, 32)); 
	    retval = 0;
	    break;
	} 
    }while(rpclient_soap_hndlerr(rpsoap,soap, rpsoap->dainfo, retries++, __FUNCTION__));


  out:
    
    return retval;
 

}


int send_measurments(struct rpclient *client_obj,struct soap *soap,  const char *address, struct logdb *db )
{
    struct measurments meas;
    struct boxInfo *boxinfo = &meas.boxInf;
    struct hostReturn hostRet;

    int err;
    int i, n, retval;
    sqlite3_stmt * ppStmt = logdb_etype_list_prep(db);
    
    rp_boxinfo_set(client_obj, boxinfo);
    syslog(LOG_DEBUG, "%s: user %s\n", __FUNCTION__ , boxinfo->name);
    rpclient_stfile_write("sendMeasurments...");
    meas.seriesCnt = logdb_etype_count(db);
    meas.series = malloc(sizeof(struct eventSeries)*meas.seriesCnt);

    for(i = 0; i < meas.seriesCnt; i++){
        struct eventSeries *eseries =  meas.series + i;
        int eid, dummy;
        time_t lastlog, lastsend;
        char *ename, *hname, *unit, *type;
        sqlite3_stmt * ppStmtLog ;
        time_t etime = 0;
        retval = logdb_etype_list_next(ppStmt, &eid, &ename, &hname, 
                                       &unit, &type);
        if(retval != 1)
            break;

        ppStmtLog  = logdb_evt_list_prep_send(db, eid);
        eseries->eid = eid;
        logdb_evt_interval(db, eid, &lastlog, &lastsend);

        eseries->time_start = date_to_str(lastsend);
        eseries->ename = strdup(ename);

        eseries->measCnt = 0;//logdb_evt_count(db, eseries->eid);
        eseries->meas = malloc(sizeof(struct eventItem)*1000);
        syslog(LOG_DEBUG, "reading %s (%d) count %d %s\n", ename, eid, eseries->measCnt, eseries->ename);
        for(n = 0; n < 1000 ; n++){
            struct eventItem *eitem = eseries->meas +n;
            char etimestr[32];
            const char *eval;
           
            retval = logdb_evt_list_next(ppStmtLog, &dummy, &etime, &eval);
            if(retval != 1)
                break;
            sprintf(etimestr, "%ld", etime); 
            
            eitem->time = strdup(etimestr);
            eitem->value = strdup(eval);

            eseries->measCnt++;
        }

        if(etime)
            eseries->time_end   = date_to_str(etime);
        else
            eseries->time_end   = date_to_str(lastlog);

        logdb_list_end(ppStmtLog);

    }

    logdb_list_end(ppStmt);

    syslog(LOG_DEBUG, "ready to send\n");
    
    struct rpclient_soap *rpsoap;
    rpsoap = client_obj->rpsoap;
	//  soap->userid = rpsoap->username;
    //soap->passwd = rpsoap->password;

    int retries = 0;
    retval = -1;
    http_da_restore(soap, rpsoap->dainfo);

    do{
	err = soap_call_liabdatalogger__sendMeasurments(soap, address, NULL, &meas, &hostRet);
	if(err== SOAP_OK){
	    hostret_handle(client_obj,soap,address,&hostRet);
	    
	    for(i = 0; i < meas.seriesCnt; i++){
		struct eventSeries *eseries =  meas.series + i;
		logdb_evt_mark_send(db, eseries->eid, eseries->time_end);
	    }
	    retval = 0;
	    rpclient_stfile_write("sendMeasurments success");
	    logdb_evt_maintain(db);
	    break;
	}
    }while(rpclient_soap_hndlerr(rpsoap,soap, rpsoap->dainfo, retries++, __FUNCTION__));
    
    measurments_free(&meas);

    return retval;
}

int rpclient_db_localid(struct rpclient *client_obj,struct soap *soap,  const char *address,  const char *localid, int dopair, struct rettext *rettxt )
{
    int err;   
    struct boxInfo boxinfo;
    int retval = -1;

    rp_boxinfo_set(client_obj, &boxinfo);
    
    int retries = 0;
    struct rpclient_soap *rpsoap ;
    rpsoap = client_obj->rpsoap;

    http_da_restore(soap, rpsoap->dainfo);

    do{
	err = soap_call_liabdatalogger__pair(soap, address, NULL,&boxinfo, (char*)localid, dopair, rettxt);
	if(err==SOAP_OK){
	    retval = 0;
	    syslog(LOG_DEBUG, "socket_hndlr_localid_chk retval %d\n", rettxt->retval);
	    break;

	}
    }while(rpclient_soap_hndlerr(rpsoap,soap, rpsoap->dainfo, retries++, __FUNCTION__));
 
    return retval;

}



char* date_to_str(time_t ttime)
{
    char str[32];

    sprintf(str, "%ld", ttime);

    return strdup(str);
}




char *read_file(const char *path)
{

    return 0;
}

float platform_get_uptime(void)
{
    FILE *file = fopen("/proc/uptime", "r");
    float uptime;
    float idle;

    if(!file)
        return -1;

    if(fscanf(file, "%f %f", &uptime, &idle)!=2){
        fclose(file);
        return -2;
    }
    
    fclose(file);
    return uptime;
    
}





char *platform_conf_get(const char *name)
{
    FILE *file;
    char sstring[128];
    char value[128];
    char line[256];
    int linemax = 100;
    sprintf(sstring, "%s=%%s\\n", name);

    file = fopen("/tmp/optionsfile", "r");
    
    if(file == NULL) {
        syslog(LOG_ERR,"%s(): %d: Error opening file '%s'\n", __FUNCTION__, __LINE__, "/tmp/optionsfile");
        return NULL;
    }

    strcpy(value, "N/A");
    
    while(!feof(file)){

        if(!fgets(line, 256, file))
            break;

        if(sscanf(line, sstring, value)==1)
            break;      
        
        
        if(linemax<=0)
            break;

        linemax--;
    }

    fclose(file);

    return strdup(value);
}


char *platform_user_get(void)
{
    int mac[8];
    int retval;
    char *hwa = platform_conf_get("liabETH");
    char user[32];

    if(hwa == NULL) {
        sprintf(user, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", 0, 0, 0, 0, 0, 0);
        return strdup(user);
    }

    if((retval = sscanf(hwa, "%x:%x:%x:%x:%x:%x", 
                        &mac[0], &mac[1], &mac[2], 
                        &mac[3], &mac[4], &mac[5]))==6){
        free(hwa);
        sprintf(user, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return strdup(user);
    } 

    free(hwa);

    syslog(LOG_ERR, "error reading mac: sscanf returned %d\n", retval);

    return NULL;
}



const char *platform_get_name()
{
    if(!device_name){
	char mac[20];
	if(get_iface_info(IF_MAC, "eth0", mac, sizeof(mac))<0)
	    strcpy(mac, "unknown");
	device_name = strdup(mac);
    }    
    
    return device_name;   
}

void platform_set_name(const char*  name)
{
    if(device_name)
	free(device_name);
    
    device_name = strdup(name);
}


void longtoip(char *ip, int len, unsigned long val) {
  snprintf(ip, len, "%d.%d.%d.%d", 
           (int)((val >> 0)&0xff),
           (int)((val >> 8)&0xff),
           (int)((val >> 16)&0xff),
           (int)((val >> 24)&0xff));
}



int get_iface_info(int type, const char *ifname, char *result, int maxlen)
{
    int retval = 0;
    //, n;
    //struct ifconf ifc; 
    //  struct ifreq ifreq,ifr; 
  struct ifreq ifr; 
  int sock;
  //struct in_addr *ia;
  char *p;

  
  assert(ifname);
  assert(result);

  sock = socket(AF_INET, SOCK_STREAM, 0); 

  if(sock == -1){
      syslog(LOG_ERR, "error opening socket ifconfiguration %d\n", errno);
      return -1;
  }

  strncpy(ifr.ifr_name,ifname,sizeof(ifr.ifr_name)-1);
  ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';

  switch(type) {
  case IF_IP:    {
      if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) { 
	  syslog(LOG_ERR, "error retreving SIOCGIFADDR %d\n", errno);
          return -1;
      } 
      p=inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr);
      strncpy(result,p,maxlen-1);
      result[maxlen-1]='\0';
      break;   
  }
  case IF_NETMASK: {
      if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0) { 
	  syslog(LOG_ERR, "error retreving SIOCGIFNETMASK %d\n", errno);
          return -1;
      }
      p=inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_netmask))->sin_addr);
      strncpy(result,p,maxlen-1);
      result[maxlen-1]='\0';
      break;
  }
  case IF_MAC: {
      int len = 0, i;
      if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
	  syslog(LOG_ERR, "error retreving SIOCGIFHWADDR %d\n", errno);
          return -1;
      }
      for (i=0; i<6; i++) {
	  len+=snprintf(result+len, maxlen-len-1, "%02X",
		      (int)(unsigned int)(unsigned char)ifr.ifr_hwaddr.sa_data[i]);
      }
      result[maxlen-1]='\0';
      break;
  }
  case IF_GATEWAY: {
      FILE *fp;
      int flags;
      unsigned long gw=0;
      
      if((fp = fopen("/proc/net/route", "r"))) {
          char line[1024];
	  retval = -1;
          while( fgets(line, sizeof(line), fp) ) {
	      if(strstr(line, ifname)) {
		  sscanf(line, "%*s %*x %lx %d %*d %*d %*d %*x %*d %*d %*d",
			 &gw, &flags);
		  if(flags & 0x2) {
		      longtoip(result, maxlen, gw);
		      retval = 0;
		  }
	      }
          }
          fclose(fp);
      }
      break;
  }
  }
  
  close(sock);

  return retval;
}
