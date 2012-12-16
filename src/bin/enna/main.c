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

#include <Ecore_Getopt.h>
#include <Elementary.h>
#include <Ems.h>

#include "enna_private.h"
#include "enna_config.h"
#include "enna_main.h"

#ifndef ELM_LIB_QUICKLAUNCH

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static const Ecore_Getopt _options =
{
   "enna",
   "%prog [options]",
   VERSION,
   "(C) 2012 Enna Team",
   "BSD",
   "Enna - a powerfull mediacenter based on EFL and Enna Media Server",
   EINA_TRUE,
   {
      ECORE_GETOPT_STORE_DEF_BOOL
      ('f', "fullscreen", "Force fullscreen mode.", EINA_FALSE),
      ECORE_GETOPT_CALLBACK_ARGS
      ('g', "geometry", "geometry to use in x:y:w:h form.", "X:Y:W:H", ecore_getopt_callback_geometry_parse, NULL),
      ECORE_GETOPT_VERSION
      ('V', "version"),
      ECORE_GETOPT_COPYRIGHT
      ('R', "copyright"),
      ECORE_GETOPT_LICENSE
      ('L', "license"),
      ECORE_GETOPT_HELP
      ('h', "help"),
      ECORE_GETOPT_STORE_STR
      ('t', "theme", "Specify theme file to be used."),
      ECORE_GETOPT_SENTINEL
   }
};

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   char *theme = NULL;
   int args;
   unsigned char quit_option = 0;
   Eina_Rectangle geometry = {0, 0, 0, 0};
   Eina_Bool is_fullscreen = 0;
   Enna *enna;

   Ecore_Getopt_Value values[] =
   {
      ECORE_GETOPT_VALUE_BOOL(is_fullscreen),
      ECORE_GETOPT_VALUE_PTR_CAST(geometry),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_BOOL(quit_option),
      ECORE_GETOPT_VALUE_STR(theme),
      ECORE_GETOPT_VALUE_NONE
   };

   eina_init();

   ecore_app_args_set(argc, (const char **) argv);
   args = ecore_getopt_parse(&_options, values, argc, argv);

   if (args < 0)
     {
        ERR("Could not parse options !");
        return EXIT_FAILURE;
     }

   if (quit_option)
     return EXIT_SUCCESS;

   if (!enna_init())
     goto shutdown;

   if (!enna_config_init())
     goto shutdown_enna;

   if (theme)
     enna_config_theme_set(theme);

   if (!ems_init(enna_config_config_get(), EINA_FALSE))
     goto shutdown_config;

   enna = enna_add(is_fullscreen, geometry);
   if (!enna)
     goto shutdown_config;

   INF("Start scanner");
   ems_scanner_start();
   INF("Start services annoucement");
   ems_avahi_start();

   ems_run();

   ems_shutdown();
   enna_config_shutdown();
   enna_shutdown();
   elm_shutdown();

   return EXIT_SUCCESS;

 shutdown_config:
   enna_config_shutdown();
 shutdown_enna:
   enna_shutdown();
 shutdown:
   enna_shutdown();
   return -1;
}
#endif
ELM_MAIN()
