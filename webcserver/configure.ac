AC_INIT([RPC Server for Control Daemon], [0.1],
        [LIAB ApS <info@liab.dk>],
        [procrelay])
AC_CONFIG_SRCDIR(src/live.c)
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE
#AC_CONFIG_HEADERS(include/config.h)

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL
dnl AC_CONFIG_MACRO_DIR([m4]) dnl CRA 09/02/04
dnl AC_DISABLE_SHARED         dnl CRA 09/02/04

AC_PROG_LIBTOOL           dnl CRA 09/02/05



AC_PATH_PROGS(BASH, bash)

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

dnl Modules...'nuff said.
AC_ARG_WITH(includes,
  [AC_HELP_STRING(
    [--with-includes=LIST],
    [add additional include paths to eesd. LIST is a colon-separated list of include paths to add e.g. --with-includes=/some/mysql/include:/my/include])
  ],
  [ ac_addl_includes=`echo "$withval" | sed -e 's/:/ /g'` ;
    for ainclude in $ac_addl_includes; do
      if test x"$ac_build_addl_includes" = x ; then
        ac_build_addl_includes="-I$ainclude"
      else
        ac_build_addl_includes="-I$ainclude $ac_build_addl_includes"
      fi
    done
    CPPFLAGS="$CPPFLAGS $ac_build_addl_includes"
  ])

AC_ARG_WITH(libraries,
  [AC_HELP_STRING(
    [--with-libraries=LIST],
    [add additional library paths eesd LIST is a colon-separated list of include paths to add e.g. --with-libraries=/some/mysql/libdir:/my/libs])
  ],
  [ ac_addl_libdirs=`echo "$withval" | sed -e 's/:/ /g'` ;
    for alibdir in $ac_addl_libdirs; do
      if test x"$ac_build_addl_libdirs" = x ; then
        ac_build_addl_libdirs="-L$alibdir"
      else
        ac_build_addl_libdirs="-L$alibdir $ac_build_addl_libdirs"
      fi
    done 
    LDFLAGS="$LDFLAGS $ac_build_addl_libdirs"
  ])



AC_CHECK_HEADER(asocket.h,, AC_MSG_ERROR(asocket.h not found try using --with-includes ))
AC_CHECK_LIB(asocket, asckt_srv_start,,AC_MSG_ERROR(asocket not found try using --with-libraries))


AC_CHECK_HEADER(pthread.h,, AC_MSG_ERROR(pthread.h not found try using --with-includes ))
AC_CHECK_LIB(pthread, pthread_create,,AC_MSG_ERROR(pthread not found try using --with-libraries))

AC_CHECK_HEADER(expat.h,, AC_MSG_ERROR(expat.h not found try using --with-includes ))
AC_CHECK_LIB(expat, XML_SetElementHandler,,AC_MSG_ERROR(expat not found try using --with-libraries))

AC_CHECK_HEADER(dlfcn.h,, AC_MSG_ERROR(dlfcn.h not found try using --with-includes ))
AC_CHECK_LIB(dl, dlsym,,AC_MSG_ERROR(dl not found try using --with-libraries))

AC_CHECK_HEADER(sqlite3.h,, AC_MSG_ERROR(sqlite3.h not found try using --with-includes ))
AC_CHECK_LIB(sqlite3, sqlite3_open,,AC_MSG_ERROR(sqlite3 not found try using --with-libraries))

AC_CHECK_HEADER(qDecoder.h,, AC_MSG_ERROR(qDecoder.h not found try using --with-includes ))
AC_CHECK_LIB(qDecoder, qEntry,,AC_MSG_ERROR(qDecoder not found try using --with-libraries))


AC_CHECK_HEADER(logdb.h,, AC_MSG_ERROR(logdb.h not found try using --with-includes ))
AC_CHECK_LIB(logdb, logdb_open,,AC_MSG_ERROR(logdb not found try using --with-libraries))

AC_CHECK_HEADER(cmddb.h,, AC_MSG_ERROR(cmddb.h not found try using --with-includes ))
AC_CHECK_LIB(cmddb, cmddb_insert,,AC_MSG_ERROR(cmddb not found try using --with-libraries))



#AC_CHECK_HEADER(stdsoap2.h,, AC_MSG_ERROR(stdsoap2.h not found try using --with-includes ))
#AC_CHECK_LIB(gsoap, soap_getheader,,AC_MSG_ERROR(gsoap not found try using --with-libraries))

dnl Check if svnversion is avaliable to get the rev number
AC_PATH_PROG(SVNVERSION, svnversion)

plugindir=/usr/share/control-daemon/plugins
AC_MSG_CHECKING(plugindir)
AC_ARG_WITH(plugindir,
  [AC_HELP_STRING(
    [--with-plugindir=DIR],
    [Where to put the plugin modules (default: /usr/share/control-daemon/plugins)])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      plugindir="$withval"
      ;;
    esac])
AC_SUBST(plugindir)
AC_DEFINE_UNQUOTED(PLUGINDIR,"${plugindir}",[Directory where module plugin files reside])
AC_MSG_RESULT(${plugindir})

cgibindir=/var/www/cgi-bin
AC_MSG_CHECKING(cgibindir)
AC_ARG_WITH(cgibindir,
  [AC_HELP_STRING(
    [--with-cgibindir=DIR],
    [Where to put the cgi scripts (default: /var/www/cgi-bin)])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      cgibindir="$withval"
      ;;
    esac])
AC_SUBST(cgibindir)
AC_DEFINE_UNQUOTED(CGIBINDIR,"${cgibindir}",[Directory where module plugin files reside])
AC_MSG_RESULT(${cgibindir})

htmldir=/var/www/htdocs
AC_MSG_CHECKING(htmldir)
AC_ARG_WITH(htmldir,
  [AC_HELP_STRING(
    [--with-htmldir=DIR],
    [Where to put html/js files (default: /var/www/htdocs)])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      htmldir="$withval"
      ;;
    esac])
AC_SUBST(htmldir)
AC_DEFINE_UNQUOTED(HTMLDIR,"${htmldir}",[Directory where html files reside])
AC_MSG_RESULT(${htmldir})

dbdir=/jffs2
AC_MSG_CHECKING(dbdir)
AC_ARG_WITH(dbdir,
  [AC_HELP_STRING(
    [--with-dbdir=DIR],
    [Where to put database files (default: /jffs2)])],
  [ case "$withval" in     
    yes|no)
    ;;
    * )
      dbdir="$withval"
      ;;
    esac])
AC_SUBST(dbdir)
AC_DEFINE_UNQUOTED(DBDIR,"${dbdir}",[Directory where database files reside])
AC_MSG_RESULT(${dbdir})

AC_CHECK_HEADER(asocket.h,, AC_MSG_ERROR(asocket.h not found try using --with-includes ))
AC_CHECK_LIB(asocket, asocket_cmd_create,,AC_MSG_ERROR(pthread not found try using --with-libraries))

AC_ARG_ENABLE([sdvp],									   
     [  --enable-sdvp   Turn on styrdinvarmepumpe version],				   
     [case "${enableval}" in
       yes) sdvp=true ;;
       no)  sdvp=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-sdvp]) ;;
     esac],[debug=false])
AM_CONDITIONAL([SDVP], [test x$sdvp = xtrue])

if [SDVP]
then
   AC_DEFINE_UNQUOTED(SDVP, "${sdvp}",[Set to build the SDVP version])
fi


AC_CONFIG_FILES([Makefile src/Makefile html/Makefile ])
AC_OUTPUT
