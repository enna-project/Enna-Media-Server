AM_CFLAGS = \
  -I$(top_srcdir) \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/include/extras \
  $(CC_PIC)
AM_CXXFLAGS = $(AM_CFLAGS) $(CXX_PIC)
LIBADD = -lm

noinst_LTLIBRARIES = \
libcJSON.la

EXTRA_DIST = \
cJSON.h

libcJSON_la_SOURCES = cJSON.c

MAINTAINERCLEANFILES = Makefile.in