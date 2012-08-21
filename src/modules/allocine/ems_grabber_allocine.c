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
#include "ems_database.h"
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

#define EMS_ALLOCINE_API_KEY "YW5kcm9pZC12M3M"
#define EMS_ALLOCINE_QUERY_SEARCH  "http://api.allocine.fr/rest/v3/search?partner=%s&filter=movie&q=%s&format=json"
#define EMS_ALLOCINE_QUERY_INFO    "http://api.allocine.fr/rest/v3/movie?partner=%s&code=%d&mediafmt=mp4-lc&format=json&filter=movie"
#define EMS_ALLOCINE_QUERY_TRAILER "http://api.allocine.fr/rest/v3/media?partner=%s&code=%d&mediafmt=mp4-hip&format=json&profile=large"

#define VH_ISALNUM(c) isalnum ((int) (unsigned char) (c))
#define VH_ISGRAPH(c) isgraph ((int) (unsigned char) (c))
#define VH_ISSPACE(c) isspace ((int) (unsigned char) (c))
#define IS_TO_DECRAPIFY(c)                      \
  ((unsigned) (c) <= 0x7F                       \
   && (c) != '\''                               \
   && !VH_ISSPACE (c)                           \
   && !VH_ISALNUM (c))

typedef struct _Ems_Allocine_Req Ems_Allocine_Req;
typedef enum _Ems_Request_State Ems_Request_State;
typedef struct _Ems_Allocine_Stats Ems_Allocine_Stats;

enum _Ems_Request_State
  {
    EMS_REQUEST_STATE_SEARCH,
    EMS_REQUEST_STATE_INFO,
    EMS_REQUEST_STATE_TRAILER
  };

struct _Ems_Allocine_Req
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

struct _Ems_Allocine_Stats
{
   int total;
   int files_grabbed;
   int covers_grabbed;
   int backdrop_grabbed;
   int multiple_results;
};

static Eina_Hash *_hash_req = NULL;
static Ems_Allocine_Stats *_stats = NULL;



#define PUTVAL(val, type, eina_type)                                    \
  do {                                                                  \
     cJSON *it;                                                         \
     Eina_Value v;                                                      \
     it = cJSON_GetObjectItem(m, #val);                                 \
     eina_value_setup(&v, eina_type);                                   \
     if (it) {                                                          \
        const char *str;                                                \
        eina_value_set(&v, it->type);                                   \
        str = eina_stringshare_add(eina_value_to_string(&v));           \
        ems_database_meta_insert(ems_config->db, req->filename, #val, str); \
     }                                                                  \
     eina_value_flush(&v);                                              \
  } while(0);                                                           \

#define PUTVALSTR(val, key)                                             \
  do {                                                                  \
     cJSON *it;                                                         \
     it = cJSON_GetObjectItem(m, val);                                  \
     if (it) {                                                          \
        eina_hash_add(req->grabbed_data->data, key, eina_stringshare_add(it->valuestring)); \
     }                                                                  \
  } while(0);                                                           \


static void
_grabber_allocine_shutdown(void)
{
   INF("Shutdown Allocine grabber");
   eina_hash_free(_hash_req);
   ecore_con_url_shutdown();
   ecore_con_shutdown();
}


static Eina_Bool
_search_data_cb(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Data *ev)
{
   Ems_Allocine_Req *req = eina_hash_find(_hash_req, ev->url_con);

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

   DBG("insert poster for %s", (const char*)data);
   ems_database_meta_insert(ems_config->db, data, "poster",  eina_stringshare_add(filename));
   eina_stringshare_del(data);
   _stats->covers_grabbed++;
   return EINA_TRUE;
}

static Eina_Bool
_backdrop_download_end_cb(void *data, const char *url,
                          const char *filename)
{

   DBG("insert backdrop for %s", (const char*)data);
   ems_database_meta_insert(ems_config->db, data, "backdrop",  eina_stringshare_add(filename));
   eina_stringshare_del(data);
   _stats->backdrop_grabbed++;
   return EINA_TRUE;
}



static void
_allocine_images_get(Ems_Allocine_Req *req, cJSON *parent, const char *name, Ems_Downloader_End_Cb end_cb)
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
                            poster = eina_stringshare_add(it->valuestring);
                            DBG("start %s download for %s", name, req->filename);
                            ems_downloader_url_download(poster, req->filename,
                                                        end_cb,
                                                        eina_stringshare_add(req->filename));
                            eina_stringshare_del(poster);
                            break;
                         }
                    }
               }
          }
     }


}

