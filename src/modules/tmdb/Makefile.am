MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_builddir)/src/include \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
@EMS_CFLAGS@

controllerdir = $(libdir)/ems/grabbers/$(MODULE_ARCH)
controller_LTLIBRARIES = module.la

module_la_SOURCES = \
ems_grabber_tmdb.c

module_la_CFLAGS = @EMS_CFLAGS@
module_la_LIBADD = $(top_builddir)/src/lib/libems.la @EMS_LIBS@
module_la_LDFLAGS = -no-undefined @lt_enable_auto_import@ -module -avoid-version
module_la_LIBTOOLFLAGS = --tag=disable-static

install-data-hook:
	rm -f $(DESTDIR)$(controllerdir)/$(controller_LTLIBRARIES)
uninstall-hook:
	rm -f $(DESTDIR)$(controllerdir)/module.*

clean-local:
	rm -rf *.gcno