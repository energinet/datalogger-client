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

#ifndef LICONNECT_H_
#define LICONNECT_H_

#include "licon_check.h"


#define DEFAULT_PORT 7414
#define UNIX_SOCKET  "/tmp/licon"


enum{
    DETACH_NONE,
    DETACH_UP,
    DETACH_NOW,
};

enum licon_status{
    LICON_IF_DOWN,
    LICON_IF_ERROR,
    LICON_IF_UP,  
    LICON_APP_ERROR,
    LICON_APP_START,
    LICON_APP_RUN,
};




struct licon{
//    char *if_list;
    struct licon_if *if_lst;
    struct licon_app *app_lst;
    char *t_user;
    char *t_passwd;
    char *cmd_up;
    char *cmd_down;
    struct licon_check *net_check;
    int chk_intv;
    int apperrcnt;
    struct licon_if *conn_if;
    struct licon_app *conn_tun;
    int sck_port;
    int d_level; 
};




#endif /* LICONNECT_H_ */
