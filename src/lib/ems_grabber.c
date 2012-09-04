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
#include <Ecore.h>

#include "Ems.h"
#include "ems_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static Eina_Array *_modules = NULL;
static Eina_List *_files = NULL;
static Ecore_Idler *_queue_idler = NULL;

static void _end_grab_cb(void *data, const char *filename, Ems_Grabber_Data *grabbed_data);

static Eina_Bool
_idler_cb(void *data)
{
   Eina_Array_Iterator iterator;
   Eina_Module *m;
   unsigned int i;
   Eina_List *l;
   Ems_Grabber_Params *file;
   const char *filename = data;
   void (*grab)(Ems_Grabber_Params *params,
                void (*Ems_Grabber_End_Cb)(void *data, const char *filename, Ems_Grabber_Data *grabbed_data),
                void *data
                );

   if (filename)
     {
        EINA_LIST_FOREACH(_files, l, file)
          {
             if (!strcmp(file->filename, filename))
               {
                  _files = eina_list_remove(_files, file);
                  eina_stringshare_del(file->filename);
                  eina_stringshare_del(file->search);
                  free(file);
                  break;
               }
          }
     }

   DBG("Still %d to grab", eina_list_count(_files));
   if (eina_list_count(_files))
     {
        EINA_ARRAY_ITER_NEXT(_modules, i, m, iterator)
          {

             grab = eina_module_symbol_get(m, "ems_grabber_grab");
             if (grab)
               {
                  Ems_Grabber_Params *f = eina_list_nth(_files, 0);
                  grab(f, _end_grab_cb, NULL);
               }
             break;
          }
     }
   else
     {
        DBG("%s was the last file to parse", filename);
        _queue_idler = NULL;
        void (*stats)(void);
        EINA_ARRAY_ITER_NEXT(_modules, i, m, iterator)
          {
             stats = eina_module_symbol_get(m, "ems_grabber_stats");
             if (stats)
               stats();
             break;
          }
     }

   return EINA_FALSE;
}

static Eina_Bool _print_hash_cb(const Eina_Hash *hash, const void *key,
                  void *data, void *fdata)
{
   Eina_Value *v = data;
   INF("Func data: Hash entry: [%s] -> %s", key, eina_value_to_string(v));
   ems_database_meta_insert(fdata, key, eina_value_to_string(v));
   return 1;
}

static void
_end_grab_cb(void *data __UNUSED__, const char *filename, Ems_Grabber_Data *grabbed_data)
{

   DBG("End Grab %s", filename);
   DBG("Grabbed data : %p", grabbed_data);
   INF("Grabber language : %s", grabbed_data->lang);
   INF("Grabber grabbed date : %lld", grabbed_data->date);
   INF("Data:");
   eina_hash_foreach(grabbed_data->data, _print_hash_cb, NULL);


   if (grabbed_data->episode_data)
     {
        INF("Episode data:");
        eina_hash_foreach(grabbed_data->episode_data, _print_hash_cb, filename);
     }

   _queue_idler = ecore_idler_add(_idler_cb, filename);
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
ems_grabber_init(void)
{
   _modules = eina_module_arch_list_get(NULL,
                                        PACKAGE_LIB_DIR "/ems/grabbers",
                                        MODULE_ARCH);

   eina_module_list_load(_modules);

   return EINA_TRUE;
}

void
ems_grabber_shutdown(void)
{
   Ems_Grabber_Params *file;

   EINA_LIST_FREE(_files, file)
     {
        eina_stringshare_del(file->search);
        eina_stringshare_del(file->filename);
        free(file);
     }

   if (_queue_idler)
     ecore_idler_del(_queue_idler);

   eina_module_list_free(_modules);
   if (_modules)
      eina_array_free(_modules);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
void ems_grabber_grab(const char *filename, const char *search, Ems_Media_Type type, unsigned int season, unsigned int episode)
{
   Ems_Grabber_Params *file;


   file = calloc(1, sizeof(Ems_Grabber_Params));
   file->filename = eina_stringshare_add(filename);
   file->search = eina_stringshare_add(search);
   file->type = type;
   file->season = season;
   file->episode = episode;
   _files = eina_list_append(_files, file);
   if (!_queue_idler)
     _queue_idler = ecore_idler_add(_idler_cb, NULL);

}


