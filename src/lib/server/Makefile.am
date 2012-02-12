MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
@EMS_CFLAGS@

noinst_LTLIBRARIES = libems_server.la

SERVER_SOURCES = \
ems_rpc_Config.azy_server.c \
ems_rpc_Config.azy_server.h \
ems_rpc_Browser.azy_server.c \
ems_rpc_Browser.azy_server.h \
ems_rpc_Common_Azy.c \
ems_rpc_Common_Azy.h \
ems_rpc_Common.c \
ems_rpc_Common.h

BUILT_SOURCES = $(SERVER_SOURCES)
CLEANFILES = $(SERVER_SOURCES)

libems_server_la_SOURCES = $(SERVER_SOURCES)


$(SERVER_SOURCES): services.azy
	@echo  "  GEN    $@"
	@azy_parser -H -p -o ./ services.azy

EXTRA_DIST = services.azy
