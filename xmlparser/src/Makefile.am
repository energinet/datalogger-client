
lib_LTLIBRARIES  = libxmlpar.la 

include_HEADERS = xml_parser.h xml_serialize.h

# General libs
libxmlpar_la_SOURCES = xml_parser.c xml_serialize.c


bin_PROGRAMS     = xmltest
xmltest_SOURCES = xmltest.c
xmltest_LDADD = -lxmlpar
xmltest_DEPENDENCIES = libxmlpar.la

# clean backup files
CLEANFILES = *~ .ver $(VERSIONINC)

AM_CFLAGS = -Wall

