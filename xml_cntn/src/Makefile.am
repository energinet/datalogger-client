
lib_LTLIBRARIES  = libxmlcntn.la
libxmlcntn_la_SOURCES = xml_cntn.c xml_cntn_sub.c xml_cntn_rcvr.c xml_cntn_subcl.c xml_cntn_trx.c xml_cntn_tx.c xml_cntn_fd.c xml_cntn_server.c xml_cntn_client.c
#libxmlcntn_la_LIBADD = -lrt


include_HEADERS = xml_cntn.h xml_cntn_sub.h xml_cntn_rcvr.h xml_cntn_tx.h xml_cntn_trx.h xml_cntn_fd.h xml_cntn_server.h xml_cntn_client.h

AM_LDFLAGS = -module -avoid-version  -lrt

AM_CFLAGS = -Wall -Werror
# -D_GNU_SOURCE 
# -rdynamic -funwind-tables
