/* Enna Media Server - a library and daemon for medias indexation and streaming
 *
 * Copyright (C) 2012 Enna Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.h>
#include <Evas.h>
#include <Elementary.h>
#include <Ems.h>

#include "enna_private.h"
#include "enna_main.h"
#include "enna_activity.h"
#include "enna_config.h"
#include "enna_view_video_list.h"
#include "enna_view_news.h"
#include "enna_exit_popup.h"
#include "enna_mainmenu.h"
#include "enna_view_player_video.h"
#include "enna_view_music_grid.h"
#include "enna_keyboard.h"
#include "enna_input.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static int _enna_init_count = 0;
static Eina_List *_enna_list = NULL;

static void
_win_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   elm_exit();
}

static void
_win_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
    Evas_Coord w, h;
    Enna *enna;
    enna = (Enna *)data;

    evas_object_geometry_get(enna->win, NULL, NULL, &w, &h);
    evas_object_resize(enna->layout, w - 2 * enna->app_x_off, h - 2 * enna->app_y_off);
    evas_object_move(enna->layout, enna->app_x_off, enna->app_y_off);
}

static void
_mainmenu_item_selected_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   const char *activity = event_info;
   Enna *enna;
   enna = (Enna *)data;

   DBG("Selected Activity : %s", activity);

   if (!strcmp(activity, "Exit"))
     elm_exit();
   else
     {
        if (!enna_activity_select(enna, activity))
          {
             ERR("Can't select activity !");
          }
     }

}

static void
_edje_signal_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *emission, const char *source __UNUSED__)
{
   //DBG("Edje Object %p receive %s %s", obj, emission, source);
   if (!strcmp(emission, "mainmenu,hide,end"))
     {
        DBG("Mainmenu hide transition end");
     }
}

static Eina_Bool
_input_event(void *data, Enna_Input event)
{
   Enna *enna = data;
   const char *tmp;

   tmp = enna_keyboard_input_name_get(event);
   INF("main input event %s", tmp);

   switch (event)
     {
      case ENNA_INPUT_QUIT:
         /* Show Exit Confirmation Menu */
         enna_exit_popup_show(enna, enna->win);
         break;
      case ENNA_INPUT_BACK:
         if (elm_object_item_widget_get(elm_naviframe_top_item_get(enna->naviframe)) != enna->mainmenu)
           /* Back to the mainmenu */
           elm_naviframe_item_pop(enna->naviframe);
         else
           /* Show Exit Confirmation Menu */
           enna_exit_popup_show(enna, enna->win);
         break;
      default:
         break;
     }

   return ENNA_EVENT_BLOCK;
}

