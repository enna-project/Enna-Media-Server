MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EMS_CFLAGS@

bin_PROGRAMS = enna-media-server

enna_media_server_SOURCES = \
main.c \
ems_config.c

enna_media_server_LDADD = \
@EMS_LIBS@

