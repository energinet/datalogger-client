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

#ifndef AEEPROM_H_
#define AEEPROM_H_

#define DEFAULT_OFFSET (1024*16)
#define DEFAULT_EE_FILE "/sys/bus/i2c/devices/0-0050/eeprom"
//#define DEFAULT_DB_FILE "eeprom"

struct aeeprom {
	char *path;
	int offset;
};

char* aeeprom_get_entry(const char *name);
int aeeprom_set_entry(const char *name, const char *value);

char* aeeprom_dump(void);

#endif /* AEEPROM_H_ */

