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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <Evas.h>
#include <Elementary.h>
#include <Ems.h>

#include "enna_private.h"
#include "enna_config.h"
#include "enna_mainmenu.h"
#include "enna_video.h"

#ifndef ELM_LIB_QUICKLAUNCH

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static void
_win_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   elm_exit();
}

static void
_win_resize(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
    Evas_Coord w, h;

    evas_object_geometry_get(enna->win, NULL, NULL, &w, &h);
    evas_object_resize(enna->ly, w - 2 * enna->app_x_off, h - 2 * enna->app_y_off);
    evas_object_move(enna->ly, enna->app_x_off, enna->app_y_off);
}

static Eina_Bool
enna_init(void)
{
   _enna_log_dom_global = eina_log_domain_register("enna", ENNA_DEFAULT_LOG_COLOR);
   if (_enna_log_dom_global < 0)
     {
        EINA_LOG_ERR("Enna Can not create a general log domain.");
        return EINA_FALSE;
     }

   enna = calloc(1, sizeof(Enna));
   if (!enna)
     {
        EINA_LOG_ERR("Not enough ram ?");
        return EINA_FALSE;
     }

   INF("Enna init done");

   return EINA_TRUE;
}

static void
_mainmenu_item_selected_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   const char *activity = event_info;

   DBG("Selected Activity : %s", activity);

   if (!strcmp(activity, "Exit"))
     elm_exit();
   else
     {
        if (enna_activity_select(activity))
          edje_object_signal_emit(elm_layout_edje_get(enna->ly), "mainmenu,hide", "enna");
     }

}

static Eina_Bool
_elm_win_event_cb(Evas_Object *obj, Evas_Object *src __UNUSED__, Evas_Callback_Type type, void *event_info)
{
   if (type == EVAS_CALLBACK_KEY_DOWN)
     {
        Evas_Event_Key_Down *ev = event_info;

        printf("keyname : %s\n", ev->keyname);

        if (!strcmp(ev->keyname, "Tab"))
          {
             printf("TADADADADA\n");
          }
        else if ((!strcmp(ev->keyname, "Left")) ||
                 (!strcmp(ev->keyname, "KP_Left")))
          {
             printf("left\n");
             elm_widget_focus_cycle(obj, ELM_FOCUS_PREVIOUS);
          }
        else if ((!strcmp(ev->keyname, "Right")) ||
                 (!strcmp(ev->keyname, "KP_Right")))
          {
             printf("right\n");
             elm_widget_focus_cycle(obj, ELM_FOCUS_NEXT);
          }
     }
   return EINA_FALSE;
}

static void
_key_down(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   DBG("Key ev : %s", ev->keyname);

   if ((!strcmp(ev->keyname, "Left")) ||
       (!strcmp(ev->keyname, "KP_Left")))
     {
        elm_widget_focus_cycle(obj, ELM_FOCUS_PREVIOUS);
     }
   else if ((!strcmp(ev->keyname, "Right")) ||
            (!strcmp(ev->keyname, "KP_Right")))
     {
        elm_widget_focus_cycle(obj, ELM_FOCUS_NEXT);
     }
   else if (!strcmp(ev->keyname, "Escape"))
     {
        /* Show Exit Confirmation Menu */
     }
   else if (!strcmp(ev->keyname, "BackSpace"))
     {
        if (!enna->mainmenu)
          {
             /* Back to the mainmenu */
          }
        else
          {
             /* Show Exit Confirmation Menu */
          }
     }
}

static void
_edje_signal_cb(void        *data,
                Evas_Object *obj,
                const char  *emission,
                const char  *source)
{

   //DBG("Edje Object %p receive %s %s", obj, emission, source);
   if (!strcmp(emission, "mainmenu,hide,end"))
     {
        DBG("Mainmenu hide transition end");
     }
}

