if BUILD_NQRSH

bin_PROGRAMS = nqrsh 

if ENABLE_DEBUG
  AM_CFLAGS = -g -fno-inline -D_GLIBCXX_DEBUG -DDEBUG -DPROFILING
else
  AM_CFLAGS = -O2 
endif

nqrsh_SOURCES = nqrsh.cpp $(top_srcdir)/src/Error.cpp \
                $(top_srcdir)/src/CommandLineArgs.cpp \
                $(top_srcdir)/src/constants.cpp \
                $(top_srcdir)/lib/getopt_long/getopt_long.c \
				$(top_srcdir)/lib/getpass/getpass.c

nqrsh_LDADD   = @READLINELIB@ @SSLLIB@ @MPATROLLIB@ @CURL_LIBS@ @XMLLIB@ @XSLTLIB@ \
                $(top_builddir)/lib/getopt_long/libgetopt_long.a \
				$(top_builddir)/lib/getpass/libgetpass.a

INCLUDES      = -I$(top_srcdir) -I$(top_srcdir)/src/include -I$(top_srcdir)/include -I$(top_srcdir)/lib/getopt_long -I$(top_srcdir)/lib/getpass

$(top_builddir)lib/getopt_long/libgetopt_long.a:
	cd $(top_builddir)/lib/getopt_long ; $(MAKE)

$(top_builddir)/lib/getpass/libgetpass.a:
	cd $(top_builddir)/lib/getpass ; $(MAKE)

else
all:
	@echo "WARNING: not building nqrsh because libxslt and/or libcurl are missing!"
endif
