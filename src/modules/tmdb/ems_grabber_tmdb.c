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

#include "Ems.h"
#include "ems_private.h"
#include "ems_utils.h"
//#include "ems_database.h"
#include "ems_downloader.h"
#include "cJSON.h"

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

#define EMS_TMDB_API_KEY "ba2eed549e5c84e712931fef2b69bfb1"
#define EMS_TMDB_QUERY_SEARCH "http://api.themoviedb.org/2.1/Movie.search/en/json/%s/%s"
#define EMS_TMDB_QUERY_INFO   "http://api.themoviedb.org/2.1/Movie.getInfo?id=%s&api_key=%s"
#define VH_ISALNUM(c) isalnum ((int) (unsigned char) (c))
#define VH_ISGRAPH(c) isgraph ((int) (unsigned char) (c))
#define VH_ISSPACE(c) isspace ((int) (unsigned char) (c))
#define IS_TO_DECRAPIFY(c)                      \
  ((unsigned) (c) <= 0x7F                       \
   && (c) != '\''                               \
   && !VH_ISSPACE (c)                           \
   && !VH_ISALNUM (c))

typedef struct _Ems_Tmdb_Req Ems_Tmdb_Req;
typedef enum _Ems_Request_State Ems_Request_State;
typedef struct _Ems_Tmdb_Stats Ems_Tmdb_Stats;

enum _Ems_Request_State
  {
    EMS_REQUEST_STATE_SEARCH,
    EMS_REQUEST_STATE_INFO
  };

struct _Ems_Tmdb_Req
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

struct _Ems_Tmdb_Stats
{
   int total;
   int files_grabbed;
   int covers_grabbed;
   int backdrop_grabbed;
   int multiple_results;
};

static Eina_Hash *_hash_req = NULL;
static Ems_Tmdb_Stats *_stats = NULL;

#define PUTVAL(val, key, type, eina_type)                               \
  do {                                                                  \
     cJSON *it;                                                         \
     Eina_Value *v;                                                     \
     v = eina_value_new(eina_type);                                     \
     it = cJSON_GetObjectItem(m, val);                                  \
     if (it) {                                                          \
        eina_value_set(v, it->type);                                    \
        eina_hash_add(hash, key, v);                                    \
     }                                                                  \
  } while(0);                                                           \


static void
_grabber_tmdb_shutdown(void)
{
   INF("Shutdown TMDb grabber");
   eina_hash_free(_hash_req);
   free(_stats);
   ecore_con_url_shutdown();
   ecore_con_shutdown();
}


