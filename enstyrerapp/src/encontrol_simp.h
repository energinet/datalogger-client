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

#ifndef ENCONTROL_SIMP_H_
#define ENCONTROL_SIMP_H_

#include "enstyrerapp.h"

#include <cmddb.h>
#include <endata.h>
#include <logdb.h>

int encontrol_simp_calc(int inst_id, struct logdb *logdb, const char *calctype, int dbglev );

#endif /* ENCONTROL_SIMP_H_ */
