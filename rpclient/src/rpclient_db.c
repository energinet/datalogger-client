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

//#include <linux/in.h>
#include <stdio.h>
#include <stdarg.h>

//#include "boxcmdq.h"
#include "rpserver.h"
#include "rpclient_cmd.h"
#include "rpclient_stfile.h"
#include "rpfileutil.h"
#include "rpclient_soap.h"
#include <cmddb.h>
#include "version.h"

enum iftypes {IF_IP, IF_NETMASK, IF_BROADCAST, IF_GATEWAY};


char *platform_user_get(void);
float platform_get_uptime(void);
char* date_to_str(time_t ttime);
int get_iface_info(int type, const char *ifname, char *result, int len);


void rpcutil_reboot(void){
    char *cmd = "reboot -f";
    fprintf(stderr, "rpclient rebooting\n");
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
  

int cmd_send_status(struct rpclient *client_obj, struct soap *soap, char *address) 
{
     struct timeval last;
     struct cmddb_cmd *stacmd = NULL, *ptr= NULL;
     
     memset(&last, 0 , sizeof(last));
     //stacmd = boxvcmd_get_upd(&last);

     stacmd =  cmddb_get_updates();
     ptr = stacmd;
     printf("status first %p\n", ptr);
     while(ptr){
	 printf("status cid %d, status %d\n",  ptr->cid, ptr->status);
	 boxcmd_sent_status(client_obj, soap, address, ptr->cid, ptr->status, NULL, ptr->retval, ptr);
	 printf(">>22\n");
	 cmddb_sent_mark(ptr->cid, ptr->status);
//	 boxvcmd_set_sent(ptr->cid, ptr->status, boxcmd_get_flags(ptr->name)&CMDFLA_KEEPCURR);
	 printf(">>22\n");
	 ptr = ptr->next;
     }
     printf(">>23\n");
     
     cmddb_sent_clean();

     printf(">>24\n");
     cmddb_cmd_delete(stacmd);
     
     return 0;
}




int cmd_run(struct rpclient *client_obj, struct soap *soap, char *address, time_t time)
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


int file_set(struct rpclient *client_obj, struct soap *soap, char *address,  
			 char *serverfile,char *filepath)
{
	struct boxInfo boxinfo;
	struct filetransf filetf;
	mode_t mode = 0666;
	char md5sum[64]= "N/A";
	size_t size  = 0;
	char *data= NULL;
	char fileid[64];
	int retval;
	rpclient_stfile_write("fileSet...");
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
		fprintf(stderr, "error...\n");
		rpclient_stfile_write("fileSet fault: set mine");
		return soap->error;
	}
	fprintf( stderr, "before gsoap\n");

    struct rpclient_soap *rpsoap;
    rpsoap = client_obj->rpsoap;
    soap->userid = rpsoap->username;
    soap->passwd = rpsoap->password;

	if((retval = soap_call_liabdatalogger__fileSet(soap, address, NULL,  &boxinfo, &filetf, &retval))!= SOAP_OK){
		char errtxt[1014];
		fprintf(stderr, "Error in file_set\n");
		soap_print_fault(soap, stderr);
		soap_sprint_fault(soap, errtxt, sizeof(errtxt));
		rpclient_stfile_write("fileSet fault: fileSet");
	}
	fprintf( stderr, "after gsoap\n");
	soap_clr_mime(soap); // don't want fault with attachments
	free(data);
	fprintf( stderr, "before free\n");
	return 0;
}

int file_get(struct rpclient *client_obj, struct soap *soap, char *address, char *filename, const char *destpath, const char *modestr){

    int retval = 0;
    int err;
    struct filetransf filetf;
    struct boxInfo boxinfo;

	rpclient_stfile_write("fileGet...");
	
    rp_boxinfo_set(client_obj, &boxinfo);

    fprintf(stderr, "runing get file %s ,%s\n",filename, destpath);

    struct rpclient_soap *rpsoap;
    rpsoap = client_obj->rpsoap;
    soap->userid = rpsoap->username;
    soap->passwd = rpsoap->password;

    if((err = soap_call_liabdatalogger__fileGet(soap, address, NULL, filename, &boxinfo, &filetf))== SOAP_OK){
		fprintf(stderr,"Received file (%d) id: %s\n", retval, filetf.mineid);
        retval = 0;
		struct soap_multipart *attachment;
		
		for (attachment = soap->mime.list; attachment; attachment = attachment->next){
			printf("MIME attachment:\n");
			printf("Memory=%p\n", (*attachment).ptr);
			printf("Size=%ul\n", (*attachment).size);
			printf("Encoding=%d\n", (int)(*attachment).encoding);
			printf("Type=%s\n", (*attachment).type?(*attachment).type:"null");
			printf("ID=%s\n", (*attachment).id?(*attachment).id:"null");
			printf("Location=%s\n", (*attachment).location?(*attachment).location:"null");
			printf("Description=%s\n", (*attachment).description?(*attachment).description:"null");
	    
			if(strcmp(attachment->id, filetf.mineid)==0){
				char md5sum[32];
				mode_t mode = (mode_t)filetf.file_mode;
				
				if(modestr){
					retval = sscanf(modestr, "%o", &mode);
					fprintf(stderr, "scanf %s, result %d %o\n", modestr, retval, mode);
				}
				
				rpfile_md5( attachment->ptr, attachment->size, md5sum);
				
				fprintf(stderr, "(test)md5local %s md5remote %s\n", md5sum, filetf.checksum);
				
				
				if(rpfile_md5chk(attachment->ptr, attachment->size, filetf.checksum)!=0){
					fprintf(stderr, "sum FAULT\n");
					rpclient_stfile_write("fileGet crc fault");
					retval = -EILSEQ;
				} else {
					fprintf(stderr, "sum OK\n");
					rpclient_stfile_write("fileGet crc ok");
					rpfile_write(destpath, attachment->ptr, attachment->size, mode);
				}
			}
			
		} 
    } else {
		char errtxt[1024];
		soap_sprint_fault(soap, errtxt, sizeof(errtxt));
		rpclient_stfile_write("fileGet fault: %s", errtxt);
        fprintf(stderr,"fileGet fault: %s\n", errtxt );
		retval = -EBADRQC;
    }
    
    return retval;
}

