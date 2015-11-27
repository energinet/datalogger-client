
bin_PROGRAMS = kamcomutil
lib_LTLIBRARIES  = libkamcom.la 

libkamcom_la_SOURCES = kamcom.c kamcom_cmd.c kamcom_types.c
libkamcom_la_LIBADD = -lm

kamcomutil_SOURCES = kamcomutil.c
kamcomutil_LDADD = -lkamcom -lm
kamcomutil_DEPENDENCIES = libkamcom.la version.h

#plugin_LTLIBRARIES  = kamcommod.la
#kamcommod_la_SOURCES = kamcommod.c 
#libkamcom_la_LIBADD = -lkamcom
#kamcommod_DEPENDENCIES = libkamcom.la

include_HEADERS = kamcom.h kamcom_cmd.h kamcom_types.h


AM_LDFLAGS = -module -avoid-version 
AM_LIBADD  = -lxmlpar -lmodule 
AM_CFLAGS = -Wall -Werror

VERSIONINC = version.h
BUILT_SOURCES = $(VERSIONINC)

# clean backup files
CLEANFILES = *~ .ver $(VERSIONINC)

version.h:
	echo '/* Auto-generated at build time. */' > $(VERSIONINC)
	echo "#define COMPILE_TIME \"`date '+%B %d, %Y at %H:%M'`\"" >> $(VERSIONINC)
	echo '#define COMPILE_BY "$(USERNAME)"'  >> $(VERSIONINC)
	if (test -n "$(SVNVERSION)" || test -f .svn); then \
		echo -n '#define BUILD "' >> $(VERSIONINC); \
		svnversion -n >> $(VERSIONINC); \
		echo '"' >> $(VERSIONINC); \
	else \
		echo '#define BUILD " "' >> $(VERSIONINC); \
	fi
	if ( test -z "$(host_alias)" ); then \
		echo "#define ARCH \"`uname -m`\"" >> $(VERSIONINC); \
	else \
	  echo "#define ARCH \"$(host_alias)\"" >> $(VERSIONINC); \
	fi
