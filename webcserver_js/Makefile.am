AUTOMAKE_OPTIONS = foreign
SUBDIRS = 
CLEANFILES = *~
BUILT_SOURCES = ${DESTDIR}@htmldir@/boxsitehelper/boxsitehelper.nocache.js
BUILT_DEPENDS = $(shell find src -name "*.java" -type f)

${DESTDIR}@htmldir@/boxsitehelper/boxsitehelper.nocache.js: $(BUILT_DEPENDS)
	ant -Dbuildroot=${DESTDIR}@htmldir@
	-rm -r ${DESTDIR}@htmldir@/WEB-INF
	-rm -r ${DESTDIR}@htmldir@/boxsitehelper/gwt


clean-local:
	ant clean