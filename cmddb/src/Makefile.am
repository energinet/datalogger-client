

plugin_LTLIBRARIES  = modcmddb.la


modcmddb_la_SOURCES = modcmddb.c
modcmddb_la_LDFLAGS = -module -avoid-version
modcmddb_la_LIBADD = -lcmddb
modcmddb_la_DEPENDENCIES = libcmddb.la

bin_PROGRAMS     = cmddbutil
cmddbutil_SOURCES = cmddbutil.c
cmddbutil_LDADD = -lcmddb
cmddbutil_DEPENDENCIES = libcmddb.la

lib_LTLIBRARIES  = libcmddb.la
libcmddb_la_SOURCES = cmddb.c



include_HEADERS = cmddb.h
