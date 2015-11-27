/*
 * Copyright (C) 2014 - 2015 LIAB ApS <info@liab.dk>
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

/** Macro to test whether system_timeout returned an error */
#define SYSTEM_TIMEOUT_ERROR(status) ((status & 0xff0000) > 0)

enum {
	SYSTEM_TIMEOUT_FORK_FAILED = 1<<16,
	SYSTEM_TIMEOUT_TIMER_CREATE_FAILED = 2<<16,
	SYSTEM_TIMEOUT_TIMER_SET_FAILED = 3<<16
};

int system_timeout(const char *cmd, long long int *timeout);
