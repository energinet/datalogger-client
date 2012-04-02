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

#ifndef ENSTYRERAPP_H_
#define ENSTYRERAPP_H_

#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s: "str"\n",__FUNCTION__, ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}

#define DEFAULT_SCH_PLANNER "alwayson"
#define DEFAULT_CMD_CNTL    "pws_relay1"
#define DEFAULT_CMD_MDSCH   "enstyrerapp_mode"

#include <logdb.h>
#include <cmddb.h>
#include "timevar.h"

struct styrerapp{
	/**
	 * en installation ID	 */
	int inst_id; 
	/**
	 * Schedule type */
	char *schedule_type;
	/**
	 * cmdname for schedule mode */
	char *cmdname_modesch;
	/**
	 * control var name */
	char *cmdname_control;
    /**
     * cmd input var */
    struct timevar cmdtype;
    /**
     * cmd description list */
    struct cmddb_desc *descs;    
    /**
     * logdb object pointer */
    struct logdb *logdb;
    /**
     * Debug level */
    int dbglev;
};



#endif /* ENSTYRERAPP_H_ */

