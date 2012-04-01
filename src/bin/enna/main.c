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

#include <Evas.h>
#include <Elementary.h>
#include <Ems.h>

#include "enna_private.h"
#include "enna_config.h"
#include "enna_mainmenu.h"
#include "enna_activity.h"

#ifndef ELM_LIB_QUICKLAUNCH

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static Evas_Object *win, *ly, *mainmenu;

static void
enna_win_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   elm_exit();
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
   else if (!strcmp(activity, "Videos"))
     {
        Evas_Object *act;
        edje_object_signal_emit(elm_layout_edje_get(ly), "mainmenu,hide", "enna");
        act = enna_activity_add(ly);
        elm_object_part_content_set(ly, "activity.swallow", act);
        evas_object_show(act);
     }

}

static Eina_Bool
enna_window_init(void)
{

   win = elm_win_add(NULL, "enna", ELM_WIN_BASIC);
   elm_win_title_set(win, "Enna Media Center");
   evas_object_smart_callback_add(win, "delete,request", enna_win_del, NULL);

   ly = elm_layout_add(win);
   elm_layout_file_set(ly, enna_config_theme_get(), "main/layout");
   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, ly);

   evas_object_show(ly);

   mainmenu = enna_mainmenu_add(ly);
   elm_object_part_content_set(ly, "mainmenu.swallow", mainmenu);
   evas_object_show(mainmenu);
   evas_object_smart_callback_add(mainmenu, "selected", _mainmenu_item_selected_cb, NULL);

   elm_object_focus_set(mainmenu, EINA_TRUE);


   evas_object_resize(win, 1280, 720);
   evas_object_show(win);

   return EINA_TRUE;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

int _enna_log_dom_global = -1;

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/



EAPI_MAIN int
elm_main(int argc, char **argv)
{

   if (!enna_init())
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
