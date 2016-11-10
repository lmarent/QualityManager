
bin_PROGRAMS = scalTest


if ENABLE_DEBUG
  AM_CXXFLAGS = -g -fno-inline -D_GLIBCXX_DEBUG -DDEBUG -DPROFILING
else
  AM_CXXFLAGS = -O2  
endif

scalTest_SOURCES = scal_test.cpp


AM_CPPFLAGS = -g 
AM_LDFLAGS = @CURL_LIBS@
LDADD =
