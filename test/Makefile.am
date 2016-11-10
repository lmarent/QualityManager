AUTOMAKE_OPTIONS = foreign

# Rules for the test code (use `make check` to execute)
TESTS = test_runner
check_PROGRAMS = $(TESTS)

test_runner_SOURCES = @top_srcdir@/src/Error.cpp \
				      @top_srcdir@/src/constants.cpp \
					  @top_srcdir@/src/Logger.cpp \
					  @top_srcdir@/src/XMLParser.cpp \
					  @top_srcdir@/src/CommandLineArgs.cpp \
					  @top_srcdir@/src/ConfigManager.cpp \
					  @top_srcdir@/src/ParserFcts.cpp \
					  @top_srcdir@/src/ConfigParser.cpp \
					  @top_srcdir@/src/FilterValue.cpp \
					  @top_srcdir@/src/FilterDefParser.cpp \
					  @top_srcdir@/src/FilterValParser.cpp \
					  @top_srcdir@/src/PageRepository.cpp \
					  @top_srcdir@/src/Module.cc \
					  @top_srcdir@/src/Timeval.cpp  \
					  @top_srcdir@/src/Event.cpp \
					  @top_srcdir@/src/QualityManagerInfo.cpp \
					  @top_srcdir@/src/CtrlComm.cpp \
					  @top_srcdir@/src/constants_qos.cpp \
					  @top_srcdir@/src/QualityManagerComponent.cpp \
					  @top_srcdir@/src/ProcModule.cpp  \
					  @top_srcdir@/src/Rule.cpp \
					  @top_srcdir@/src/RuleFileParser.cpp \
					  @top_srcdir@/src/RuleIdSource.cpp \
					  @top_srcdir@/src/MAPIRuleParser.cpp \
					  @top_srcdir@/src/RuleManager.cpp \
					  @top_srcdir@/src/EventScheduler.cpp \
					  @top_srcdir@/src/PerfTimer.cpp \
					  @top_srcdir@/src/QOSProcessor.cpp \
					  @top_srcdir@/src/ModuleLoader.cpp \
					  @top_srcdir@/src/QualityManager.cpp \
					  @top_srcdir@/test/QoSProcessor_test.cpp \
					  @top_srcdir@/test/QoSProcessorThreaded_test.cpp \
					  @top_srcdir@/test/QualityManager_test.cpp \
					  @top_srcdir@/test/QualityManagerThreaded_test.cpp \
					  @top_srcdir@/test/test_runner.cpp

if ENABLE_DEBUG
  AM_CXXFLAGS = -g -I@top_srcdir@/include $(CPPUNIT_CFLAGS) \
				-I$(top_srcdir)/lib/getopt_long -I$(top_srcdir)/lib/httpd \
			    -fno-inline -ggdb -DDEBUG -DPROFILING -DINTEL			    
else
  AM_CXXFLAGS = -O2 -I@top_srcdir@/include $(CPPUNIT_CFLAGS) \
				-I$(top_srcdir)/lib/getopt_long -I$(top_srcdir)/lib/httpd
endif
			     
AM_CPPFLAGS = -g -I@top_srcdir@/include @NL3_CFLAGS@ @NL_ROUTE_3_CFLAGS@

AM_LDFLAGS = @NL3_LIBS@ @NL_ROUTE_3_LIBS@ $(CPPUNIT_LIBS) @PTHREADLIB@ \
			-ldl -lcppunit \
			 $(top_builddir)/lib/getopt_long/libgetopt_long.a \
			 $(top_builddir)/lib/httpd/libhttpd.a
			 	 
LDADD = @NL3_CFLAGS@ @NL_ROUTE_3_CFLAGS@

