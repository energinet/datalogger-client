
lib_LTLIBRARIES  = libxmlpar.la 

include_HEADERS = xml_parser.h xml_serialize.h xml_stack.h xml_doc.h xml_attr.h xml_item.h 

# General libs
libxmlpar_la_SOURCES = xml_parser.c xml_stack.c xml_doc.c xml_attr.c xml_item.c


bin_PROGRAMS     = xmltest
xmltest_SOURCES = xmltest.c
xmltest_LDADD = -lxmlpar
xmltest_DEPENDENCIES = libxmlpar.la

# clean backup files
CLEANFILES = *~ .ver $(VERSIONINC)

AM_CFLAGS = -Wall -Werror

