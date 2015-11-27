VERSION = 1
PATCHLEVEL = 0
VERSIONINC = version.h

plugin_LTLIBRARIES  = heatupmod.la

heatupmod_la_SOURCES = heatupmod.c
heatupmod_la_LDFLAGS   = -module -avoid-version
heatupmod_la_LIBADD = -lmodbus

BUILT_SOURCES = version.h

# clean backup files
CLEANFILES = *~ .ver $(VERSIONINC)
AM_CFLAGS = -Wall -Wextra -Werror

version.h:
	echo '/* Auto-generated at build time. */' > $(VERSIONINC)
	echo "#define COMPILE_TIME \"`date '+%B %d, %Y at %H:%M'`\"" >> $(VERSIONINC)
	echo '#define COMPILE_BY "$(USERNAME)"'  >> $(VERSIONINC)
	echo '#define RELEASE "$(VERSION).$(PATCHLEVEL)"' >> $(VERSIONINC)
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

