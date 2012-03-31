#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <Evas.h>
#include <Elementary.h>
#include <Ems.h>

#include "enna_config.h"
#include "enna_private.h"

#ifndef ELM_LIB_QUICKLAUNCH

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

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

static Eina_Bool
enna_window_init(void)
{
   Evas_Object *win, *ly;

   win = elm_win_add(NULL, "enna", ELM_WIN_BASIC);
   elm_win_title_set(win, "Enna Media Center");
   evas_object_smart_callback_add(win, "delete,request", enna_win_del, NULL);

   ly = elm_layout_add(win);
   elm_layout_file_set(ly, enna_config_theme_get(), "main/layout");
   elm_win_resize_object_add(win, ly);
   evas_object_show(ly);

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
