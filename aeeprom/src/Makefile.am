lib_LTLIBRARIES  = libaeeprom.la
libaeeprom_la_SOURCES = aeeprom.c

bin_PROGRAMS     = aeeprom_util
aeeprom_util_SOURCES = aeeprom_util.c
aeeprom_util_LDADD = -laeeprom
aeeprom_util_DEPENDENCIES = libaeeprom.la

include_HEADERS = aeeprom.h
