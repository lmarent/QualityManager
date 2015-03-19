#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([qualityManager],[0.1],[la.marentes455@uniandes.edu.co])
AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_SRCDIR([src/main.cpp])

## ------------------------Begin instantiation part 
## Meaning:
## An instantiating macro is one which defines one or more tags, 
## usually referring to files that are to be translated by the generated 
## configure scripts, from a template containing Autoconf substitution 
## variables.

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE

# Checks for build environment
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_INSTALL

LT_INIT([dlopen])
AC_PROG_MAKE_SET

AC_LANG([C++])


dnl # Platform 
case "$target" in
*-linux*)
        OS=LINUX
        AC_DEFINE(LINUX, 1, "Linux OS")
        ;;
*-sunos*)
        OS=SUNOS
        AC_DEFINE(SUNOS, 1, "SUNOS")
        ;;
*-irix*)
        OS=IRIX
        AC_DEFINE(IRIX, 1, "IRIX OS")
        ;;
*-solaris*)
        OS=SOLARIS
        AC_DEFINE(SOLARIS, 1, "Solaris OS")
        ;;
*-freebsd*)
        OS=FREEBSD
        AC_DEFINE(FREEBSD, 1, "FreeBSD OS")
        ;;
*dec-os*)
        OS=ALPHA
        AC_DEFINE(ALPHA, 1, "Alpha")
        ;;
*)
        OS=UNIX
        AC_DEFINE(UNIX, 1, "Unix")
        ;;
esac

dnl # Hardware

case "$target" in
*i*86*)
        PROC=LINUX
        AC_DEFINE(INTEL, 1, "Intel")
        ;;
esac

# use our own defaults
LDFLAGS="$LDFLAGS -L/lib -L/usr/local/lib -L/usr/lib "
CFLAGS="$_CFLAGS -Wall"
CXXFLAGS="$_CXXFLAGS -Wall -Wno-deprecated "

dnl # check the options

AC_WITH(ssl,
  [LDFLAGS="$LDFLAGS -L$withval/lib"
   CXXFLAGS="$CXXFLAGS -I$withval/include"
   CFLAGS="$CFLAGS -I$withval/include"
   CPPFLAGS="$CPPFLAGS -I$withval/include"
   SSL_DIR=$withval
])

AC_WITH(xml2,
  [XML2_DIR=$withval
])

AC_WITH(xslt,
  [XSLT_DIR=$withval
])


AC_ARG_ENABLE(debug,
  [  --enable-debug            enable debug build ],
  [case "${enableval}" in
    yes) debug=true ;;
    no)  debug=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
  esac],[debug=false])
AM_CONDITIONAL(ENABLE_DEBUG, test x$debug = xtrue)
if test $debug = true ; then
  AC_DEFINE(ENABLE_DEBUG, 1, [enable debug build])
fi

AC_ARG_ENABLE(ssl,
  [  --enable-ssl            enable ssl ],
  [case "${enableval}" in
    yes) ssl=true ;;
    no)  ssl=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-ssl) ;;
  esac],[ssl=false])
AM_CONDITIONAL(ENABLE_SSL, test x$ssl = xtrue)
if test $ssl = true ; then
  if test -z "$SSL_DIR" && test -d /usr/local/ssl ; then
    LDFLAGS="$LDFLAGS -L/usr/local/ssl/lib"
    CXXFLAGS="$CXXFLAGS -I/usr/local/ssl/include"
    CFLAGS="$CFLAGS -I/usr/local/ssl/include"
    CPPFLAGS="$CPPFLAGS -I/usr/local/ssl/include"
  fi
  LIBS="$LIBS -lcrypto"

  AC_CHECK_HEADERS([openssl/ssl.h openssl/err.h openssl/bio.h openssl/evp.h],,
  [AC_MSG_ERROR([cannot find ssl headers])])
  AC_CHECK_LIB(ssl, [SSL_library_init],
  [SSLLIB="-lssl"],
  [AC_MSG_ERROR([cannot find open ssl])])
  USE_SSL="-DUSE_SSL"

  SSL_PASSWD=netmate
  AC_DEFINE_UNQUOTED(SSL_PASSWD, "$SSL_PASSWD", [password for ssl encryption])
  AC_DEFINE(USE_SSL, 1, [use ssl flag])
fi


AC_ARG_ENABLE(threads,
  [  --enable-threads            enable threads ],
  [case "${enableval}" in
    yes) threads=true ;;
    no)  threads=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-threads) ;;
  esac],[threads=false])
