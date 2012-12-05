MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
@EMS_CFLAGS@

bin_PROGRAMS = enna-media-server emc

enna_media_server_SOURCES = \
enna_media_server.c

enna_media_server_LDADD = \
@EMS_LIBS@ \
$(top_builddir)/src/lib/libems.la

emc_SOURCES = \
emc.c \
cmd_scan.c \
cmd_node.c \
cmd_medias.c

emc_LDADD = \
@EMS_LIBS@ \
$(top_builddir)/src/lib/libems.la


