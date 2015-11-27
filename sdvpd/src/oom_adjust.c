/*
 * Energinet Datalogger
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
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/*
HJP LIAB october 2014
Simple oom adjustment of the current PID.
Note that if not privileged, it is only possible to raise oom_adj, and the 
resulting value will be adj-1.
*/
void oom_adjust ( char adj ) {

	FILE *oom_adj;
	int ret;

	char buffer[sizeof("/proc/65535/oom_adj")];
	sprintf(buffer, "/proc/%u/oom_adj", getpid());

	oom_adj = fopen(buffer, "a");

	if (oom_adj == NULL) {
		return;
	}

	sprintf(buffer, "%d",adj);

	ret = fwrite(buffer, 1, strlen(buffer), oom_adj);

	fclose(oom_adj);

}
