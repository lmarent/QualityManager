AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS= -I m4

bin_PROGRAMS = qualityManager


if ENABLE_DEBUG
  AM_CXXFLAGS = -g -fno-inline -D_GLIBCXX_DEBUG -DDEBUG -DPROFILING
else
  AM_CXXFLAGS = -O2  
endif

qualityManager_SOURCES = Error.cpp constants.cpp Logger.cpp \
						 XMLParser.cpp CommandLineArgs.cpp \
						 ConfigManager.cpp ParserFcts.cpp \
						 ConfigParser.cpp FilterValue.cpp \
						 FilterDefParser.cpp FilterValParser.cpp \
					     PageRepository.cpp Module.cc \
						 Timeval.cpp  Event.cpp CtrlComm.cpp \
						 constants_qos.cpp QualityManagerComponent.cpp \
						 ProcModule.cpp Rule.cpp \
						 RuleFileParser.cpp QualityManagerInfo.cpp \
						 RuleIdSource.cpp MAPIRuleParser.cpp RuleManager.cpp \
						 EventScheduler.cpp PerfTimer.cpp \
						 QualityManager.cpp QOSProcessor.cpp \
						 ModuleLoader.cpp main.cpp

qualityManager_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/lib/getopt_long \
						  -I$(top_srcdir)/lib/httpd

qualityManager_LDADD = $(top_builddir)/lib/getopt_long/libgetopt_long.a \
					   $(top_builddir)/lib/httpd/libhttpd.a \
					   @PTHREADLIB@ @DLLIB@ @SSLLIB@ @XMLLIB@

# what flags you want to pass to the C compiler & linker
AM_CPPFLAGS = -g -I@top_srcdir@/include @poco_CFLAGS@ @NL3_CFLAGS@ @NL_ROUTE_3_CFLAGS@
AM_LDFLAGS = @poco_LDFLAGS@ @NL3_LIBS@ @NL_ROUTE_3_LIBS@
LDADD = @poco_LIBS@ 
