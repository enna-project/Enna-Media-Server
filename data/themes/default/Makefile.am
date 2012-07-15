MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = edje_cc

EDJE_FLAGS = \
-id $(top_srcdir)/data/themes/default/images \
-id $(top_srcdir)/data/themes/default/elm/images \
-fd $(top_srcdir)/data/themes/default/fonts

EDJE_VERB = $(EDJE_VERB_$(V))
EDJE_VERB_ = $(EDJE_VERB_$(AM_DEFAULT_VERBOSITY))
EDJE_VERB_1 = -v

defaultdir = $(pkgdatadir)/theme

default_DATA = default.edj

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
elm/images/tree_arrow_down.png \
images/player_osd.png \
images/player_play.png \
images/player_pause.png \
images/player_slider_bg.png \
images/player_slider_inside.png \
images/player_slider_knob.png \
images/player_slider_out.png \
images/player_slider_time.png


EDC = \
default.edc \
player.edc \
elm/button.edc \
elm/elm.edc \
elm/genlist.edc \
elm/scroller.edc \
elm/video.edc

EXTRA_DIST = \
  $(IMG) \
  $(EDC)

default.edj: Makefile $(EXTRA_DIST)
	$(AM_V_GEN)$(EDJE_CC) $(EDJE_VERB) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default/default.edc \
	$(top_builddir)/data/themes/default/default.edj

clean-local:
	rm -f *.edj

