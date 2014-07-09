AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = qualityManager
qualityManager_SOURCES = main.cpp \
						 NetworkQualityServer.cpp \
						 ConnectionHandler.cpp \
						 RouteManager.cpp \
						 RouteException.cpp \
						 route_functions.c \
						 route_functions.h \
						 HandlerManager.cpp \
						 tc_functions.c \
						 tc_functions.h \
						 TrafficControlException.cpp \
						 TrafficControlManager.cpp \
						 RouteSys.cpp \
						 TrafficControlSys.cpp \
						 NetworkInterface.cpp
						 

# what flags you want to pass to the C compiler & linker
AM_CPPFLAGS = -g -I@top_srcdir@/include @poco_CFLAGS@ @NL3_CFLAGS@ @NL_ROUTE_3_CFLAGS@
AM_LDFLAGS = @poco_LDFLAGS@ @NL3_LIBS@ @NL_ROUTE_3_LIBS@
LDADD = @poco_LIBS@ 