AM_CONDITIONAL(ENABLE_THREADS, test x$threads = xtrue)
if test $threads = true ; then
  AC_DEFINE(ENABLE_THREADS, 1, [enable multi threading])
fi


dnl # check size

AC_CHECK_SIZEOF(off_t)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([net/bpf.h net/ethernet.h ether.h arpa/inet.h fcntl.h float.h types.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h termios.h unistd.h float.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_CHECK_FUNCS([alarm bzero floor getaddrinfo inet_ntoa gettimeofday memmove memset select socket strdup strchr strerror strstr atoll strtof getopt_long])


PKG_CHECK_MODULES(NL3, libnl-3.0 >= 3.0, [
	AC_SUBST(NL3_CFLAGS)
	AC_SUBST(NL3_LIBS)
	AC_DEFINE(HAVE_LIBNL, [1], ["libnl is available"])
		has_libnl="yes"
], [
	AC_MSG_WARN([libnl not found])
])


PKG_CHECK_MODULES(NL_ROUTE_3, libnl-route-3.0 >= 3.0, [
	AC_SUBST(NL_ROUTE_3_CFLAGS)
	AC_SUBST(NL_ROUTE_3_LIBS)
	AC_DEFINE(HAVE_LIBNL_ROUTE, [1], ["libnl-route is available"])
		has_libnl_route="yes"
], [
	AC_MSG_WARN([libnl_route not found])
])



if test $threads = true ; then
  AC_CHECK_HEADERS([pthread.h])
  if test $OS = FREEBSD ; then
    OCCFLAGS=$CXXFLAGS
    OCFLAGS=$CFLAGS
    CXXFLAGS="$CXXFLAGS -D_THREAD_SAFE -I/usr/local/include/pthread/linuxthreads"
    CFLAGS="$CFLAGS -D_THREAD_SAFE -I/usr/local/include/pthread/linuxthreads"
    have_linux_threads="yes"
    AC_CHECK_LIB(lthread, pthread_create,
    [PTHREADLIB="-llthread -llgcc_r"],[ have_linux_threads="no" ] )
    #if no linuxthreads try libc_r 
    CXXFLAGS="$OCXXFLAGS -pthread"
    CFLAGS="$OCFLAGS -pthread"
    AC_CHECK_LIB(c_r, pthread_create, [PTHREADLIB="-lc_r"])
    # actually newer FREEBSD version should have pthread... 
  else
    AC_CHECK_LIB(pthread, pthread_create,
      [PTHREADLIB="-lpthread"])
  fi
fi



# check for Poco Foundation library
# Checks for libraries.
AC_LANG(C++)
SAVED_LDFLAGS=$LDFLAGS
SAVED_CPPFLAGS=$CPPFLAGS
SAVED_LIBS=$LIBS
CPPFLAGS="$CPPFLAGS -I/usr/local/include/Poco"
LDFLAGS="$LDFLAGS -L/usr/local/lib/"
LIBS="$LIBS -lPocoFoundation -lPocoUtil -lPocoNet"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <DateTime.h>],
    [Poco::DateTime now])],
    [poco_LIBS="-lPocoFoundation -lPocoUtil -lPocoNet"] 
    [poco_CFLAGS="-DHAVE_PCAP -I/usr/local/include/Poco"] 
    [poco_LDFLAGS]="-L/usr/local/lib/"]
    [HAVE_POCO=1],
    [AC_MSG_WARN([libPoco is not installed.])])
CPPFLAGS=$SAVED_CPPFLAGS 
LDFLAGS=$SAVED_LDFLAGS
LIBS= $SAVED_LIBS


dnl # Checks for XML lib
if test -z "$XML2_DIR" ; then 
  AC_PATH_PROG(XML_CONFIG, xml2-config, no) 
  if test "$XML_CONFIG" = "no" ; then
    AC_MSG_WARN([cannot find xml2-config])
  else
    XML_LIBS=`$XML_CONFIG --libs` 
    XML_CFLAGS=`$XML_CONFIG --cflags` 
    LDFLAGS="$LDFLAGS $XML_LIBS" 
    CFLAGS="$CFLAGS $XML_CFLAGS" 
    CXXFLAGS="$CXXFLAGS $XML_CFLAGS" 
    CPPFLAGS="$CPPFLAGS $XML_CFLAGS" 
  fi
