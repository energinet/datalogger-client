# (C) LIAB ApS 2009 - 2014

SHELL = /bin/sh
PWD := $(shell pwd)

CROSS_COMPILE = --host=arm-unknown-linux-gnu
COMPONENTNAME = liabconnect

ARMLIBDIR = /opt/crosstool/gcc-4.0.2-glibc-2.3.6/arm-unknown-linux-gnu/arm-unknown-linux-gnu/lib
ARMINCLUDEDIR = /opt/crosstool/gcc-4.0.2-glibc-2.3.6/arm-unknown-linux-gnu/arm-unknown-linux-gnu/include

NCFAPPS =  lognetcdf netcdf-module
NCFLIBS = netcdf

PARALLEL = 9 #Parallel MAKE (CPU cnt+1)

SUBAPPS = $(shell awk -F"\t" '{if ($$1) print $$1}' components)
# webcserver_js $(ENERGINET)
# flash-chart

ENERGINETOSSDIRS = aeeprom armlib cmddb db-local en_modules flash-chart kamcom-mod licon-mod modemd webcserver_js db-module enstyrerapp lgmod logdb modem-module rpclient wirelesstrx-module asocket libsdvp lognetcdf netcdf-module rpfchart-arm wr-module bash-module contdaem eggctrl libstrophe patches sdvpd wdt xml_cntn endata firmwareupdate kamcom licon modbusd socketmod webcserver xmlparser

ENERGINETOSSFILES = Makefile .gitignore README README.md

SUBLIBS = gsoap qDecoder sqlite expat

BUILDDIR   = buildroot
DEVICEDIR  = deviceroot
DEVICEROOT = $(PWD)/$(DEVICEDIR)
BUILDROOT  = $(PWD)/$(BUILDDIR)

OPENSOURCE = opensourceedition
OPENSOURCETMPDIR = /tmp/
COMPONENTNAME = contdaem
COPY_LIBS = usb confuse z

CONFIGURE_ARM_OPTS = $(CROSS_COMPILE) --with-libraries=$(PWD)/armlib/lib:$(BUILDROOT)/usr/lib:$(ARMLIBDIR) --with-includes=$(PWD)/armlib/include:$(BUILDROOT)/usr/include:$(ARMINCLUDEDIR)
CONFIGURE_OPTS =  --includedir=/usr/include --sysconfdir=/etc --bindir=/usr/bin  --libdir=/usr/lib
CONFIGURE_EXTRA =
INSTALLSTR = install-strip
LIBBASE = armlib

base: version.txt cp-include cp-so sublibs subapps versionstamp 

xmpp: CONFIGURE_EXTRA = --enable-sdvp
xmpp: base $(addsuffix -mk, $(SUBAPPS_XMPP)) tar

sdvp: CONFIGURE_EXTRA = --enable-sdvp
sdvp: base $(addsuffix -mk, $(SUBAPPS_SDVP)) tar

dacs: base $(addsuffix -mk, $(SUBAPPS_DACS)) tar

all: version.txt cp-include cp-so sublibs subapps versionstamp tar

sublibs: $(addsuffix -lib, $(SUBLIBS))
subapps: $(addsuffix -mk, $(SUBAPPS))


.PRECIOUS: %/configure
%/configure: %/configure.ac
	cd $* ; autoreconf --install

.PRECIOUS: %/Makefile
%/Makefile: %/configure
	cd $* ; ./configure $(CONFIGURE_ARM_OPTS) $(CONFIGURE_OPTS) $(CONFIGURE_EXTRA) $(shell grep -P "$*\t" components | awk -F"\t" '{if ($$1) print $$2}')

%-mk: %/Makefile
	mkdir -p $(BUILDROOT)
	$(MAKE) -j$(PARALLEL) -C  $* DB_FILE="/jffs2/bigdb.sql" DESTDIR=$(BUILDROOT) $(INSTALLSTR)
	test -f $(BUILDROOT)/usr/lib/*.la; \
	if ([ $$? -ne 1 ] ) ; then \
		rm $(BUILDROOT)/usr/lib/*.la;\
	fi ; \

%-lib: %/Makefile
	mkdir -p $(BUILDROOT)
	$(MAKE) -j$(PARALLEL) -C  $* DB_FILE="/jffs2/bigdb.sql" DESTDIR=$(BUILDROOT) $(INSTALLSTR)
	-rm $(BUILDROOT)/usr/lib/*.la

%-mknon:
	$(MAKE) -C  $* install-strip


%-install:
	mkdir -p $(DEVICEROOT)
	$(MAKE) -C  $* DESTDIR=$(DEVICEROOT) install

%-build:
	mkdir -p $(DEVICEROOT)
	$(MAKE) -C  $* 

%-clean: 
	-$(MAKE) -C $* clean


%-distclean: %-clean
	-rm $*/Makefile 
	-rm $*/configure 

