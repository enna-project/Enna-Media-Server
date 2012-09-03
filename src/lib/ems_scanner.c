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

#include <sys/stat.h>
#include <inttypes.h>

#include <Ecore.h>
#include <Eio.h>

#include "Ems.h"
#include "ems_private.h"
#include "ems_scanner.h"
#include "ems_config.h"
#include "ems_database.h"
#include "ems_grabber.h"
#include "ems_utils.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

typedef struct _Ems_Scanner Ems_Scanner;

struct _Ems_Scanner
{
   int is_running;
   Ecore_Timer *schedule_timer;
   Eina_List *scan_files;
   int progress;
   double start_time;
   int64_t time;
};

static Eina_Bool _file_filter_cb(void *data, Eio_File *handler, Eina_File_Direct_Info *info);
static void _file_main_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info);
static void _file_done_cb(void *data, Eio_File *handler);
static void _file_error_cb(void *data, Eio_File *handler, int error);

static Ems_Scanner *_scanner = NULL;

static Eina_Bool
_schedule_timer_cb(void *data __UNUSED__)
{

   ems_scanner_start();

   _scanner->schedule_timer = NULL;

   return EINA_FALSE;
}

static Eina_Bool
_ems_util_has_suffix(const char *name, const char *extensions)
{
   Eina_Bool ret = EINA_FALSE;
   int i;
   char **arr = eina_str_split(extensions, ",", 0);

   if (!arr)
     return EINA_FALSE;

   for (i = 0; arr[i]; i++)
     {
        if (eina_str_has_extension(name, arr[i]))
          {
             ret = EINA_TRUE;
             break;
          }
     }
   free(arr[0]);
   free(arr);

   return ret;
}


static Eina_Bool
_file_filter_cb(void *data, Eio_File *handler __UNUSED__, Eina_File_Direct_Info *info)
{
   const char *ext = NULL;
   Ems_Directory *dir = data;

   if (*(info->path + info->name_start) == '.' )
     return EINA_FALSE;

   if ( info->type == EINA_FILE_DIR )
     return EINA_TRUE;

   switch (dir->type)
     {
      case EMS_MEDIA_TYPE_VIDEO:
      case EMS_MEDIA_TYPE_TVSHOW:
         ext = ems_config->video_extensions;
         break;
      case EMS_MEDIA_TYPE_MUSIC:
         ext = ems_config->music_extensions;
         break;
      case EMS_MEDIA_TYPE_PHOTO:
         ext = ems_config->photo_extensions;
         break;
      default:
         ERR("Unknown type %d", dir->type);
         return EINA_FALSE;
     }

   if (_ems_util_has_suffix(info->path + info->name_start, ext))
     {
        const char *type;
        struct stat st;
        int64_t mtime;
        uint64_t hash, size;
        char uuid[50];
        char ssize[17];
        char *tmp;


        switch (dir->type)
          {
           case EMS_MEDIA_TYPE_VIDEO:
              type = eina_stringshare_add("video");
              break;
           case EMS_MEDIA_TYPE_MUSIC:
              type = eina_stringshare_add("music");
              break;
           case EMS_MEDIA_TYPE_PHOTO:
              type = eina_stringshare_add("photo");
              break;
           case EMS_MEDIA_TYPE_TVSHOW:
              type = eina_stringshare_add("tvshow");
           default:
              ERR("Unknown type %d", dir->type);
              type = NULL;
              break;
          }

        eina_stringshare_del(type);
        stat(info->path, &st);

        if (!ems_utils_hash_compute(info->path, &hash, &size) || !size || !hash)
          {
             ERR("%s is corrupted ?", info->path);
             return EINA_FALSE;
          }

        snprintf(uuid, sizeof(uuid), "%032"PRIx64"-%016"PRIx32,
                 hash,
                 (uint32_t)size);

        mtime = ems_database_file_mtime_get(uuid);

        if (mtime < 0)
          {
             /* File doesn't exists in the db, insert a new one, start time is here a markup, once the scan is finished we do a request on this value to see which files changed, these files can be then removed from the db, as they are removed on the filesystem*/

             DBG("Insert %s with hash : %s", info->path, uuid);

             ems_database_file_insert(uuid, dir->label, info->path, (int64_t)st.st_mtime, _scanner->time);
             snprintf(ssize, sizeof(ssize), "%"PRIx64, (uint64_t)size);
             /* Insert metadata name, filesize and filename in database */
             ems_database_meta_insert(uuid, "filesize", ssize);
             ems_database_meta_insert(uuid, "filename", info->path);
             /* Insert metadata clean_name in database based on searched string */

             //FIXME: season, episode should be passed to the grabber
             tmp = ems_utils_decrapify(info->path, NULL, NULL);
             ems_database_meta_insert(uuid, "clean_name", tmp);
             /* Queue file in grabber list */
             ems_grabber_grab(uuid, tmp, dir->type);
          }
        else
          {
             DBG("File %s already exists in db, update it", info->path);

             ems_database_file_insert(uuid, dir->label, info->path, (int64_t)st.st_mtime, _scanner->time);
             /* we grab only file who changed since the last scan */
             if (mtime != (int64_t)st.st_mtime)
               {
                  INF("File changed on disk %s", info->path);
                  //ems_grabber_grab(info->path, dir->type);
               }

          }

        /* Keep track of files, this list should be remobed */
        _scanner->scan_files = eina_list_append(_scanner->scan_files,
                                                eina_stringshare_add(info->path));
        /* Commit db each 100 files ? does it really works ?*/
        if (!eina_list_count(_scanner->scan_files) % 100)
             ems_database_flush();

        return EINA_TRUE;
     }
   else
     return EINA_FALSE;

}

