MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = server extras

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
-I$(top_srcdir)/src/lib/server \
-I$(top_builddir)/src/lib/server \
-I$(top_srcdir)/src/lib/extras \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" \
@EMS_CFLAGS@

includes_HEADERS = Ems.h
includesdir = $(includedir)/enna

lib_LTLIBRARIES = libems.la

libems_la_SOURCES = \
ems_config.c \
ems_main.c \
ems_scanner.c \
ems_config.h \
ems_private.h \
ems_scanner.h \
ems_server.c \
ems_database.c \
ems_database.h \
ems_parser.c \
ems_parser.h \
ems_utils.c \
ems_utils.h \
ems_stream_server.c \
ems_stream_server.h \
extras/http_parser.c

if HAVE_MACOSX
else
libems_la_SOURCES += ems_avahi.c ems_avahi.h
endif


libems_la_LIBADD = server/libems_server.la @EMS_LIBS@
libems_la_LDFLAGS = -no-undefined @lt_enable_auto_import@ -version-info @version_info@ @release_info@