.PHONY: deviceroot
deviceroot: $(BUILDROOT) 
	-rm -r $(DEVICEROOT)
	cp -ra $(BUILDROOT) $(DEVICEROOT)
	-rm -r $(DEVICEROOT)/usr/share/control-daemon/plugins/*.la
	-rm -r $(DEVICEROOT)/usr/share/control-daemon/plugins/*.a
	-rm -r $(DEVICEROOT)/usr/lib/*.la
	-rm -r $(DEVICEROOT)/usr/lib/*.a
	-rm -r $(DEVICEROOT)/usr/include
	-rm -r $(DEVICEROOT)/usr/remove
	-rm -r $(DEVICEROOT)/usr/local
	-rm -r $(DEVICEROOT)/usr/lib/pkgconfig

.PHONY: buildroot
buildroot: cp-so

.PHONY: buildroot.tar.gz
buildroot.tar.gz: buildroot
	cd $(BUILDROOT) ; tar -cvzf ../$(BUILDDIR).tar.gz *


cp-so:
	mkdir -p $(BUILDROOT)/usr/lib
	list='$(COPY_LIBS)'; for libs in $$list; do \
	cp -a armlib/lib/lib$$libs*.so* $(BUILDROOT)/usr/lib ;\
	done

cp-include:
	mkdir -p $(BUILDROOT)/usr/include
	-cp  armlib/include/*.h $(BUILDROOT)/usr/include
	mkdir -p $(BUILDROOT)/usr/include/openssl
	-cp  armlib/include/openssl/*.h $(BUILDROOT)/usr/include/openssl



tar: deviceroot
	cd $(DEVICEROOT) ; tar -cvzf ../$(COMPONENTNAME).tar.gz *
	ls -l  $(COMPONENTNAME).tar.gz
	ls -lh  $(COMPONENTNAME).tar.gz

rcp-%: deviceroot
	rcp -r $(DEVICEROOT)/* root@$*:/

versionstamp:
	echo -n "svn: " > firmwareupdate/files/version	
	if test -d .git ; then \
		if (git describe --tags 2> /dev/null) ; then \
		    git describe --tags | xargs echo -n >> firmwareupdate/files/version; \
	        else \
	            git log -1 --pretty=format:'%h (%ci)' --abbrev-commit| xargs echo -n >> firmwareupdate/files/version; \
		fi ; \
	    else \
		svnversion | xargs echo -n >> firmwareupdate/files/version; \
	fi ; 
	date >> firmwareupdate/files/version
	cat firmwareupdate/files/version

.PHONY: version.txt
version.txt:	
	echo "" > version.txt
	if test -d .git ; then \
		if (git describe --tags 2> /dev/null) ; then \
		    git describe --tags | xargs echo -n >> version.txt; \
	        else \
	            git log -1 --pretty=format:'%h (%ci)' --abbrev-commit| xargs echo -n >> version.txt; \
		fi ; \
	    else \
		svnversion | xargs echo -n >> version.txt; \
	fi ; 

clean:  $(addsuffix -clean, $(SUBAPPS))
	-rm -r $(DEVICEROOT)
	-rm -r $(BUILDROOT)

distclean: $(addsuffix -distclean, $(SUBAPPS))
	-rm -r $(DEVICEROOT)
	-rm -r $(BUILDROOT)
	-rm -fr expat sqlite qDecoder gsoap
	-rm $(COMPONENTNAME).tar.gz
	-find -name config.guess -exec rm {} \;
	-find -name config.log -exec rm {} \;
	-find -name config.status -exec rm {} \;
	-find -name config.sub -exec rm {} \;
	-find -name '*~' -exec rm {} \;
	-find -name '#*#' -exec rm {} \;
	-find -name aclocal.m4 -exec rm {} \;
	-find -name autom4te.cache -exec rm -fr {} \; -a -prune
	-find -name .deps -exec rm -fr {} \; -a -prune
	-find -name depcomp -exec rm {} \;
	-find -name libtool -exec rm {} \;
	-find -name ltmain.sh -exec rm {} \;
	-find -name missing -exec rm {} \;

cleansrc:
	-rm -fr  expat-2.0.1.tar.gz sqlite-autoconf-3070603.tar.gz qDecoder-10.1.1.tar.gz gsoap_2.7.16.zip

jffs2upd: versionstamp tar
	cp  $(COMPONENTNAME).tar.gz firmwareupdate/files
	cd firmwareupdate; ./mkudtimg.sh files/ allfiles

buildtar: all
	-rm -rf $(BUILDROOT)/usr/remove
	cd $(BUILDROOT) ; tar cvzf ../$(BUILDDIR).tar.gz *
	ls -l  $(BUILDDIR).tar.gz

svnci:
	list='$(SUBAPPS)'; for svnext in $$list; do \
	cd $(PWD)/$$svnext; svn ci; cd ..;\
	done
	svn ci

ose:
	-rm -r $(OPENSOURCETMPDIR)/$(OPENSOURCE)
	-rm $(OPENSOURCE).tar.gz
	mkdir -p $(OPENSOURCETMPDIR)/$(OPENSOURCE)
	list='$(ENERGINETOSS)'; for svnexp in $$list; do \
	svn export $$svnexp $(OPENSOURCETMPDIR)/$(OPENSOURCE)/$$svnexp;\
	done
	tar xvf armlibOSE.tar.gz -C $(OPENSOURCETMPDIR)/$(OPENSOURCE)
	cd $(OPENSOURCETMPDIR)/$(OPENSOURCE); find -name '*.c' -exec patch {} $(PWD)/gplheader-patch \;; find -name '*.h' -exec patch {} $(PWD)/patches/gplheader-patch \;; find -name '*.rej' -exec rm {} \;; find -name '*.orig' -exec rm {} \;
	cp Makefile.OSE $(OPENSOURCETMPDIR)/$(OPENSOURCE)/Makefile
	cp patches/gsoap-patch $(OPENSOURCETMPDIR)/$(OPENSOURCE)/
	cd $(OPENSOURCETMPDIR); tar cvzf $(PWD)/$(OPENSOURCE).tar.gz $(OPENSOURCE)
	ls -l $(OPENSOURCE).tar.gz
	ls -lh $(OPENSOURCE).tar.gz

oss:
	-rm -rf $(OPENSOURCETMPDIR)/$(OPENSOURCE)
	-rm -f $(OPENSOURCE).tar.gz
	list='$(ENERGINETOSSDIRS)'; for gitexp in $$list; do \
		mkdir -p $(OPENSOURCETMPDIR)/$(OPENSOURCE)/$$gitexp; \
		cd $$gitexp;\
		git archive HEAD | tar -x -C $(OPENSOURCETMPDIR)/$(OPENSOURCE)/$$gitexp;\
		cd ..;\
	done
	list='$(ENERGINETOSSFILES)'; for gitexp in $$list; do \
		cp $$gitexp $(OPENSOURCETMPDIR)/$(OPENSOURCE)/; \
	done
	cp components-kamstrup $(OPENSOURCETMPDIR)/$(OPENSOURCE)/components
	tar cvJf $(OPENSOURCETMPDIR)/$(OPENSOURCE).tar.xz $(OPENSOURCETMPDIR)/$(OPENSOURCE);

#qDecoder Special Start
qDecoder-10.1.1.tar.gz:
	wget http://www.qdecoder.org/ftp/pub/qdecoder/qDecoder-10.1.1.tar.gz
qDecoder: qDecoder-10.1.1.tar.gz
	tar xvf qDecoder-10.1.1.tar.gz
	mv qDecoder-10.1.1 qDecoder
	rm qDecoder/configure
	mkdir -p $(BUILDROOT)/usr/include $(BUILDROOT)/usr/lib
	$(MAKE) qDecoder/Makefile
	sed -ir "s/{LN_S} -f /{LN_S} -fr /" qDecoder/src/Makefile
qDecoder/configure.ac: qDecoder
qDecoder-lib: INSTALLSTR = install
qDecoder/Makefile: CONFIGURE_OPTS = --disable-socket --disable-ipc --disable-datastructure  --libdir=$(BUILDROOT)/usr/lib --prefix=$(PWD) --includedir=$(BUILDROOT)/usr/include $(CROSS_COMPILE)
#qDecoder Special End

#gSOAP Special Start
gsoap_2.7.16.zip:
	wget http://downloads.sourceforge.net/project/gsoap2/gSOAP/gSOAP%202.7.16%20stable/gsoap_2.7.16.zip
gsoap: gsoap_2.7.16.zip
	unzip gsoap_2.7.16.zip
	mv gsoap-2.7 gsoap
	rm gsoap/configure
	cd gsoap; patch -p1 < ../patches/gsoap-patch 
gsoap/Makefile: CONFIGURE_OPTS = --bindir=/usr/remove --libdir=/usr/lib --includedir=/usr/include  LDFLAGS=-L$(PWD)/$(LIBBASE)/lib CPPFLAGS="-I$(PWD)/$(LIBBASE)/include"
# --enable-debug
gsoap/Makefile: CONFIGURE_ARM_OPTS = $(CROSS_COMPILE)
gsoap/configure.ac: gsoap
gsoap-lib: INSTALLSTR = install
gsoap-lib: PARALLEL = 1
#gSOAP Special End

#sqlite Special Start
sqlite-autoconf-3070603.tar.gz:
	wget http://www.sqlite.org/sqlite-autoconf-3070603.tar.gz
sqlite: sqlite-autoconf-3070603.tar.gz
	tar xvf sqlite-autoconf-3070603.tar.gz
	-rm -fr sqlite
	mv sqlite-autoconf-3070603 sqlite
sqlite/configure.ac: sqlite
#sqlite Special End

#expat Special Start
expat-2.0.1.tar.gz:
	wget http://sourceforge.net/projects/expat/files/expat/2.0.1/expat-2.0.1.tar.gz
expat: expat-2.0.1.tar.gz
	tar xvf expat-2.0.1.tar.gz
	-rm -fr expat
	mv expat-2.0.1 expat
expat/configure: expat
expat-lib: INSTALLSTR = installlib
#expat Special End

#gSOAP depencies special Start
rpclient-mk: PARALLEL = 1
rpfchart-arm-mk: PARALLEL = 1
endata-mk: PARALLEL = 1
#gSOAP depencies special End
