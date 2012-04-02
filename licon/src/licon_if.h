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

#ifndef LICON_IF_H_
#define LICON_IF_H_

#include <sys/time.h>
#include <pthread.h>


enum iftype{
    /**
     * Ethernet */
    IFTYPE_ETH,
    /**
     * ppp connection */
    IFTYPE_PPP,
};

enum ifstate{
    /**
     * Initial state (only at application start) */
    IFSTATE_INIT,
    /**
     * Error state */
    IFSTATE_ERR,
    /**
     * Down state (no link) */
    IFSTATE_DOWN,
    /**
     * Wait state (Deactivated for a moment)*/
    IFSTATE_WAIT,
    /**
     * Ip state (possible to connect) */
    IFSTATE_UP,
    /**
     * Try connecting state (if is trying to connect) */
    IFSTATE_TRYCONN,
    /**
     * Connected (if is connected) */
    IFSTATE_CONN,
};

struct licon_if_ppp{
    int unit;
    char *peer;
    char *apn;
};


struct licon_if_eth{
    int dhcp;
    char *ip;
    char *mask;
    char *gateway;
    char *dns;
};

struct licon_if{
    /**
     * Interface type */
    enum iftype type;
    /**
     * Name of the interface */
    char  *name;

    union{
	struct licon_if_eth eth;
	struct licon_if_ppp ppp;
    };

    /**
     * Command to run before connection 
     * Ex. turn on modem */
    char  *precmd;
    /**
     * Command to run after disconnecting 
     * Ex. turn off modem */
    char  *postcmd;
    /**
     * Command to repair a connection 
     * Ex. reset modem */
    char  *repcmd;
    /**
     * The path to the pid file */
    const char  *pidfile;
    /**
     * Next check time */
    time_t chk_next;
    /**
     * Interface state */
    enum ifstate state;
    int fail;
    int maxfail;
    int index;
    struct licon_waittime *waittime;
    /**
     * Always on flag */
    int alwayson;
    /**
     * If is on */
    int is_on;
    /**
     * Mutex */
    pthread_mutex_t mutex;
    /**
     * Next interface in list */
    struct licon_if *next;
};



/**
 * Get the an interface state string 
 */
const char *licon_if_state_str(enum ifstate state);


int licon_if_set_state(struct licon_if * if_obj, int status, int timeout);
int licon_if_get_state(struct licon_if * if_obj);

struct licon_if *licon_if_create(const char* name, const char *mode, const char *param1);
struct licon_if *licon_if_create_str(const char* if_str);
void licon_if_print(struct licon_if * interface);
struct licon_if *licon_if_add(struct licon_if * list, struct licon_if * new);

/**
 * Get a pointer to an interface object 
 * @param list The interface list
 * @param name The name to search for
 * @return a pointer to the interface object or NULL if not found */
struct licon_if *licon_if_get(struct licon_if * list, const char* name);

/**
 * Print status for an interface into a buffer
 * @param if_obj The interface object
 * @param buf The output buffer
 * @param maxlen The length of the buffer
 * @return The length of the printed text or negative if error */
int licon_if_status(struct licon_if * if_obj, char* buf, int maxlen);

int licon_if_chk(struct licon_if* if_obj);

int licon_eth_link_sta(const char *ifname);

int licon_eth_connect(struct licon_if* if_obj);
int licon_eth_down(struct licon_if* if_obj);

int licon_if_alwayson(struct licon_if* if_obj, int seton);

int licon_if_connect(struct licon_if* if_st, int dbglev);
int licon_if_disconnect(struct licon_if* if_obj);


#endif /*LICON_IF_H_*/
