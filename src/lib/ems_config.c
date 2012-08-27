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

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <Eet.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include "ems_private.h"
#include "ems_config.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static Eet_Data_Descriptor *conf_edd = NULL;
static Eet_Data_Descriptor *directory_edd = NULL;

static const char *
ems_config_filename_get(void)
{
   static const char *filename = NULL;
   char tmp[PATH_MAX];

   if (filename)
     return filename;

   snprintf(tmp, sizeof(tmp), "%s/.config/enna-media-server/%s", getenv("HOME"), EMS_CONFIG_FILE);
   filename =  eina_stringshare_add(tmp);

   DBG("CONFIG filename : %s", filename);

   return filename;
}

static const char *
ems_config_dirname_get(void)
{
   static const char *dir_name = NULL;
   char *tmp;
   if (dir_name)
       return dir_name;

   tmp = ecore_file_dir_get(ems_config_filename_get());
   dir_name = eina_stringshare_add(tmp);
   free(tmp);

   return tmp;
}


static const char *
ems_config_tmp_filename_get(void)
{
   static const char *filename = NULL;
   char tmp[PATH_MAX];

   if (filename)
     return filename;

   if (!filename)
     {
        filename = getenv("TMPDIR");
        if (!filename) filename = "/tmp";
     }

   snprintf(tmp, sizeof(tmp), "%s/enna-media-server/%s", filename, EMS_CONFIG_FILE);
   filename = eina_stringshare_add(tmp);

   DBG("TMP filename : %s", filename);

   return filename;
}

static const char *
ems_config_tmp_dirname_get(void)
{
   static const char *dir_name = NULL;
   char *tmp;
   if (dir_name)
       return dir_name;

   tmp = ecore_file_dir_get(ems_config_tmp_filename_get());
   dir_name = eina_stringshare_add(tmp);
   free(tmp);

   return tmp;
}

static const char *
ems_config_default_filename_get(void)
{
   static const char *filename = NULL;
   char tmp[PATH_MAX];

   if (filename)
     return filename;

   snprintf(tmp, sizeof(tmp), "%s/%s", PACKAGE_DATA_DIR, EMS_CONFIG_FILE);
   filename = eina_stringshare_add(tmp);

   return filename;
}

static Eet_Data_Descriptor *
ems_config_descriptor_new(const char *name, int size)
{
   Eet_Data_Descriptor_Class eddc;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc), name, size))
     return NULL;

   return eet_data_descriptor_stream_new(&eddc);
}


static void
_make_config(const char *config_file)
{
   Eet_File *ef;
   FILE *f;
   int textlen;
   char *text;
   const char *config;

   if (!ecore_file_is_dir(ems_config_tmp_dirname_get()))
     ecore_file_mkpath(ems_config_tmp_dirname_get());

   if (config_file)
     {
        config = config_file;
     }
   else
     {
        if (!ecore_file_is_dir(ems_config_dirname_get()))
          ecore_file_mkpath(ems_config_dirname_get());
        config = ems_config_filename_get();
     }

   INF("Config file : %s", config);

   ef = eet_open(ems_config_tmp_filename_get(),
                 EET_FILE_MODE_READ_WRITE);
   if (!ef)
     ef = eet_open(ems_config_tmp_filename_get(),
                   EET_FILE_MODE_WRITE);

   f = fopen(config, "rb");
   if (!f)
     {
        WRN("Could not open '%s', setup default config.", ems_config_filename_get());
        if (ecore_file_exists(ems_config_default_filename_get()))
          {
             ecore_file_cp(ems_config_default_filename_get(), ems_config_filename_get());
             f = fopen(ems_config_filename_get(), "rb");
             if (!f)
               return;
          }
        else
          return;
     }

   fseek(f, 0, SEEK_END);
   textlen = ftell(f);
   rewind(f);
   text = (char *)malloc(textlen);
   if (!text)
     {
        fclose(f);
        eet_close(ef);
        return;
     }

   if (fread(text, textlen, 1, f) != 1)
     {
        free(text);
        fclose(f);
        eet_close(ef);
        return;
     }

   fclose(f);
   if (eet_data_undump(ef, "config", text, textlen, 1))
     INF("Updating configuration %s", config);
   free(text);
   eet_close(ef);
}

