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
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>
#include <Ecore_File.h>

#include "ems_private.h"
#include "ems_downloader.h"
#include "ems_config.h"
#include "ems_database.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Ems_Downloader Ems_Downloader;
typedef struct _Ems_Downloader_Req Ems_Downloader_Req;

struct _Ems_Downloader
{
   Eina_Hash *hash_req;
};

struct _Ems_Downloader_Req
{
   const char *url;
   const char *filename;
   Ecore_Con_Url *ec_url;
   long size;
   int fd;
   void (*end_cb)(void *data, const char *url, const char *filename);
   void *data;
};

static Ems_Downloader *_downloader = NULL;

static void
_request_free_cb(Ems_Downloader_Req *req)
{
   if (!req)
     return;

   if (req->fd != -1)
     close(req->fd);
   eina_stringshare_del(req->filename);
   eina_stringshare_del(req->url);
   free(req);
}

/* static Eina_Bool */
/* _url_progress_cb(void *data, int type, void *event_info) */
/* { */
/*    Ecore_Con_Event_Url_Progress *url_progress = event_info; */
/*    float percent; */

/*    if (url_progress->down.total > 0) */
/*      { */
  
/*         percent = (url_progress->down.now / url_progress->down.total) * 100; */
/*         printf("Total of download complete of %s: %0.1f (%0.0f)%%\n", ecore_con_url_url_get(url_progress->url_con), percent, url_progress->down.now); */
/*      } */

/*    return EINA_TRUE; */
/* } */


static Eina_Bool
_url_complete_cb(void *data __UNUSED__, int type __UNUSED__, void *event_info)
{
   Ecore_Con_Event_Url_Complete *url_complete = event_info;

   Ems_Downloader_Req *req = eina_hash_find(_downloader->hash_req, url_complete->url_con);

   if (!req)
     return ECORE_CALLBACK_PASS_ON;

   if (req->end_cb)
     req->end_cb(req->data, req->url, req->filename);

   //ecore_con_url_free(req->ec_url);
   eina_hash_del(_downloader->hash_req, req->ec_url, req);

   return ECORE_CALLBACK_DONE;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
ems_downloader_init(void)
{

   _downloader = calloc(1, sizeof(Ems_Downloader));
   if (!_downloader)
     return EINA_FALSE;

   _downloader->hash_req = eina_hash_pointer_new((Eina_Free_Cb)_request_free_cb);

   //ecore_event_handler_add(ECORE_CON_EVENT_URL_PROGRESS, _url_progress_cb, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, _url_complete_cb, NULL);

   return EINA_TRUE;
}

Eina_Bool
ems_downloader_url_download(const char *url, const char *file,
                            Ems_Downloader_End_Cb end_cb, void *data )
{
   Ems_Downloader_Req *req;
   char filename[PATH_MAX];
   char directory[PATH_MAX];
   const char *tmp;
   const char *hash;
   if (!url)
     return EINA_FALSE;

   req = calloc(1, sizeof(Ems_Downloader_Req));
   if (!req)
     return EINA_FALSE;

   req->ec_url = ecore_con_url_new(url);
   req->end_cb = end_cb;
   req->data = data;

   req->url = eina_stringshare_add(url);

   if (!req->ec_url)
     goto err1;

   DBG("Url : %s\n", url);

   hash = ems_database_file_hash_get(ems_config->db, file);
   if (!hash)
     goto err1;


   snprintf(directory, sizeof(directory), "%s/%s",
            ems_config_cache_dirname_get(), hash);
   eina_stringshare_del(hash);

   if (!ecore_file_is_dir(directory))
     ecore_file_mkdir(directory);

   tmp = ecore_file_file_get(url);
   snprintf(filename, sizeof(filename), "%s/%s",
            directory, tmp);

   req->filename = eina_stringshare_add(filename);

   DBG("save file to : %s\n", filename);

   if (ecore_file_exists(filename))
     ERR("File %s already exists", filename);

   req->fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC, 0644);
   if (req->fd == -1)
     {
        ERR("Unable to open %s for writing", filename);
        goto err2;
     }

   eina_hash_add(_downloader->hash_req, req->ec_url, req);
   ecore_con_url_fd_set(req->ec_url, req->fd);

  if (!ecore_con_url_get(req->ec_url))
     {
        ERR("could not realize request.");
        goto err3;
     }

   return EINA_TRUE;

 err3:
   ecore_con_url_free(req->ec_url);
   close(req->fd);
 err2:
   eina_stringshare_del(req->filename);
 err1:
   eina_stringshare_del(req->url);
   free(req);

   return EINA_FALSE;
}


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
