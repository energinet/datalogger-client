/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2011 LIAB ApS <info@liab.dk>
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

#ifndef CMDDB_H_
#define CMDDB_H_
#include <time.h>

#define DEFAULT_CMD_LIST_PATH "/jffs2/cmdlist.db"
#define CIDAUTO 0

#define CMDFLA_KEEPCURR   1 /**< Keep current value in db */
#define CMDFLA_NODELAY    2 /**< The commans/value is not delayable */
#define CMDFLA_SETTING    CMDFLA_KEEPCURR  /**< Keep current value in db */
#define CMDFLA_ACTION     0 /**< Keep current value in db */
#define CMDFLA_CONNECTED  4 /**< The operation is done connected */ 

#ifndef SOAP_TYPE_cmdsta
enum cmdsta {
	CMDSTA_QUEUED = 1, /**< The command is set in queue */
	CMDSTA_SEND, /**< The command has been send to box */
	CMDSTA_EXECUTING, /**< The command has been executed on box */
	CMDSTA_EXECUTED, /**< The command has been executed on box */
	CMDSTA_DELETIG, /**< The command is marked for deleting on box */
	CMDSTA_DELETED, /**< The command has been deleted on box */
	CMDSTA_EXECDEL, /**< The commend has been marked for
	 deletion but has allready been executed on box */
	CMDSTA_EXECERR, /**< An error while executing the command */
	CMDSTA_ERROBS, /**< An newer command of the same type is executed */
	CMDSTA_UNKNOWN,
/**< An unknown state */
};
#endif

struct cmddb_cmd {
	int cid;
	char *name;
	char *value;
	int status;
	time_t stime;
	char *retval;
	time_t ttime;
	struct cmddb_cmd *next;
};

#define CMD_HND_PARAM struct cmddb_cmd* cmd, void* userdata, void* sessdata, char** retstr

struct cmddb_desc {
	char *name;
	unsigned long flags;
	int (*handler)(CMD_HND_PARAM);
	void* userdata;
	struct cmddb_desc *next;
};

struct cmddb_cmd *cmddb_cmd_create(int cid, const char *name,
		const char* value, int status, time_t stime, time_t ttime,
		const char *retval);

void cmddb_cmd_delete(struct cmddb_cmd *cmd);

struct cmddb_desc *cmddb_desc_create(const char *name, int(*handler)(
		CMD_HND_PARAM), void* userdata, unsigned long flags);

struct cmddb_desc *cmddb_desc_add(struct cmddb_desc *list,
		struct cmddb_desc *new);

void cmddb_desc_delete(struct cmddb_desc *desc);

/**
 * Open command database
 */
int cmddb_db_open(const char *path, int dbglev);

/**
 * Close command database
 */
int cmddb_db_close(void);

/**
 * Print all command in the database
 */
int cmddb_db_print(void);

/**
 * Insert a command into the db
 * @param cid   : command id
 * @param name  : command name
 * @param value : command param/set value
 * @param ttime : command trigger time
 * @param pseq  : previous command in a sequence 
 * @param now   : time (now or a point in time)
 */

int cmddb_insert(int cid, const char *name, const char *value, time_t ttime,
		int pseq, time_t now);

/**
 * Remove a command from the list
 */
int cmddb_remove(int cid);

/**
 * Mark a command for removal 
 */
int cmddb_mark_remove(int cid);

/**
 * Set command status 
 * @param cid    : command id
 * @param status : new status 
 * @param now    : time (now or a point in time)
 * @param retval : pointer to the return value text to write
 */
int cmddb_status_set(int cid, const char *col, int status, char* retval,
		int remsent, time_t now);

/**
 * Get the status for a given command id
 * @param cid    : command id
 * @param now    : time (now or a point in time)
 */
int cmddb_status_get(int cid);

/**
 * Get commands to execute
 * @param list : list of enabled commands 
 * @param now  : time (now or a point in time)
 */
int cmddb_exec_list(struct cmddb_desc *list, void *sessdata, time_t now);

time_t cmddb_value(const char *name, char *buf, size_t maxsize, time_t now);

time_t cmddb_time_execed(const char *name, time_t now);

int cmddb_mark_prev(char *name, time_t ttime);
int cmddb_sent_mark(int cid, int status);

int cmddb_sent_clean(void);

int cmddb_notsent_clean(void);

struct cmddb_cmd *cmddb_get_updates(void);

int cmd_cid_change(int cid_old, int cid_new);

#endif /* CMDDB_H_ */
