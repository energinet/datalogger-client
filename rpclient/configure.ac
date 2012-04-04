AC_INIT([RPC Server for Control Daemon], [0.1],
        [LIAB ApS <info@liab.dk>],
        [rpc_server])
#AC_CONFIG_SRCDIR(src/rpserver.c)
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



AC_CHECK_HEADER(pthread.h,, AC_MSG_ERROR(pthread.h not found try using --with-includes ))
AC_CHECK_LIB(pthread, pthread_create,,AC_MSG_ERROR(pthread not found try using --with-libraries))

AC_CHECK_HEADER(dlfcn.h,, AC_MSG_ERROR(dlfcn.h not found try using --with-includes ))
AC_CHECK_LIB(dl, dlsym,,AC_MSG_ERROR(dl not found try using --with-libraries))

AC_CHECK_HEADER(stdsoap2.h,, AC_MSG_ERROR(stdsoap2.h not found try using --with-includes ))
#AC_CHECK_LIB(gsoap, soap_init,,AC_MSG_ERROR(gsoap not found try using --with-libraries))

AC_CHECK_HEADER(sqlite3.h,, AC_MSG_ERROR(sqlite3.h not found try using --with-includes ))
AC_CHECK_LIB(sqlite3, sqlite3_open,,AC_MSG_ERROR(sqlite3 not found try using --with-libraries))


AC_CHECK_HEADER(logdb.h,, AC_MSG_ERROR(logdb.h not found try using --with-includes ))
AC_CHECK_LIB(logdb, logdb_open,,AC_MSG_ERROR(logdb not found try using --with-libraries))

AC_CHECK_HEADER(asocket.h,, AC_MSG_ERROR(asocket.h not found try using --with-includes ))
AC_CHECK_LIB(asocket, asocket_cmd_create,,AC_MSG_ERROR(pthread not found try using --with-libraries))

AC_CHECK_HEADER(aeeprom.h,, AC_MSG_ERROR(aeeprom.h not found try using --with-includes ))
AC_CHECK_LIB(aeeprom, aeeprom_get_entry,,AC_MSG_ERROR(aeeprom not found try using --with-libraries))

AC_CHECK_HEADER(cmddb.h,, AC_MSG_ERROR(cmddb.h not found try using --with-includes ))
AC_CHECK_LIB(cmddb, cmddb_exec_list,,AC_MSG_ERROR(cmddb not found try using --with-libraries))

AC_CHECK_HEADER(zlib.h,, AC_MSG_ERROR(zlib.h not found try using --with-includes ))
AC_CHECK_LIB(z, gzread,,AC_MSG_ERROR(libz not found try using --with-libraries))

AC_CHECK_HEADER(openssl/md5.h,, AC_MSG_ERROR(openssl/md5.h not found try using --with-includes ))
AC_CHECK_LIB(crypto, MD5,,AC_MSG_ERROR(libcrypto not found try using --with-libraries))

AC_PATH_PROG(soapctool, soapcpp2)


dnl Check if svnversion is avaliable to get the rev number
AC_PATH_PROG(SVNVERSION, svnversion)

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

certdir=/etc/certs
AC_MSG_CHECKING(certdir)
AC_ARG_WITH(certdir,
  [AC_HELP_STRING(
    [--with-certdir=DIR],
    [Where to certificats (default: /etc/certs)])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      certdir="$withval"
      ;;
    esac])
AC_SUBST(certdir)
AC_DEFINE_UNQUOTED(CERTDIR,"${certdir}",[Directory where certificats reside])
AC_MSG_RESULT(${certdir})



rpcheader=`pwd`/include/rpserver.h
AC_MSG_CHECKING(rpcheader) 
AC_ARG_WITH(rpcheader,
  [AC_HELP_STRING(
    [--with-rpcheader=DIR],
    [Where to find the rpc header definition])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      rpcheader="$withval"
      ;;
    esac])
AC_SUBST(rpcheader)
AC_DEFINE_UNQUOTED(RPCHEADER,"${rpcheader}",[Path to the rpcheader])
AC_MSG_RESULT(${rpcheader})



AC_CONFIG_FILES([Makefile gsoap/Makefile scripts/Makefile src/Makefile])
AC_OUTPUT