static Eina_Bool
enna_window_init(void)
{

   enna->win = elm_win_add(NULL, "enna", ELM_WIN_BASIC);
   elm_win_title_set(enna->win, "Enna Media Center");
   evas_object_smart_callback_add(enna->win, "delete,request", _win_del, NULL);
   evas_object_event_callback_add(enna->win, EVAS_CALLBACK_RESIZE, _win_resize, NULL);
   enna->ly = elm_layout_add(enna->win);
   evas_object_event_callback_add(enna->ly, EVAS_CALLBACK_KEY_DOWN,
                                  _key_down, enna->win);
   edje_object_signal_callback_add(elm_layout_edje_get(enna->ly), "*", "*",
                                    _edje_signal_cb, NULL);

   elm_layout_file_set(enna->ly, enna_config_theme_get(), "main/layout");
   evas_object_size_hint_weight_set(enna->ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   //elm_win_resize_object_add(win, enna->ly);

   evas_object_show(enna->ly);

   enna->mainmenu = enna_mainmenu_add(enna->ly);
   elm_object_part_content_set(enna->ly, "mainmenu.swallow", enna->mainmenu);
   evas_object_show(enna->mainmenu);
   evas_object_smart_callback_add(enna->mainmenu, "selected", _mainmenu_item_selected_cb, NULL);

   elm_object_focus_set(enna->mainmenu, EINA_TRUE);

   if (!enna->app_w || !enna->app_h)
     {
        enna->app_w = 1280;
        enna->app_h = 720;
     }
   evas_object_resize(enna->win, enna->app_w, enna->app_h);

   evas_object_resize(enna->ly, enna->app_w - 2 * enna->app_x_off, enna->app_h - 2 * enna->app_y_off);
   evas_object_move(enna->ly, enna->app_x_off, enna->app_y_off);

   evas_object_show(enna->win);

   enna_video_init();

   return EINA_TRUE;
}

static void
_opt_geometry_parse(const char *opt,
                    Evas_Coord *pw, Evas_Coord *ph,
                    Evas_Coord *px, Evas_Coord *py)
{
   int w = 0, h = 0;
   int x = 0, y = 0;
   int ret;

   ret = sscanf(opt, "%dx%d:%d:%d", &w, &h, &x, &y);

   if ( ret != 2 && ret != 4 )
     return;

   if (pw) *pw = w;
   if (ph) *ph = h;
   if (px) *px = x;
   if (py) *py = y;
}

static void _usage_print(char *binname)
{
   printf("Enna Media Center\n");
   printf(" Usage: %s [options ...]\n", binname);
   printf(" Available options:\n");
   printf("  -c, (--config):  Specify configuration file to be used.\n");
   printf("  -f, (--fs):      Force fullscreen mode.\n");
   printf("  -h, (--help):    Display this help.\n");
   printf("  -t, (--theme):   Specify theme name to be used.\n");
   printf("  -g, (--geometry):Specify window geometry. (geometry=1280x720)\n");
   printf("  -g, (--geometry):Specify window geometry and offset. (geometry=1280x720:10:20)\n");
   printf("\n");
   printf("  -V, (--version): Display Enna version number.\n");
   exit(EXIT_SUCCESS);
}

static void _version_print(void)
{
   printf(PACKAGE_STRING"\n");
   exit(EXIT_SUCCESS);
}

static int _parse_command_line(int argc, char **argv)
{
   int c, id;
   char short_options[] = "Vhfc:t:b:g:p:";
   struct option long_options [] =
     {
       { "help",          no_argument,       0, 'h' },
       { "version",       no_argument,       0, 'V' },
       { "fs",            no_argument,       0, 'f' },
       { "config",        required_argument, 0, 'c' },
       { "theme",         required_argument, 0, 't' },
       { "geometry",      required_argument, 0, 'g' },
       { 0,               0,                 0,  0  }
     };

   /* command line argument processing */
   while (1)
     {
        c = getopt_long(argc, argv, short_options, long_options, &id);

        if (c == EOF)
          break;

        switch (c)
          {
           case 0:
              /* opt = long_options[index].name; */
              break;

           case '?':
           case 'h':
              _usage_print(argv[0]);
              return -1;

           case 'V':
              _version_print();
              break;

           case 'f':
              enna->run_fullscreen = 1;
              break;

           case 'c':
              enna->config_file = eina_stringshare_add(optarg);
              break;

           case 't':
              enna->theme_file = eina_stringshare_add(optarg);
              break;

           case 'g':
              _opt_geometry_parse(optarg, &enna->app_w, &enna->app_h, &enna->app_x_off, &enna->app_y_off);
              break;
           default:
              _usage_print(argv[0]);
              return -1;
          }
     }

   return 0;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

int _enna_log_dom_global = -1;

Enna *enna = NULL;

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/



EAPI_MAIN int
elm_main(int argc, char **argv)
{

   if (!enna_init())
     goto shutdown;

   if (_parse_command_line(argc, argv) < 0)
     goto shutdown;

   if (!enna_config_init())
     goto shutdown;

   if (!ems_init(enna_config_config_get()))
     return EXIT_FAILURE;

   if (!enna_window_init())
     goto shutdown_config;

   INF("Start scanner");
   ems_scanner_start();
   INF("Start services annoucement");
   ems_avahi_start();

   ems_run();

   ems_shutdown();
   elm_shutdown();

   return EXIT_SUCCESS;

 shutdown_config:
   enna_config_shutdown();
 shutdown:
   return -1;
}
#endif
ELM_MAIN()