static Ems_Config *
_config_get(Eet_Data_Descriptor *edd)
{
   Ems_Config *config = NULL;
   Eet_File *file;

   if (!ecore_file_is_dir(ems_config_tmp_dirname_get()))
     ecore_file_mkdir(ems_config_tmp_dirname_get());
   file = eet_open(ems_config_tmp_filename_get(),
                   EET_FILE_MODE_READ_WRITE);

   config = eet_data_read(file, edd, "config");
   if (!config)
     {
        WRN("Warning no configuration found! This must not happen, we will go back to a void configuration");
        return NULL;
     }

   eet_close(file);

   return config;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

const char *
ems_config_cache_dirname_get(void)
{
   static const char *dir_name = NULL;
   char tmp[PATH_MAX];

   if (dir_name)
       return dir_name;

   if (!ems_config)
     return NULL;

   /* cache path is not set or doesn't exist : save cache in $HOME/.cache/enna-media-server */
   if (!ems_config->cache_path || !ems_config->cache_path[0] || !ecore_file_exists(ems_config->cache_path))
     {
        snprintf(tmp, sizeof(tmp), "%s/.cache/enna-media-server", getenv("HOME"));
        WRN("Cache directory is not set, save cache data in %s", tmp);
     }
   else
     {
        /* create a directory enna-media-server into cache specified in config */
        snprintf(tmp, sizeof(tmp), "%s/enna-media-server", ems_config->cache_path);
        INF("Save cache data in %s\n", tmp);
     }

   if (!ecore_file_is_dir(tmp))
     ecore_file_mkpath(tmp);

   dir_name = eina_stringshare_add(tmp);
   return dir_name;
}

Eina_Bool
ems_config_init(const char *config_file)
{
   directory_edd = ENNA_CONFIG_DD_NEW("Ems_Directory",
                                      Ems_Directory);
#undef T
#undef D
#define T Ems_Directory
#define D directory_edd
   ENNA_CONFIG_VAL(D, T, path, EET_T_STRING);
   ENNA_CONFIG_VAL(D, T, label, EET_T_STRING);
   ENNA_CONFIG_VAL(D, T, type, EET_T_UINT);

   conf_edd = ENNA_CONFIG_DD_NEW("config", Ems_Config);
#undef T
#undef D
#define T Ems_Config
#define D conf_edd
   ENNA_CONFIG_VAL(D, T, version, EET_T_UINT);
   ENNA_CONFIG_VAL(D, T, port, EET_T_SHORT);
   ENNA_CONFIG_VAL(D, T, port_stream, EET_T_SHORT);
   ENNA_CONFIG_VAL(D, T, name, EET_T_STRING);
   ENNA_CONFIG_LIST(D, T, places, directory_edd);
   ENNA_CONFIG_VAL(D, T, video_extensions, EET_T_STRING);
   ENNA_CONFIG_VAL(D, T, music_extensions, EET_T_STRING);
   ENNA_CONFIG_VAL(D, T, photo_extensions, EET_T_STRING);
   ENNA_CONFIG_VAL(D, T, blacklist, EET_T_STRING);
   ENNA_CONFIG_VAL(D, T, scan_period, EET_T_UINT);
   ENNA_CONFIG_VAL(D, T, cache_path, EET_T_STRING);

   if (ecore_file_exists(config_file))
     _make_config(config_file);
   else
     _make_config(NULL);

   ems_config = _config_get(conf_edd);

   DBG("Config values :\n\tversion: %d\n"
       "\tport: %d\n"
       "\tport stream: %d\n"
       "\tname: %s\n"
       "\tvideo ext: %s\n"
       "\tmusic ext: %s\n"
       "\tphoto ext: %s\n"
       "\tblacklist: %s\n"
       "\tscan period : %d\n"
       "\tcache path : %s",
       ems_config->version,
       ems_config->port,
       ems_config->port_stream,
       ems_config->name,
       ems_config->video_extensions,
       ems_config->music_extensions,
       ems_config->photo_extensions,
       ems_config->blacklist,
       ems_config->scan_period,
       ems_config->cache_path);

   return EINA_TRUE;
}

void
ems_config_shutdown(void)
{
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