static void
_file_main_cb(void *data __UNUSED__, Eio_File *handler __UNUSED__, const Eina_File_Direct_Info *info __UNUSED__)
{

}

static void
_file_done_cb(void *data, Eio_File *handler __UNUSED__)
{
   Ems_Directory *dir = data;

   _scanner->is_running--;

   if (!_scanner->is_running)
     {
        const char *f;

        /* TODO: get the list of deleted file */
        ems_database_deleted_files_remove(_scanner->time, dir->label);

        /* Flush the database */
        ems_database_flush();

	/* Schedule the next scan */
	if (_scanner->schedule_timer)
	  ecore_timer_del(_scanner->schedule_timer);
	if (ems_config->scan_period)
	  {
	     _scanner->schedule_timer = ecore_timer_add(ems_config->scan_period, _schedule_timer_cb, NULL);
	     /* TODO: covert time into a human redeable value */
	     INF("Scan finished in %3.3fs, schedule next scan in %d seconds", ecore_time_get() - _scanner->start_time, ems_config->scan_period);
	  }
	else
	  {
	     INF("Scan finished in %3.3fs, scan schedule disabled according to the configuration.", ecore_time_get() - _scanner->start_time);
	  }

        INF("%d file scanned", eina_list_count(_scanner->scan_files));
        /* Free the scan list */

        EINA_LIST_FREE(_scanner->scan_files, f)
          {
             eina_stringshare_del(f);
          }

        _scanner->scan_files = NULL;
        _scanner->progress = 0;
        _scanner->start_time = 0;

     }
}

static void
_file_error_cb(void *data, Eio_File *handler __UNUSED__, int error __UNUSED__)
{
   Ems_Directory *dir = data;

   _scanner->is_running--;
   ERR("Unable to parse %s", dir->path);
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
ems_scanner_init(void)
{
   _scanner = calloc(1, sizeof(Ems_Scanner));
   if (!_scanner)
     return EINA_FALSE;

   if (!ems_database_init())
     {
        ERR("Unable to create database");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

void
ems_scanner_shutdown(void)
{
   if (_scanner)
     {
        const char *f;

        if (_scanner->scan_files)
          EINA_LIST_FREE(_scanner->scan_files, f)
            eina_stringshare_del(f);

	if (_scanner->schedule_timer)
	  ecore_timer_del(_scanner->schedule_timer);

        ems_database_shutdown();

        free(_scanner);
     }
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

void
ems_scanner_start(void)
{
   Eina_List *l;
   Ems_Directory *dir;

   if (!_scanner)
     {
        ERR("Init scanner first : ems_scanner_init()");
     }

   if (_scanner->is_running)
     {
        WRN("Scanner is already running, did you try to run the scanner twice ?");
        return;
     }

   /* /\* Disable scan for now : EIO is broken *\/ */
   /* return; */

   _scanner->time = time(NULL);
   _scanner->start_time =  ecore_time_get();
   /* TODO : get all files in the db and see if they exist on the disk */

   /* Scann all files on the disk */
   DBG("Scanning videos directories :");
   EINA_LIST_FOREACH(ems_config->places, l, dir)
     {
        _scanner->is_running++;
        DBG("Scanning %s: %s", dir->label, dir->path);
        eio_dir_direct_ls(dir->path,
                          _file_filter_cb,
                          _file_main_cb,
                          _file_done_cb,
                          _file_error_cb,
                          dir);

     }
}



void
ems_scanner_state_get(Ems_Scanner_State *state, double *percent)
{

   if (!_scanner)
     return;

   if (_scanner->is_running && state)
     *state = EMS_SCANNER_STATE_RUNNING;
   else if (state)
     *state = EMS_SCANNER_STATE_IDLE;

   if (eina_list_count(_scanner->scan_files) && percent)
     *percent = (double) _scanner->progress / eina_list_count(_scanner->scan_files);
   else if (percent)
     *percent = 0.0;

   INF("Scanner is %s (%3.3f%%)", _scanner->is_running ? "running" : "in idle", (double) _scanner->progress * 100.0 / eina_list_count(_scanner->scan_files));

}