static Eina_Bool
_search_data_cb(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Data *ev)
{
   Ems_Tmdb_Req *req = eina_hash_find(_hash_req, ev->url_con);

   if (!req)
     return  ECORE_CALLBACK_PASS_ON;

   if (req->buf)
     eina_strbuf_append_length(req->buf, (char*)&ev->data[0], ev->size);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_poster_download_end_cb(void *data, const char *url,
                        const char *filename)
{
   Ems_Tmdb_Req *req = data;


   DBG("insert poster for %s", req->filename);

   ems_database_meta_insert(data, "poster",  eina_stringshare_add(filename));
   _stats->covers_grabbed++;
   return EINA_TRUE;
}

static Eina_Bool
_backdrop_download_end_cb(void *data, const char *url,
                          const char *filename)
{

   DBG("insert backdrop for %s", (const char*)data);
   ems_database_meta_insert(data, "backdrop",  eina_stringshare_add(filename));
   _stats->backdrop_grabbed++;
   return EINA_TRUE;
}



static cJSON *
_tmdb_images_get(Ems_Tmdb_Req *req, cJSON *parent, const char *name)
{
   int size = 0;
   int i;
   cJSON *images, *m;
   const char *search;

   images = cJSON_GetObjectItem(parent, name);
   if (images)
     {
        const char *poster = NULL;
        size = cJSON_GetArraySize(images);
        for (i = 0; i < size; i++)
          {
             cJSON *it_image, *it;
             m = cJSON_GetArrayItem(images, i);
             if (!m)
               break;
             it_image =  cJSON_GetObjectItem(m, "image");
             if (it_image)
               {
                  it = cJSON_GetObjectItem(it_image, "size");
                  if (it && !strcmp(it->valuestring, "original"))
                    {
                       it = cJSON_GetObjectItem(it_image, "url");
                       if (it && it->valuestring)
                         {
                           return it;
                         }
                    }
               }
          }
     }

   return NULL;
}

static Eina_Bool
_search_complete_cb(void *data __UNUSED__, int type __UNUSED__, void *event_info)
{
   Ecore_Con_Event_Url_Complete *url_complete = event_info;

   Ems_Tmdb_Req *req = eina_hash_find(_hash_req, url_complete->url_con);

   if (!req)
     return ECORE_CALLBACK_PASS_ON;

   DBG("download completed for %s with status code: %d", req->filename, url_complete->status);
   if (url_complete->status != 200)
     {
        if (req->end_cb)
          req->end_cb(req->data, req->filename, req->grabbed_data);
        //ecore_con_url_free(req->ec_url);
        eina_hash_del(_hash_req, req->ec_url, req);
        return ECORE_CALLBACK_DONE;
     }


   switch(req->state)
     {
      case EMS_REQUEST_STATE_SEARCH:
         if (req->buf)
           {
              cJSON *root;
              cJSON *m;
              int size = 0;
              const char *buf = eina_strbuf_string_get(req->buf);

              if (!buf)
                {
                   if (req->end_cb)
                     req->end_cb(req->data, req->filename, req->grabbed_data);
                   //ecore_con_url_free(req->ec_url);
                   eina_hash_del(_hash_req, req->ec_url, req);

                   return ECORE_CALLBACK_DONE;
                }

              //DBG("Search request data : %s", eina_strbuf_string_get(req->buf));
              root = cJSON_Parse(buf);
              if (root)
                size = cJSON_GetArraySize(root);
              DBG("%s", cJSON_Print(root));
              //DBG("Size %d", size);

              if (!size)
                {
                   DBG("No result found");
                   goto end_req;
                   return ECORE_CALLBACK_DONE;
                }
              else if (size > 1)
                {
                   DBG("More then one result, take the first");
                   _stats->multiple_results++;
                }
              {
                  m = cJSON_GetArrayItem(root, 0);
                  if (!m)
                  {
                      ERR("Unable to get movie info");
                      goto end_req;
                  }
                  _stats->files_grabbed++;


                  Eina_Hash *hash = eina_hash_string_superfast_new(NULL);

                  req->grabbed_data->data = eina_list_append(req->grabbed_data->data, hash);
                  DBG("find info for %s", req->filename);
                  PUTVAL("score", "score", valuedouble, EINA_VALUE_TYPE_DOUBLE);
                  PUTVAL("popularity", "popularity", valueint, EINA_VALUE_TYPE_INT);
                  PUTVAL("translated", "translated", valueint, EINA_VALUE_TYPE_INT);
                  PUTVAL("adult", "adult", valueint, EINA_VALUE_TYPE_INT);
                  PUTVAL("language", "language", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("original_name", "original_title", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("name", "title", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("alternative_name", "alternative_title", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("id", "tmdb_id", valueint, EINA_VALUE_TYPE_INT);
                  PUTVAL("imdb", "imdb_id", valueint, EINA_VALUE_TYPE_INT);
                  PUTVAL("type", "type", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("url", "url", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("votes", "votes", valueint, EINA_VALUE_TYPE_INT);
                  PUTVAL("rating", "rating", valuedouble, EINA_VALUE_TYPE_DOUBLE);
                  PUTVAL("certification", "certification", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("overview", "overview", valuestring, EINA_VALUE_TYPE_STRINGSHARE);


                  cJSON *it;
                  it = cJSON_GetObjectItem(m, "overview");
                  if (it) printf("Overview : %s", it->valuestring);



                  PUTVAL("released", "released", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("version", "version", valueint, EINA_VALUE_TYPE_INT);
                  PUTVAL("last_modified_at", "last_modified_at", valuestring, EINA_VALUE_TYPE_STRINGSHARE);

                  cJSON *poster, *backdrops;

                  poster = _tmdb_images_get(req, m, "posters");
                  if (poster) printf("pster : %s", poster->valuestring);

                  backdrops = _tmdb_images_get(req, m, "backdrops");
                  if (backdrops) printf("backdrop : %s", backdrops->valuestring);

                  /*ems_downloader_url_download(poster, "tmdb",
                                              end_cb,
                                              req->filename);*/
                  cJSON_Delete(root);
              }
           end_req:
              if (req->end_cb)
                  req->end_cb(req->data, req->filename, req->grabbed_data);
              //ecore_con_url_free(req->ec_url);
              eina_hash_del(_hash_req, req->ec_url, req);
              return ECORE_CALLBACK_DONE;
           }
         else
           {
              if (req->end_cb)
                req->end_cb(req->data, req->filename, req->grabbed_data);
           }
         break;
      case EMS_REQUEST_STATE_INFO:;
      default:
         break;
     }

   if (req->end_cb)
     req->end_cb(req->data, req->filename, req->grabbed_data);
   //ecore_con_url_free(req->ec_url);
   eina_hash_del(_hash_req, req->ec_url, req);

   return ECORE_CALLBACK_DONE;
}

static void
_request_free_cb(Ems_Tmdb_Req *req)
{
   if (!req)
     return;
   if (req->filename) eina_stringshare_del(req->filename);
   if (req->search) eina_stringshare_del(req->search);
   if (req->buf) eina_strbuf_free(req->buf);
   if (req->ec_url) ecore_con_url_free(req->ec_url);

   free(req);
}


static Eina_Bool
_grabber_tmdb_init(void)
{
   INF("Init TMDb grabber");
   ecore_con_init();
   ecore_con_url_init();

   _hash_req = eina_hash_pointer_new((Eina_Free_Cb)_request_free_cb);

   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, (Ecore_Event_Handler_Cb)_search_complete_cb, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, (Ecore_Event_Handler_Cb)_search_data_cb, NULL);

   _dom = eina_log_domain_register("ems_grabber_tmdb", ENNA_DEFAULT_LOG_COLOR);

   _stats = calloc(1, sizeof(Ems_Tmdb_Stats));

   return EINA_TRUE;
}


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

EAPI void
ems_grabber_grab(Ems_Grabber_Params *params, Ems_Grabber_End_Cb end_cb, void *data)
{
   char url[PATH_MAX];
   Ecore_Con_Url *ec_url = NULL;
   char *tmp;
   char *s;
   Ems_Tmdb_Req *req;

   if (!params)
       return;

   if (params->type != EMS_MEDIA_TYPE_VIDEO)
     return;

   DBG("Grab %s of type %d", params->filename, params->type);
   _stats->total++;

   s = ems_utils_escape_string(params->search);

   snprintf(url, sizeof (url), EMS_TMDB_QUERY_SEARCH,
            EMS_TMDB_API_KEY, s);

   DBG("Search for %s", url);

   ec_url = ecore_con_url_new(url);
   if (!ec_url)
     {
        ERR("error when creating ecore con url object.");
        return;
     }

   req = calloc(1, sizeof(Ems_Tmdb_Req));
   req->filename = eina_stringshare_add(params->filename);
   req->search = eina_stringshare_add(s);
   if (s)
     free(s);
   req->ec_url = ec_url;
   req->end_cb = end_cb;
   req->data = data;
   req->buf = eina_strbuf_new();
   req->grabbed_data = calloc(1, sizeof(Ems_Grabber_Data));
   req->grabbed_data->data = NULL;
   req->state = EMS_REQUEST_STATE_SEARCH;

   eina_hash_add(_hash_req, ec_url, req);

   if (!ecore_con_url_get(ec_url))
     {
        ERR("could not realize request.");
        eina_hash_del(_hash_req, ec_url, req);
     }
}

EAPI void
ems_grabber_stats(void)
{
   INF("Stats for TMDB module");
   INF("Total files grabbed : %d", _stats->total);
   INF("Files grabbed : %d (%3.3f%%)", _stats->files_grabbed,  _stats->files_grabbed * 100.0 / _stats->total);
   INF("Covers grabbed : %d (%3.3f%%)", _stats->covers_grabbed,  _stats->files_grabbed * 100.0 / _stats->total);
   INF("Backdrop grabbed : %d (%3.3f%%)", _stats->backdrop_grabbed,  _stats->backdrop_grabbed * 100.0 / _stats->total);
   INF("Multipled results : %d (%3.3f%%)", _stats->multiple_results,  _stats->multiple_results * 100.0 / _stats->total);
   INF("End of stats");
}

EINA_MODULE_INIT(_grabber_tmdb_init);
EINA_MODULE_SHUTDOWN(_grabber_tmdb_shutdown);

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