static Eina_Bool
_enna_window_init(Enna *enna)
{
   Eina_Bool shaped = EINA_FALSE;
   const char *sshaped = NULL;
   Evas_Coord minw, minh;

   enna->win = elm_win_add(NULL, "enna", ELM_WIN_BASIC);
   elm_win_title_set(enna->win, "Enna Media Center");
   evas_object_smart_callback_add(enna->win, "delete,request", _win_del, enna);
   evas_object_event_callback_add(enna->win, EVAS_CALLBACK_RESIZE, _win_resize, enna);
   /*
    * create the naviframe, it's the main element that will handle all our subviews.
    * the first subview to be added is the mainmenu
    */
   enna->naviframe = elm_naviframe_add(enna->win);
//   elm_object_style_set(enna->naviframe, "enna");
   evas_object_show(enna->naviframe);

   /* Add Main input listener, it reacts to all keys wich are not handled by other view */
   enna->il = enna_input_listener_add("enna_main", _input_event, enna);

   enna->mainmenu = enna_activity_select(enna, "MainMenu");
   evas_object_smart_callback_add(enna->mainmenu, "selected", _mainmenu_item_selected_cb, enna);
   elm_object_focus_set(enna->mainmenu, EINA_TRUE);

   enna->layout = elm_layout_add(enna->win);
   edje_object_signal_callback_add(elm_layout_edje_get(enna->layout), "*", "*",
                                    _edje_signal_cb, enna);

   elm_layout_file_set(enna->layout, enna_config_theme_get(), "main/layout");
   evas_object_size_hint_weight_set(enna->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(enna->layout);

   elm_object_part_content_set(enna->layout, "activity.swallow", enna->naviframe);

   sshaped = elm_layout_data_get(enna->layout, "shaped");

   if (sshaped)
     shaped = !strcmp(sshaped, "1");

   elm_win_shaped_set(enna->win, shaped);
   elm_win_borderless_set(enna->win, shaped);
   elm_win_alpha_set(enna->win, shaped);
   elm_win_fullscreen_set(enna->win, enna->run_fullscreen);

   if (!enna->app_w || !enna->app_h)
     {
        edje_object_size_min_get(elm_layout_edje_get(enna->layout), &minw, &minh);
        if (!minw || !minh)
          {
             enna->app_w = 1280;
             enna->app_h = 720;
          }
        else
          {
             enna->app_w = minw;
             enna->app_h = minh;
          }
     }
   evas_object_resize(enna->win, enna->app_w, enna->app_h);

   evas_object_resize(enna->layout, enna->app_w - 2 * enna->app_x_off, enna->app_h - 2 * enna->app_y_off);
   evas_object_move(enna->layout, enna->app_x_off, enna->app_y_off);

   evas_object_show(enna->win);

   return EINA_TRUE;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

int _enna_log_dom_global = -1;

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

int
enna_init(void)
{
   if (++_enna_init_count != 1)
     return _enna_init_count;

   _enna_log_dom_global = eina_log_domain_register("enna", ENNA_DEFAULT_LOG_COLOR);
   if (_enna_log_dom_global < 0)
     {
        EINA_LOG_ERR("Enna Can not create a general log domain.");
        return EINA_FALSE;
     }

   enna_keyboard_init();

   enna_activity_init();

   //Register all activities here
   enna_activity_register("MainMenu", enna_mainmenu_add);
   enna_activity_register("Videos", enna_view_video_list_add);
   enna_activity_register("Music", enna_view_music_grid_add);
   enna_activity_register("VideoPlayer", enna_view_player_video_add);
   enna_activity_register("News", enna_view_news_add);

   INF("Enna init done");

   return _enna_init_count;
}

void
enna_shutdown(void)
{
   Eina_List *l, *ll;
   Enna *e;

   if (--_enna_init_count != 0)
     return;

   DBG("Enna shutdown");

   EINA_LIST_FOREACH_SAFE(_enna_list, l, ll, e)
     enna_del(e);

   eina_log_domain_unregister(_enna_log_dom_global);
   _enna_log_dom_global = -1;
}

Enna *
enna_add(Eina_Bool is_fullscreen, Eina_Rectangle geometry)
{
   Enna *enna;

   enna = calloc(1, sizeof(Enna));
   if (!enna)
     {
        CRIT("Not enough ram ?");
        return NULL;
     }

   if (!_enna_window_init(enna))
     {
        CRIT("Failed to create Enna window !");
        free(enna);

        return NULL;
     }

   enna->run_fullscreen = is_fullscreen;
   enna->app_w = geometry.w;
   enna->app_h = geometry.h;
   enna->app_x_off = geometry.x;
   enna->app_y_off = geometry.y;

   INF("Enna add done");

   _enna_list = eina_list_append(_enna_list, enna);

   return enna;
}

void
enna_del(Enna *enna)
{
   if (!eina_list_data_find(_enna_list, enna))
     {
        ERR("Wrong Enna pointer !");

        return;
     }

   _enna_list = eina_list_remove(_enna_list, enna);
   if (enna->mainmenu)
     evas_object_del(enna->mainmenu);
   if (enna->naviframe)
     evas_object_del(enna->naviframe);
   if (enna->win)
     evas_object_del(enna->win);
   free(enna);
}
