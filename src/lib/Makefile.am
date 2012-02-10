MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EMS_CFLAGS@

includes_HEADERS = Ems.h
includesdir = $(includedir)/enna

lib_LTLIBRARIES = libems.la

SERVER_SOURCES = \
EMS_Config.azy_server.c \
EMS_Config.azy_server.h \
EMS_Common_Azy.c \
EMS_Common_Azy.h \
EMS_Common.c \
EMS_Common.h

BUILT_SOURCES = $(SERVER_SOURCES)
CLEANFILES = $(SERVER_SOURCES)

libems_la_SOURCES = \
ems_avahi.c \
ems_config.c \
ems_main.c \
ems_server.c \
$(SERVER_SOURCES)



$(SERVER_SOURCES): server/services.azy
	@echo  "  GEN    $@"
	@azy_parser -H -p -o ./ server/services.azy

libems_la_LIBADD = \
@EMS_LIBS@

EXTRA_DIST = ems_private.h \
	ems_config.h \
	server/services.azy


