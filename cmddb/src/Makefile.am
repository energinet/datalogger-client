plugin_LTLIBRARIES  = modcmddb.la

modcmddb_la_SOURCES = modcmddb.c
modcmddb_la_LDFLAGS = -module -avoid-version
modcmddb_la_LIBADD = -lcmddb

bin_PROGRAMS     = cmddbutil
cmddbutil_SOURCES = cmddbutil.c
cmddbutil_LDADD = -lcmddb

lib_LTLIBRARIES  = libcmddb.la
libcmddb_la_SOURCES = cmddb.c

include_HEADERS = cmddb.h
