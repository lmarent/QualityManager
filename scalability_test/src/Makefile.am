
bin_PROGRAMS = scalTest


if ENABLE_DEBUG
  AM_CXXFLAGS = -g -fno-inline -D_GLIBCXX_DEBUG -DDEBUG -DPROFILING
else
  AM_CXXFLAGS = -O2  
endif

scalTest_SOURCES = Error.cpp \
		   Logger.cpp \
		   CommandLineArgs.cpp \
		   constants.cpp \
		   scal_test.cpp



scalTest_CPPFLAGS = -I$(top_srcdir)/scalability_test/include \
		    -I$(top_srcdir)/lib/getopt_long \
		    -I$(top_srcdir)/lib/httpd

scalTest_LDADD = $(top_builddir)/lib/getopt_long/libgetopt_long.a \
		 $(top_builddir)/lib/httpd/libhttpd.a \
		 @PTHREADLIB@ @DLLIB@ @SSLLIB@ @XMLLIB@


AM_CPPFLAGS = -g 
AM_LDFLAGS = @CURL_LIBS@
LDADD =
