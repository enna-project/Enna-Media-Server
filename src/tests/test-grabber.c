/*  Media Server - a library and daemon for medias indexation and streaming
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

#include "ems_private.h"

static void
_end_grab_cb(void *data __UNUSED__, const char *filename __UNUSED__)
{
   DBG("End Grab");
   ecore_main_loop_quit();
}


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

int main(int argc __UNUSED__, char **argv)
{
   Eina_Module *m;
   char tmp[PATH_MAX];

   void (*grab)(const char *filename, Ems_Media_Type type,
		void (*Ems_Grabber_End_Cb)(void *data, const char *filename),
		void *data
		);

   ems_init(NULL);
   eina_init();
   ecore_init();
   ecore_con_init();
   ecore_con_url_init();

   DBG("%s init", argv[0]);


   if (!argv[1])
     {
        printf("USAGE : %s grabber_name", argv[0]);
        exit(0);
     }

   DBG("Try to load %s", argv[1]);
   DBG("Searh for modules in %s with arch %s", PACKAGE_LIB_DIR "/ems/grabbers", MODULE_ARCH);

   snprintf(tmp, sizeof(tmp), PACKAGE_LIB_DIR"/ems/grabbers/%s/%s/module.so", argv[1], MODULE_ARCH);
   DBG("Complete path module %s", tmp);
   m = eina_module_new(tmp);

   eina_module_load(m);

   grab = eina_module_symbol_get(m, "ems_grabber_grab");
   if (grab)
     {
        DBG("Grab file");
        grab(argv[2],
             1,
             _end_grab_cb, NULL);
     }



   ecore_main_loop_begin();

   eina_module_free(m);

   ems_shutdown();

   return EXIT_SUCCESS;
}
