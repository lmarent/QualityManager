EXTRA_DIST = ProcModule.h ProcModule.cpp ProcError.h ProcError.cpp

INCLUDES = -I$(top_srcdir)/include \
           -I/usr/src/linux/include

lib_LTLIBRARIES = htb.la tbf.la

if ENABLE_DEBUG
  AM_CFLAGS = -g -DDEBUG -DPROFILING -fPIC @NL3_CFLAGS@ @NL_ROUTE_3_CFLAGS@
  AM_CXXFLAGS = -g -DDEBUG -DPROFILING @NL3_CFLAGS@ @NL_ROUTE_3_CFLAGS@
else
  AM_CFLAGS = -O2 -fPIC @NL3_CFLAGS@ @NL_ROUTE_3_CFLAGS@
  AM_CXXFLAGS = -O2 @NL3_CFLAGS@ @NL_ROUTE_3_CFLAGS@
endif

htb_la_LDFLAGS = -export-dynamic -module @NL3_LIBS@ @NL_ROUTE_3_LIBS@
htb_la_SOURCES = htb_functions.c htb.cpp
htb_la_LIBADD = ProcModule.lo ProcError.lo

tbf_la_LDFLAGS = -export-dynamic -module @NL3_LIBS@ @NL_ROUTE_3_LIBS@
tbf_la_SOURCES = tbf.cpp
tbf_la_LIBADD = ProcModule.lo ProcError.lo

