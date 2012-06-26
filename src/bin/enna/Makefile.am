MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/enna-media-server\" \
-I$(top_builddir)/src/lib \
@ENNA_CFLAGS@

bin_PROGRAMS = enna

enna_SOURCES = \
main.c \
enna_activity.c \
enna_activity.h \
enna_config.c \
enna_config.h \
enna_exit_popup.c \
enna_exit_popup.h \
enna_mainmenu.c \
enna_mainmenu.h \
enna_music.c \
enna_music.h \
enna_private.h \
enna_video.c \
enna_video.h \
enna_view_music_grid.c \
enna_view_music_grid.h \
enna_view_video_list.c \
enna_view_video_list.h

enna_LDADD = \
@ENNA_LIBS@ \
$(top_builddir)/src/lib/libems.la

