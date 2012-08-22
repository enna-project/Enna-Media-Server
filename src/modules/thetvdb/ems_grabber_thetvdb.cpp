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

#include <Ecore.h>
#include <Ecore_Con.h>

#include "tinyxml2.h"

#ifdef __cplusplus
extern "C" {
#endif /* ifdef __cplusplus */

#include "Ems.h"
#include "ems_private.h"
#include "ems_utils.h"
#include "ems_database.h"
#include "ems_downloader.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static int _dom;

#define ENNA_DEFAULT_LOG_COLOR EINA_COLOR_LIGHTBLUE
#ifdef ERR
# undef ERR
#endif /* ifdef ERR */
#define ERR(...)  EINA_LOG_DOM_ERR(_dom, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif /* ifdef DBG */
#define DBG(...)  EINA_LOG_DOM_DBG(_dom, __VA_ARGS__)
#ifdef INF
# undef INF
#endif /* ifdef INF */
#define INF(...)  EINA_LOG_DOM_INFO(_dom, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif /* ifdef WRN */
#define WRN(...)  EINA_LOG_DOM_WARN(_dom, __VA_ARGS__)
#ifdef CRIT
# undef CRIT
#endif /* ifdef CRIT */
#define CRIT(...) EINA_LOG_DOM_CRIT(_enna_log_dom_global, __VA_ARGS__)

#define EMX_THETVDB_DEFAULT_LANG        "en"

#define EMS_THETVDB_API_KEY             "7E2162A2014D5EC9"
#define EMS_THETVDB_QUERY_SEARCH        "http://thetvdb.com/api/GetSeries.php?seriesname=%s"
#define EMS_THETVDB_QUERY_INFO          "http://thetvdb.com/api/%s/series/%s/%s.xml"
#define EMS_THETVDB_QUERY_EPISODE_INFO  "http://thetvdb.com/api/%s/series/%s/default/%u/%u/%s.xml"
#define EMS_THETVDB_COVER               "http://thetvdb.com/banners/%s"

typedef struct _Ems_TheTVDB_Req Ems_TheTVDB_Req;

typedef enum _Ems_Request_State
{
   EMS_REQUEST_STATE_SEARCH,
   EMS_REQUEST_STATE_INFO,
   EMS_REQUEST_STATE_EPISODE_INFO,
} Ems_Request_State;

struct _Ems_TheTVDB_Req
{
   const char *filename;
   const char *search;
   Eina_Strbuf *buf;
   Ecore_Con_Url *ec_url;
   Ems_Request_State state;
   Ems_Grabber_Data *grabbed_data;
   Ems_Grabber_End_Cb end_cb;
   void *data;
};

static Eina_Hash *_hash_req = NULL;

static void
_grabber_thetvdb_shutdown(void)
{
   INF("Shutdown TheTVDB grabber");
   eina_hash_free(_hash_req);
   ecore_con_url_shutdown();
   ecore_con_shutdown();
}

static Eina_Bool
_search_data_cb(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Data *ev)
{
   Ems_TheTVDB_Req *req = (Ems_TheTVDB_Req *)eina_hash_find(_hash_req, ev->url_con);

   if (!req)
     return  ECORE_CALLBACK_PASS_ON;

   if (req->buf)
     eina_strbuf_append_length(req->buf, (char*)&ev->data[0], ev->size);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_search_complete_cb(void *data __UNUSED__, int type __UNUSED__, void *event_info)
{
   return ECORE_CALLBACK_PASS_ON;
}

static void
_request_free_cb(void *data)
{
   Ems_TheTVDB_Req *req = (Ems_TheTVDB_Req *)data;
   if (!req)
     return;
   if (req->filename) eina_stringshare_del(req->filename);
   if (req->search) eina_stringshare_del(req->search);
   if (req->buf) eina_strbuf_free(req->buf);
   free(req);
}

static Eina_Bool
_grabber_thetvdb_init(void)
{
   INF("Init TheTVDB grabber");
   ecore_con_init();
   ecore_con_url_init();

   _hash_req = eina_hash_pointer_new((Eina_Free_Cb)_request_free_cb);

   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, (Ecore_Event_Handler_Cb)_search_complete_cb, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, (Ecore_Event_Handler_Cb)_search_data_cb, NULL);

   _dom = eina_log_domain_register("ems_grabber_thetvdb", ENNA_DEFAULT_LOG_COLOR);

   return EINA_TRUE;
}


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

EAPI void
ems_grabber_grab(const char *filename, Ems_Media_Type type, Ems_Grabber_Params params, Ems_Grabber_End_Cb end_cb, void *data)
{
   char url[PATH_MAX];
   Ecore_Con_Url *ec_url = NULL;
   char *tmp;
   char *search = NULL;
   Ems_TheTVDB_Req *req;

   if (type != EMS_MEDIA_TYPE_TVSHOW)
     return;

   DBG("Grab %s of type %d (episode:%d season:%d)", filename, type, params.episode, params.season);

   search = ems_utils_escape_string(filename);

   snprintf(url, sizeof (url), EMS_THETVDB_QUERY_SEARCH,
            EMS_THETVDB_API_KEY, search);

   DBG("Search for %s", url);

   ec_url = ecore_con_url_new(url);
   if (!ec_url)
     {
        ERR("error when creating ecore con url object.");
        return;
     }

   req = (Ems_TheTVDB_Req *)calloc(1, sizeof(Ems_TheTVDB_Req));
   req->filename = eina_stringshare_add(filename);
   req->search = eina_stringshare_add(search);
   if (search)
     free(search);
   req->ec_url = ec_url;
   req->end_cb = end_cb;
   req->data = data;
   req->buf = eina_strbuf_new();
   req->grabbed_data = (Ems_Grabber_Data *)calloc(1, sizeof (Ems_Grabber_Data));
   req->grabbed_data->data = eina_hash_string_superfast_new(NULL);
   req->grabbed_data->lang = "fr";
   req->state = EMS_REQUEST_STATE_SEARCH;

   eina_hash_add(_hash_req, ec_url, req);

   if (!ecore_con_url_get(ec_url))
     {
        ERR("could not realize request.");
        eina_hash_del(_hash_req, ec_url, req);
        ecore_con_url_free(ec_url);
     }
}

EINA_MODULE_INIT(_grabber_thetvdb_init);
EINA_MODULE_SHUTDOWN(_grabber_thetvdb_shutdown);

#ifdef __cplusplus
}
#endif /* ifdef __cplusplus */

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
