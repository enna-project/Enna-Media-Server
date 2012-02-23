MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EMS_CFLAGS@

bin_PROGRAMS = test_avahi_service

test_avahi_service_SOURCES = \
avahi_service.c

test_avahi_service_LDADD = \
@EMS_LIBS@