else          
   LDFLAGS="$LDFLAGS -L$XML2_DIR/lib -lz -lm"          
   CFLAGS="$CFLAGS -I$XML2_DIR/include" 
   CXXFLAGS="$CXXFLAGS -I$XML2_DIR/include" 
   CPPFLAGS="$CPPFLAGS -I$XML2_DIR/include" 
fi

if test "$XSLT_DIR" ; then
   LDFLAGS="$LDFLAGS -L$XSLT_DIR/lib"          
   CFLAGS="$CFLAGS -I$XSLT_DIR/include" 
   CXXFLAGS="$CXXFLAGS -I$XSLT_DIR/include"
   CPPFLAGS="$CPPFLAGS -I$XSLT_DIR/include"
fi

AC_CHECK_HEADERS([libxml/parser.h],,[AC_MSG_ERROR([cannot find libxml2 headers])]) 
AC_CHECK_LIB(xml2, xmlParseFile,
  [XMLLIB="-lxml2"],
  [AC_MSG_ERROR([cannot find libxml2])])
dnl # check xslt lib
AC_CHECK_HEADERS([libxslt/transform.h libxslt/xsltutils.h],,
  [AC_MSG_WARN([cannot find xslt headers])])
AC_CHECK_LIB(xslt, xsltApplyStylesheet,
  [XSLTLIB="-lxslt"],
  [AC_MSG_WARN([cannot find libxsltl])])


dnl # checks for programs

AC_CHECK_PROG(DOXY,doxygen,doxygen,[])
if test "$DOXY" = "" ; then
  AC_MSG_WARN([doxygen not installed, can not build documentation])
fi

dnl # set version and data

dnl # Output substitution

AC_DEFINE_UNQUOTED(BUILD_TIME, "`date +"%d/%m/%Y %H:%M:%S"`", "release date")

dnl # set default directories

if test "${prefix}" = "NONE" ; then
  prefix=${ac_default_prefix}
fi

dnl # set default config directory for testing
#sysconfdir="${sysconfdir}/$PACKAGE"

sysconfdir="/home/luis/QualityManager/etc"

#DEF_SYSCONFDIR=`eval echo ${sysconfdir}`
DEF_SYSCONFDIR="/home/luis/QualityManager/etc"
AC_DEFINE_UNQUOTED(DEF_SYSCONFDIR, "$DEF_SYSCONFDIR", "sysconfig dir")

DEF_STATEDIR=`eval echo ${localstatedir}`
AC_DEFINE_UNQUOTED(DEF_STATEDIR, "$DEF_STATEDIR", "state dir")

libdir="${libdir}/$PACKAGE"
if test "${exec_prefix}" = "NONE" ; then
  exec_prefix=${prefix}
fi
DEF_LIBDIR=`eval echo ${libdir}`
AC_DEFINE_UNQUOTED(DEF_LIBDIR, "$DEF_LIBDIR", "lib dir")

DEF_BINDIR=`eval echo ${bindir}`
AC_DEFINE_UNQUOTED(DEF_BINDIR, "$DEF_BINDIR", "bin dir")


AC_SUBST(poco_CFLAGS)
AC_SUBST(poco_LIBS)
AC_SUBST(poco_LDFLAGS)

AC_SUBST(VERSION)
AC_SUBST(REL_DATE)
AC_SUBST(OS)
AC_SUBST(ENABLE_NF)
AC_SUBST(ENABLE_TEST)
AC_SUBST(ENABLE_DEBUG)
AC_SUBST(READLINELIB)
AC_SUBST(HISTORYLIB)
AC_SUBST(PTHREADLIB)
AC_SUBST(DLLIB)
AC_SUBST(SSLLIB)
AC_SUBST(ULOGLIB)
AC_SUBST(XMLLIB)
AC_SUBST(XSLTLIB)
AC_SUBST(MATHLIB)
AC_SUBST(USE_SSL)
AC_SUBST(SSL_PASSWD)
AC_SUBST(IPTABLES_VERSION)
AC_SUBST(CURLLIB)
AC_SUBST(DEF_SYSCONFDIR)
AC_SUBST(DEF_STATEDIR)
AC_SUBST(DEF_LIBDIR)
AC_SUBST(DEF_BINDIR)
AC_SUBST(DOXY)



AC_CONFIG_FILES([Makefile
				lib/httpd/Makefile \
				lib/getopt_long/Makefile \
				proc_modules/Makefile \
                src/Makefile])

AC_OUTPUT
