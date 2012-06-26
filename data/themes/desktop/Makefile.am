MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = edje_cc

EDJE_FLAGS = \
-id $(top_srcdir)/data/themes/desktop/images \
-id $(top_srcdir)/data/themes/desktop/elm/images \
-fd $(top_srcdir)/data/themes/desktop/fonts

EDJE_VERB = $(EDJE_VERB_$(V))
EDJE_VERB_ = $(EDJE_VERB_$(AM_DEFAULT_VERBOSITY))
EDJE_VERB_1 = -v

desktopdir = $(pkgdatadir)/theme

desktop_DATA = desktop.edj

IMG = \
images/background.png \
elm/images/list_bg.png \
elm/images/list_selected.png \
elm/images/scroll_bg_h.png \
elm/images/scroll_bg_v.png \
elm/images/scroll_h.png \
elm/images/scroll_v.png \
elm/images/tree_arrow_left.png \
elm/images/tree_arrow_right.png \
elm/images/tree_arrow_down.png


EDC = \
desktop.edc \
icons.edc \
elm/button.edc \
elm/elm.edc \
elm/genlist.edc \
elm/gengrid.edc \
elm/scroller.edc

EXTRA_DIST = \
  $(IMG) \
  $(EDC)

desktop.edj: Makefile $(EXTRA_DIST)
	$(AM_V_GEN)$(EDJE_CC) $(EDJE_VERB) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/desktop/desktop.edc \
	$(top_builddir)/data/themes/desktop/desktop.edj

clean-local:
	rm -f *.edj

