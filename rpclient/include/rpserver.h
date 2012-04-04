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

#ifndef	__SERVER_H
#define	__SERVER_H
//gsoap liabdatalogger service style: rpc
//gsoap liabdatalogger service name: liabdatalogger
//gsoap liabdatalogger service namespace: urn:liabdatalogger
//gsoap liabdatalogger service port: http://datalogger.styrdinvarmepumpe.dk/rpserver.cgi

/* Struct used for retreiving devices available */
 

typedef char *xsd__string;
typedef float xsd__float;
typedef int xsd__int;
typedef unsigned long xsd__ulong;


struct apiVersion{
  xsd__string description;
  xsd__int major;
  xsd__int minor;
  xsd__int sub;
};


struct rettext{
    xsd__string text;
    xsd__int retval;
};


struct connectionStatus{
    xsd__string if_name;
    xsd__string ip;
    unsigned long tx_byte;
    unsigned long rx_byte;
    int signal;
};

struct boxStatus{
    xsd__int    uptime;
};

struct boxInfo{
    xsd__string type;
    xsd__string name;
    xsd__ulong last_log_entry;
    struct connectionStatus conn;
    struct boxStatus        status;
    xsd__string rpversion;
    xsd__string localid;
//    $int sequenceListCnt;
//    int *sequenceList;
};

enum cmdsta{
    CMDSTA_QUEUED =1, /**< The command is set in queue */
    CMDSTA_SEND,      /**< The command has been send to box */
    CMDSTA_EXECUTING, /**< The command has been executed on box */
    CMDSTA_EXECUTED,  /**< The command has been executed on box */
    CMDSTA_DELETIG,   /**< The command is marked for deleting on box */
    CMDSTA_DELETED,   /**< The command has been deleted on box */
    CMDSTA_EXECDEL,   /**< The commend has been marked for 
			 deletion but has allready been executed on box */
    CMDSTA_EXECERR,   /**< An error while executing the command */
    CMDSTA_ERROBS,    /**< An newer command of the same type is executed */
    CMDSTA_UNKNOWN,   /**< An unknown state */
};

    /* CMDSTA_WATING   , /\**< The command is wating to be executed *\/ */


/**
 * Command to be executed on the remote box
 */
struct boxCmd{
    int sequence;   /**< The command sequence number */
    char *name;     /**< The command name */
    $int paramsCnt; /**< number of paramaters*/
    char **params;  /**< array of paramaters */
    char* trigtime; /**< The time when the command must execute */
    int pseq;       /**< */
    int status;     /**< The command status */
};

/**
 * 
 */
struct boxCmdUpdate{
    struct boxInfo boxInf;
    int sequence;   /**< The command sequence number */
    int status;     /**< The command status on the box */
    char *retval;   /**< The return value of the command (only on executed return) */
    char *timestamp;/**< Time of status change */ 
    struct boxCmd cmd;
};


/**
 * The return value from the server
 */
struct hostReturn{
    $int cmdCnt; /**< number of commands */
    struct boxCmd *cmds; /**< array of commands */
};


struct status{
  xsd__string @type;
  xsd__string name;
  xsd__string value;
};


/**
 * Event type meta data
 */
struct eTypeMeta{
    int eid;       /**< The eid on the box */
    char *ename;   /**< The event name */
    char *hname;   /**< The human readble description */
    char *unit;    /**< The unit */
    char *type;    /**< Type filed */
};


struct measMeta{
    struct boxInfo boxInf;
    $int etypeCnt;
    struct eTypeMeta *etypes;
};

struct eventItem{ 
    char* time; 
    char* value; 
}; 

struct eventSeries{ 
    int eid; 
    char *ename;
    char *time_start; 
    char *time_end;
    $int measCnt; 
    struct eventItem *meas; 
}; 


struct measurments{ 
    struct boxInfo boxInf; 
    $int seriesCnt;
    struct eventSeries *series; 
}; 

struct signalResponse { };




int liabdatalogger__getApiVersion(struct apiVersion *versionResp);
int liabdatalogger__statusTelegram(struct status *status, struct signalResponse *out);

/**
 * Send box information 
 */
int liabdatalogger__sendBoxInfo(struct boxInfo *boxInf, struct hostReturn *hostRet);

/**
 * Send measurments 
 */
int liabdatalogger__sendMeasurments(struct measurments* meas, struct hostReturn *hostRet); 


/**
 * Send information about the different event types 
 */
int liabdatalogger__sendMetadata(struct measMeta* metaData,  struct hostReturn *hostRet); 

/**
 * Send command status update 
 * @param update, command that has been updated
 */
int liabdatalogger__cmdUpdate(struct boxCmdUpdate *update, int* retval); 


struct filetransf{
    char *name;
    char *checksum;
    unsigned long file_mode;
    char *mineid;
}; 



int liabdatalogger__fileGet(char *filename, struct boxInfo *boxInf, struct filetransf *filetf);

//int liabdatalogger__fileConf(char *confname, struct boxInfo *boxInf, struct filetransf *filetf);


int liabdatalogger__fileSet(struct boxInfo *boxInf, struct filetransf *filetf,  int* retval);
//int liabdatalogger__fileSet(struct filetransf *file, int *retval);




int liabdatalogger__pair(struct boxInfo *boxInf, char *localid, int dopair, struct rettext *rettxt);



#endif /* __SERVER_H */