static Eina_Bool
_search_complete_cb(void *data __UNUSED__, int type __UNUSED__, void *event_info)
{
   Ecore_Con_Event_Url_Complete *url_complete = event_info;
   Ems_Allocine_Req *req = eina_hash_find(_hash_req, url_complete->url_con);
   cJSON *root;

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
              cJSON *feed, *it, *movies;
              cJSON *m;
              int nb_results = 0;
	      int code;
	      char url[PATH_MAX];
              const char *buf = eina_strbuf_string_get(req->buf);

              if (!buf)
                {
                   if (req->end_cb)
                     req->end_cb(req->data, req->filename, req->grabbed_data);
                   eina_hash_del(_hash_req, req->ec_url, req);

                   return ECORE_CALLBACK_DONE;
                }

              //DBG("Search request data : %s", eina_strbuf_string_get(req->buf));
              root = cJSON_Parse(buf);
              DBG("%s", cJSON_Print(root));
	      feed =  cJSON_GetObjectItem(root, "feed");
	      it = cJSON_GetObjectItem(feed, "totalResults");
	      if (it)
		{
		   nb_results = it->valueint;
		   DBG("NB results : %d", nb_results);
		}
	      else
		{
		   ERR("Cannot find totalResults tag !");
                   goto end_req;
		}
              if (!nb_results)
                {
                   WRN("No result found");
                   goto end_req;
                }
              else if (nb_results > 1)
                {
                   INF("More then one result, take the first");
                   _stats->multiple_results++;
                }
	      movies = cJSON_GetObjectItem(feed, "movie");
	      if (!movies)
		{
		   ERR("Error cannot found movie tag!");
                   goto end_req;
		}
              m = cJSON_GetArrayItem(movies, 0);
              if (!m)
                {
                   ERR("Unable to get movie info");
                   goto end_req;
                }
	      it = cJSON_GetObjectItem(m, "code");
	      if (it)
		{
		   code = it->valueint;
		   DBG("Movie code : %d", code);
		}
	      else
		{
		   ERR("Unable to get movie code");
                   goto end_req;
		}

              _stats->files_grabbed++;

	      if (root) cJSON_Delete(root);

	      snprintf(url, sizeof (url), EMS_ALLOCINE_QUERY_INFO,
		       EMS_ALLOCINE_API_KEY, code);

	      DBG("Search for %s", url);
	      req->ec_url = ecore_con_url_new(url);
	      if (!req->ec_url)
		{
		   ERR("error when creating ecore con url object.");
		   goto end_req;
		}
	      eina_strbuf_free(req->buf);
	      req->buf =  eina_strbuf_new();
	      req->state = EMS_REQUEST_STATE_INFO;

	      if (!ecore_con_url_get(req->ec_url))
		{
		   ERR("could not realize request.");
		   eina_hash_del(_hash_req, req->ec_url, req);
		   ecore_con_url_free(req->ec_url);
		   goto end_req;
		}

	      return ECORE_CALLBACK_DONE;
           }
         else
           {
              if (req->end_cb)
                req->end_cb(req->data, req->filename, req->grabbed_data);
           }
         break;
      case EMS_REQUEST_STATE_INFO:
	 DBG("Request movie info");
	 if (req->buf)
           {
	      const char *buf = eina_strbuf_string_get(req->buf);
	      cJSON *m, *trailer;
              int code;
              char url[PATH_MAX];

              if (!buf)
                {
                   if (req->end_cb)
                     req->end_cb(req->data, req->filename, req->grabbed_data);
                   eina_hash_del(_hash_req, req->ec_url, req);

                   return ECORE_CALLBACK_DONE;
                }

              root = cJSON_Parse(buf);
              DBG("%s", cJSON_Print(root));

	      m = cJSON_GetObjectItem(root, "movie");

	      PUTVALSTR("originalTitle", "original_title");
              PUTVALSTR("title", "title");

              trailer = cJSON_GetObjectItem(m, "trailer");
              if (!trailer)
                {
                   ERR("Cannot find tag trailer");
                   goto end_req;
                }
              m = cJSON_GetObjectItem(trailer, "code");
              if (!m)
                {
                   INF("Cannot find trailer for this movie");
                   goto end_req;
                }

              code = m->valueint;
              snprintf(url, sizeof (url), EMS_ALLOCINE_QUERY_TRAILER,
		       EMS_ALLOCINE_API_KEY, code);

	      DBG("Search for %s", url);
	      req->ec_url = ecore_con_url_new(url);
	      if (!req->ec_url)
		{
		   ERR("error when creating ecore con url object.");
		   goto end_req;
		}
	      eina_strbuf_free(req->buf);
	      req->buf =  eina_strbuf_new();
	      req->state = EMS_REQUEST_STATE_TRAILER;

	      if (!ecore_con_url_get(req->ec_url))
		{
		   ERR("could not realize request.");
		   eina_hash_del(_hash_req, req->ec_url, req);
		   ecore_con_url_free(req->ec_url);
		   goto end_req;
		}

	      return ECORE_CALLBACK_DONE;
	   }
	 break;
      case EMS_REQUEST_STATE_TRAILER:
	 DBG("Request trailer info");
	 if (req->buf)
           {
	      const char *buf = eina_strbuf_string_get(req->buf);
	      cJSON *media, *rendition, *width, *height, *it, *m;
              const char *trailer_url;
              int w = 0, h = 0;
              int i;
              int size = 0;

              if (!buf)
                {
                   if (req->end_cb)
                     req->end_cb(req->data, req->filename, req->grabbed_data);
                   eina_hash_del(_hash_req, req->ec_url, req);

                   return ECORE_CALLBACK_DONE;
                }

              root = cJSON_Parse(buf);
              DBG("%s", cJSON_Print(root));

              media = cJSON_GetObjectItem(root, "media");
              if (!media)
                {
                   ERR("Cannot find tag media");
                   goto end_req;
                }
              rendition = cJSON_GetObjectItem(media, "rendition");
              if (!rendition)
                {
                   ERR("Cannot find tag rendition");
                   goto end_req;
                }
              size = cJSON_GetArraySize(rendition);
              DBG("Find %d trailers", size);
              /* parse all results and try to take the bigger resolution*/
              for (i = 0; i < size; i++)
                {
                   it = cJSON_GetArrayItem(rendition, i);
                   if (it)
                     {
                        width = cJSON_GetObjectItem(it, "width");
                        height = cJSON_GetObjectItem(it, "height");
                        if ((width->valueint > w) && (height->valueint > h))
                          {
                             m = it;
                             w = width->valueint;
                             h = height->valueint;
                             DBG("trailer resolution %dx%d", width->valueint, height->valueint);
                          }
                     }
                }

              PUTVALSTR("href", "trailer");
           }
         break;
      default:
	break;
     }

   if (req->end_cb)
     req->end_cb(req->data, req->filename, req->grabbed_data);
   //ecore_con_url_free(req->ec_url);
   eina_hash_del(_hash_req, req->ec_url, req);

   return ECORE_CALLBACK_DONE;

 end_req:
   if (root) cJSON_Delete(root);

   if (req->end_cb)
     req->end_cb(req->data, req->filename, req->grabbed_data);
   //ecore_con_url_free(req->ec_url);
   eina_hash_del(_hash_req, req->ec_url, req);
   return ECORE_CALLBACK_DONE;
}

