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
#include "ems_utils.h"

static Eina_Bool _print_hash_cb(const Eina_Hash *hash, const void *key,
                  void *data, void *fdata)
{
   INF("Func data: Hash entry: [%s] -> %s", key, data);
   return 1;
}

static void
_end_grab_cb(void *data __UNUSED__, const char *filename __UNUSED__, Ems_Grabber_Data *grabbed_data)
{
   DBG("End Grab %s", filename);
   DBG("Grabbed data : %p", grabbed_data);
   INF("Grabber language : %s", grabbed_data->lang);
   INF("Grabber grabbed date : %lld", grabbed_data->date);
   eina_hash_foreach(grabbed_data->data, _print_hash_cb, NULL);
   ecore_main_loop_quit();
}


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

int main(int argc, char **argv)
{
   Eina_Module *m;
   char tmp[PATH_MAX];
   Ems_Media_Type type = 1;
   Ems_Grabber_Grab grab;
   char *title;
   Ems_Grabber_Params params;

   if (argc != 4)
     {
	printf("USAGE : %s grabber_name filename media_type\n", argv[0]);
	exit(-1);
     }

   ems_init(NULL);
   eina_init();
   ecore_init();
   ecore_con_init();
   ecore_con_url_init();

   DBG("Try to load %s", argv[1]);
   DBG("Searh for modules in %s with arch %s", PACKAGE_LIB_DIR "/ems/grabbers", MODULE_ARCH);

   snprintf(tmp, sizeof(tmp), PACKAGE_LIB_DIR"/ems/grabbers/%s/%s/module.so", argv[1], MODULE_ARCH);
   m = eina_module_new(tmp);

   if (!eina_module_load(m))
     {
       ERR("unable to load module (%s): %s", tmp, eina_error_msg_get(eina_error_get()));
     }

   grab = eina_module_symbol_get(m, "ems_grabber_grab");
   if (grab)
     {
        INF("Grab title %s", argv[2]);
        type = atoi(argv[3]);

        title = ems_utils_decrapify(argv[2], &params.season, &params.episode);

        grab(title,
             type,
             params,
             _end_grab_cb, NULL);

        free(title);
     }


   ecore_main_loop_begin();

   eina_module_free(m);

   ems_shutdown();

   return EXIT_SUCCESS;
}
