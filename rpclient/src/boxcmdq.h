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

#ifndef BOXCMDQ_H_
#define BOXCMDQ_H_
#include <time.h>
#include <sys/time.h>

#define DEFAULT_CMD_LIST_PATH "/jffs2/cmdlist.sqlite"

struct boxvcmd{
    int cid;
    char *name;
    char *value;
    int status;
    struct timeval stime;
    char *retval;
    struct boxvcmd *next;
};

/**
 * Delete boxvcmd
 */
void boxvcmd_delete(struct boxvcmd *cmd);


/**
 * Open the command/update list 
 */
int boxvcmd_open(void);


/**
 * Close the command/update list
 */
void boxvcmd_close(void);


/**
 * Set command into the command list/queue
 */
int boxvcmd_queue(int cid, const char *name, const char *value, struct timeval *ttime, struct timeval *stime, int pseq);


/**
 * Remove command from the list 
 */
int boxvcmd_remove(int cid);

/**
 * Change status of a specific command 
 */
int boxvcmd_status_set(int cid, int status, struct timeval *stime, char* retval);

int boxvcmd_set_sent(int cid, int status, int keep);
/**
 * Get the status of a specific command 
 */
int boxvcmd_status_get(int cid, time_t *stime);


/**
 * Print the command list for debug purpose
 */
int boxcmdq_list_print(void);


/**
 * Read the current value/paramater of a variable/command
 */
char* boxcmdq_value_last(const char *name, struct timeval *time);
unsigned long boxcmdq_value_updtime(const char *name, struct timeval *time);

struct boxvcmd *boxvcmd_get_exec(struct timeval *time, unsigned long flags );


/**
 * Get all updated status' 
 * All obsolete command/value updates will be removed
 * @param last the time the update was send 
 * @retuen a list if commands/updates 
 */
struct boxvcmd *boxvcmd_get_upd(struct timeval *last);

#endif /* BOXCMDQ_H_ */
