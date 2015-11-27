


plugin_LTLIBRARIES  = kamcommod.la
kamcommod_la_SOURCES = kamcommod.c 


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
