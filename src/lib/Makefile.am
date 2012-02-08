MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EMS_CFLAGS@

lib_LTLIBRARIES = libems.la

libems_la_SOURCES = \
ems_avahi.c \
ems_config.c \
ems_main.c

libems_la_LIBADD = \
@EMS_LIBS@


