
lib_LTLIBRARIES  =  libcontlive.la
#lib_LTLIBRARIES  = libxmlcntn.la libcontlive.la
#libxmlcntn_la_SOURCES = xmlcntn.c
libcontlive_la_SOURCES = contlive.c

bin_PROGRAMS     = contdaemutil
contdaemutil_SOURCES = contdaemutil.c
contdaemutil_LDADD =  -lcontlive
#contdaemutil_LDADD = -lxmlcntn -lcontlive
contdaemutil_DEPENDENCIES = libcontlive.la
#contdaemutil_DEPENDENCIES = libxmlcntn.la libcontlive.la

plugin_LTLIBRARIES  = socketmod.la  xmlsocket.la  #xmlsckmaster.la # REMOVED UNTIL CONVERTED TO NEW LIB xmlsocket.la xmlsckmaster.la
socketmod_la_SOURCES = socketmod.c 
socketmod_la_DEPENDENCIES = libcontlive.la
xmlsocket_la_SOURCES = xmlsocket.c xmlcontdaem.c
#xmlsocket_la_LIBADD = -lxmlcntn
#xmlsocket_la_DEPENDENCIES = libxmlcntn.la

xmlsckmaster_la_SOURCES =  xmlsckmaster.c xmlcontdaem.c
#xmlsckmaster_la_LIBADD = -lxmlcntn
#xmlsckmaster_DEPENDENCIES = libxmlcntn.la

include_HEADERS = contlive.h

AM_LDFLAGS = -module -avoid-version -Wall -Werror
