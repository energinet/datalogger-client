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

#ifndef XML_CNTN_GEN_H_
#define XML_CNTN_GEN_H_

#include <sys/syscall.h>


#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s[%ld]: "str"\n",__FUNCTION__,(long int)syscall(__NR_gettid) , ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s[%ld]: "str"\n",__FUNCTION__,(long int)syscall(__NR_gettid), ## arg);}}



#endif /* XML_CNTN_GEN_H_ */