static void
_request_free_cb(Ems_Allocine_Req *req)
{
   if (!req)
     return;
   if (req->filename) eina_stringshare_del(req->filename);
   if (req->search) eina_stringshare_del(req->search);
   if (req->buf) eina_strbuf_free(req->buf);
   free(req);
}


static Eina_Bool
_grabber_allocine_init(void)
{
   INF("Init Allociné grabber");
   ecore_con_init();
   ecore_con_url_init();

   _hash_req = eina_hash_pointer_new((Eina_Free_Cb)_request_free_cb);

   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, (Ecore_Event_Handler_Cb)_search_complete_cb, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, (Ecore_Event_Handler_Cb)_search_data_cb, NULL);

   _dom = eina_log_domain_register("ems_grabber_allocine", ENNA_DEFAULT_LOG_COLOR);

   _stats = calloc(1, sizeof(Ems_Allocine_Stats));

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
   Ems_Allocine_Req *req;

   if (type != EMS_MEDIA_TYPE_VIDEO)
     return;

   DBG("Grab %s of type %d", filename, type);
   _stats->total++;

   search = ems_utils_escape_string(filename);

   snprintf(url, sizeof (url), EMS_ALLOCINE_QUERY_SEARCH,
            EMS_ALLOCINE_API_KEY, search);

   DBG("Search for %s", url);

   ec_url = ecore_con_url_new(url);
   if (!ec_url)
     {
        ERR("error when creating ecore con url object.");
        return;
     }

   req = calloc(1, sizeof(Ems_Allocine_Req));
   req->filename = eina_stringshare_add(filename);
   req->search = eina_stringshare_add(search);
   if (search)
     free(search);
   req->ec_url = ec_url;
   req->end_cb = end_cb;
   req->data = data;
   req->buf = eina_strbuf_new();
   req->grabbed_data = calloc(1, sizeof (Ems_Grabber_Data));
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

EAPI void
ems_grabber_stats(void)
{
   INF("Stats for ALLOCINE module");
   INF("Total files grabbed : %d", _stats->total);
   INF("Files grabbed : %d (%3.3f%%)", _stats->files_grabbed,  _stats->files_grabbed * 100.0 / _stats->total);
   INF("Covers grabbed : %d (%3.3f%%)", _stats->covers_grabbed,  _stats->files_grabbed * 100.0 / _stats->total);
   INF("Backdrop grabbed : %d (%3.3f%%)", _stats->backdrop_grabbed,  _stats->backdrop_grabbed * 100.0 / _stats->total);
   INF("Multipled results : %d (%3.3f%%)", _stats->multiple_results,  _stats->multiple_results * 100.0 / _stats->total);
   INF("End of stats");
}

EINA_MODULE_INIT(_grabber_allocine_init);
EINA_MODULE_SHUTDOWN(_grabber_allocine_shutdown);

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
