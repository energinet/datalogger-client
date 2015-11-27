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

#include "module_base.h"
#include "module_value.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/input.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <pthread.h> 


/**
 * @addtogroup module_xml
 * @{
 * @section switchmodxml Switch module
 * Module for switching between values. \n
 * <b>Typename: "switch" </b>\n
 * <b> Tags: </b>
 * <ul>
 * <li><b>switch:</b> Create a switched value 
 * - <b> name:</b>  The output name\n
 * - <b> control:</b>  The control value 
 * <ul>
 * <li><b>case:</b> A case value
 * - <b> val:</b> The switch case value \n
 * - <b> out:</b> The output value in this case \n
 * - <b> default:</b> Set true if the case are the default case \n
 * </ul>
 * </ul>
 * Example from a setup:
 @verbatim 
<module type="switch" name="switch" verbose="false" flags="log" omode="change">	
    <switch name="settemp" control="cmdvars.sgready">
	<case val="1" out="cmdvars.settemp_1" default="true"/>
	<case val="2" out="cmdvars.settemp_2"/>
	<case val="3" out="cmdvars.settemp_3"/>
	<case val="4" out="cmdvars.settemp_4"/>
    </switch>	
  </module>
@endverbatim
 @}
*/

/**
 * Case object 
 */
struct mcase{
    /**
     * The case value */
    int val;
    /**
     * The case ouput value */
    struct evalue *out;
    /**
     * The next object in the list */
    struct mcase *next;
};

struct mswitch{
    /**
     * The swich input value */
    struct evalue *control;
    /**
     * The switch output value */
    struct evalue *out;
    /**
     * The next object in the list */
    struct mswitch *next;
};

struct mcase *mcase_create(struct module_base *base, const char **attr)
{

    
}