int hostret_handle(struct rpclient *client_obj, struct soap *soap, char *address, struct hostReturn *hostRet)
{
    int i, retval;

    time_t stime = time(NULL);
   
    for(i = 0; i < hostRet->cmdCnt; i++){
		struct timeval ttime;
        struct boxCmd *cmd = hostRet->cmds + i;

		rpcutil_time_from_str(&ttime, cmd->trigtime);
		
		boxcmd_print(cmd);
		
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

    return 0;
}





int rp_boxinfo_set(struct rpclient *client_obj,struct boxInfo *boxinfo)
{
    char result[20];
    if(!get_iface_info(IF_IP, "eth0", result, sizeof(result)))
        strcpy(result, "unknown");

    boxinfo->type     = strdup("test");
    boxinfo->name     = platform_user_get();
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



int send_metadata(struct rpclient *client_obj,struct soap *soap,  char *address, struct logdb *db )
{
    static int update_sent = 0;

    struct measMeta metaData;
    struct boxInfo *boxinfo = &metaData.boxInf;
    struct hostReturn hostRet;
    int err;
    int i, retval;
    char etype_srv[32];
    char etype_cli[32];

    //  struct logdb *db = logdb_open(DEFAULT_DB_FILE, 15000, 0);
    sqlite3_stmt * ppStmt = NULL; //logdb_etype_list_prep(db);
    rpclient_stfile_write("sendMetadata checking");
    if(logdb_setting_get(db, "etype_srv_updated", etype_srv, 32)&&logdb_setting_get(db, "etype_updated",etype_cli, 32)){
        if(strcmp(etype_cli, etype_srv)==0){
            /* allready updated */
            printf("Etype is up to date\n");
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
    
    fprintf(stderr,"sendMetadata cnt %d %p...\n", metaData.etypeCnt, metaData.etypes);        

    for(i = 0; i < metaData.etypeCnt; i++){
        int eid;
        char *ename, *hname, *unit, *type;
        struct eTypeMeta *etype = metaData.etypes +i;
        retval = logdb_etype_list_next(ppStmt, &eid, &ename, &hname, 
                                       &unit, &type);
        if(retval != 1)
            break;
        
        //printf("test %d %i '%s' '%s' '%s' '%s' \n", i , eid, ename, hname, unit, type);
        
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

        //printf("test %d %i '%s' '%s' '%s' '%s' \n", i , etype->eid, etype->ename, etype->hname, etype->unit, etype->type);
    }


    logdb_list_end(ppStmt);  

    printf("ready to send\n");

    struct rpclient_soap *rpsoap;
    rpsoap = client_obj->rpsoap;
    soap->userid = rpsoap->username;
    soap->passwd = rpsoap->password;

    if((err = soap_call_liabdatalogger__sendMetadata(soap, address, NULL, &metaData, &hostRet))== SOAP_OK){
        hostret_handle(client_obj, soap,address,&hostRet);
        fprintf(stderr,"sendMetadata success\n");
        logdb_setting_set(db, "etype_srv_updated",  
                          logdb_setting_get(db, "etype_updated",etype_cli, 32)); 
        retval = 0;
		rpclient_stfile_write("sendMetadata success");
    } else {
		char errtxt[1024];
		soap_sprint_fault(soap, errtxt, sizeof(errtxt));
		rpclient_stfile_write("sendMetadata fault: %s", errtxt);
        fprintf(stderr,"sendMetadata fault: %s\n", errtxt );
		retval = -1;
    }

  out:
    
    fprintf(stderr, "closing db %d\n", retval);
    
    //   logdb_close(db);

    fprintf(stderr, "returned %d\n", retval);
    
    return retval;
 

}


int send_measurments(struct rpclient *client_obj,struct soap *soap,  char *address, struct logdb *db )
{
    struct measurments meas;
    struct boxInfo *boxinfo = &meas.boxInf;
    struct hostReturn hostRet;

    int err;
    int i, n, retval;
    sqlite3_stmt * ppStmt = logdb_etype_list_prep(db);
    
    rp_boxinfo_set(client_obj, boxinfo);
    printf("user %s\n", boxinfo->name);
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
        printf("reading %s (%d) count %d %s\n", ename, eid, eseries->measCnt, eseries->ename);
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
            printf(".");
            eseries->measCnt++;
        }
        printf("\n");
        if(etime)
            eseries->time_end   = date_to_str(etime);
        else
            eseries->time_end   = date_to_str(lastlog);

        logdb_list_end(ppStmtLog);

    }

    logdb_list_end(ppStmt);

    printf("ready to send\n");
    
    struct rpclient_soap *rpsoap;
    rpsoap = client_obj->rpsoap;
    soap->userid = rpsoap->username;
    soap->passwd = rpsoap->password;

    if((err = soap_call_liabdatalogger__sendMeasurments(soap, address, NULL, &meas, &hostRet))== SOAP_OK){
        hostret_handle(client_obj,soap,address,&hostRet);

        for(i = 0; i < meas.seriesCnt; i++){
            struct eventSeries *eseries =  meas.series + i;
            logdb_evt_mark_send(db, eseries->eid, eseries->time_end);
        }
        retval = 0;
		rpclient_stfile_write("sendMeasurments success");
		logdb_evt_maintain(db);
    } else {
		char errtxt[1024];
		soap_sprint_fault(soap, errtxt, sizeof(errtxt));
		rpclient_stfile_write("sendMeasurments fault: %s", errtxt);
        fprintf(stderr,"sendMeasurments fault: %s\n", errtxt );
        retval = -1;
    }
    
    measurments_free(&meas);
    //  logdb_close(db);

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
        fprintf(stderr,"%s(): %d: Error opening file '%s'\n", __FUNCTION__, __LINE__, "/tmp/optionsfile");
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

    fprintf(stderr, "error reading mac: sscanf returned %d\n", retval);

    return NULL;
}


void longtoip(char *ip, int len, unsigned long val) {
  snprintf(ip, len, "%d.%d.%d.%d", 
           (int)((val >> 0)&0xff),
           (int)((val >> 8)&0xff),
           (int)((val >> 16)&0xff),
           (int)((val >> 24)&0xff));
}



int get_iface_info(int type, const char *ifname, char *result, int len)
{
  int retval = 0;
  struct ifconf ifc; 
  struct ifreq ifreq,*ifr; 
  int sd;
  struct in_addr *ia;
      
  sd = socket(AF_INET, SOCK_STREAM, 0); 

  switch(type) {

    case IF_IP:
      {
        int n; 
      
        ifc.ifc_buf = NULL; 
        ifc.ifc_len = sizeof(struct ifreq) * 10; 
        ifc.ifc_buf = realloc(ifc.ifc_buf, ifc.ifc_len); 
        
        if (ioctl(sd, SIOCGIFCONF, &ifc) < 0) { 
          return retval;
        } 
        
        ifr = ifc.ifc_req; 
        for (n = 0; n < ifc.ifc_len; n += sizeof(struct ifreq)) { 
          ia= (struct in_addr *) ((ifr->ifr_ifru.ifru_addr.sa_data)+2); 
          if(strcmp(ifr->ifr_ifrn.ifrn_name, ifname) == 0) {
            if(result) {
              snprintf(result, len, "%s", (char *)inet_ntoa(*ia)); 
              retval = 1;
              break; 
            }
          } 
          ifr++; 
        }
        
        free(ifc.ifc_buf); 
      }
      break;

    case IF_NETMASK:
      strncpy(ifreq.ifr_name, ifname, sizeof(ifreq.ifr_name));
      if (ioctl(sd, SIOCGIFNETMASK, &ifreq) == 0) {
        ia= (struct in_addr *) ((ifreq.ifr_ifru.ifru_addr.sa_data)+2); 
        if(result) {
          snprintf(result, len, "%s", (char *)inet_ntoa(*ia)); 
          retval = 1;
          break;
        }
      }
      break;

    case IF_GATEWAY:
      {
        FILE *fp;
//        int bytes;
        int flags;
        unsigned long gw=0;

        if((fp = fopen("/proc/net/route", "r"))) {
          char line[1024];

          while( fgets(line, sizeof(line), fp) ) {
            if(strstr(line, ifname)) {
                sscanf(line, "%*s %*x %lx %d %*d %*d %*d %*x %*d %*d %*d",
                              &gw, &flags);
              if(flags & 0x2) {
                if(result)
                  longtoip(result, len, gw);
                retval = 1;
              }
            }
          }
          fclose(fp);
        }
      }
      break;
  }

  close(sd);

  return retval;
}
