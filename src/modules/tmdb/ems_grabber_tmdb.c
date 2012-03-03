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
#include "cJSON.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

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
typedef struct _Ems_Tmdb_Movie Ems_Tmdb_Movie;

enum _Ems_Request_State
  {
    EMS_REQUEST_STATE_SEARCH,
    EMS_REQUEST_STATE_INFO
  };

struct _Ems_Tmdb_Req
{
   const char *search;
   Eina_Strbuf *buf;
   Ecore_Con_Url *ec_url;
   Ems_Request_State state;

};

struct _Ems_Tmdb_Movie
{
   double score;
   char popularity;
   Eina_Bool translated;
   Eina_Bool adult;
   const char *language;
   const char *original_name;
   const char *name;
   const char *alternative_name;
   const char *type;
   int id;
   int imdb_id;
   const char *url;
   int votes;
   float rating;
   const char *certification;
   const char *overview;
   const char *released;
   const char *poster;
   const char backdrop;
   int version;
   const char *last_modified_at;
};

static void
_request_free(void *data)
{
   Ems_Tmdb_Req *req = data;

   if (!req)
     return;

   if (req->search) eina_stringshare_del(req->search);
   if (req->buf) eina_strbuf_free(req->buf);
   //free(req);
}


static Eina_Bool
_grabber_tmdb_init(void)
{
   INF("Init TMDb grabber");
   ecore_con_init();
   ecore_con_url_init();

   return EINA_TRUE;
}


static void
_grabber_tmdb_shutdown(void)
{
   INF("Shutdown TMDb grabber");
   ecore_con_url_shutdown();
   ecore_con_shutdown();
}


Eina_Bool
_search_data_cb(void *data, int type, Ecore_Con_Event_Url_Data *ev)
{
   Ems_Tmdb_Req *req = ecore_con_url_data_get(ev->url_con);

   if (!req || ev->url_con != req->ec_url)
     return ECORE_CALLBACK_RENEW;

   if (req->buf)
     eina_strbuf_append_length(req->buf, (char*)&ev->data[0], ev->size);
   else
     {
        req->buf = eina_strbuf_new();
        eina_strbuf_append_length(req->buf, (char*)&ev->data[0], ev->size);
     }

   return ECORE_CALLBACK_RENEW;
}

#define GETVAL(val, type)                       \
  do {                                          \
     cJSON *it;                                 \
     it = cJSON_GetObjectItem(m, #val);         \
     if (it)                                    \
       movie->val = it->type;                   \
  } while(0);                                   \

#define GETVALSTR(val)                                          \
  do {                                                          \
     cJSON *it;                                                 \
     it = cJSON_GetObjectItem(m, #val);                         \
     if (it)                                                    \
       movie->val = eina_stringshare_add(it->valuestring);      \
  } while(0);                                                   \

static Eina_Bool
_search_complete_cb(void *data, int type, void *event_info)
{
   Ecore_Con_Event_Url_Complete *url_complete = event_info;

   Ems_Tmdb_Req *req = ecore_con_url_data_get(url_complete->url_con);

   if (!req || url_complete->url_con != req->ec_url)
     return EINA_TRUE;

   DBG("download completed with status code: %d", url_complete->status);

   switch(req->state)
     {
      case EMS_REQUEST_STATE_SEARCH:
         if (req->buf)
           {
              cJSON *root;
              cJSON *m;
              Ems_Tmdb_Movie *movie;
              int size;

              DBG("Search request data : %s", eina_strbuf_string_get(req->buf));
              root = cJSON_Parse(eina_strbuf_string_get(req->buf));
              size = cJSON_GetArraySize(root);

              DBG("Size %d", size);

              if (!size)
                {
                   DBG("No result found");
                   return EINA_TRUE;
                }
              else if (size > 1)
                {
                   DBG("More then one result, take the first");
                }

              m = cJSON_GetArrayItem(root, 0);
              if (!m)
                {
                   ERR("Unable to get movie info");
                   return EINA_TRUE;
                }

              movie = calloc(1, sizeof(Ems_Tmdb_Movie));
              GETVAL(score, valuedouble);
              GETVAL(popularity, valueint);
              GETVAL(translated, valueint);
              GETVAL(adult, valueint);
              GETVALSTR(language);
              GETVALSTR(original_name);
              GETVALSTR(name);
              GETVALSTR(alternative_name);
              GETVAL(id, valueint);
              GETVAL(imdb_id, valueint);
              GETVALSTR(type);
              GETVALSTR(url);
              GETVAL(votes, valueint);
              GETVAL(rating, valuedouble);
              GETVALSTR(certification);
              GETVALSTR(overview);
              GETVALSTR(released);
              GETVAL(version, valueint);
              GETVALSTR(last_modified_at);
              cJSON_Delete(root);
           }
         break;
      case EMS_REQUEST_STATE_INFO:
         break;
      default:
         _request_free(req);
         break;
     }

   return EINA_TRUE;
}




/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

EAPI void
ems_grabber_grab(const char *filename, Ems_Media_Type type)
{
   char url[PATH_MAX];
   Ecore_Con_Url *ec_url = NULL;
   char *tmp;
   char *search;
   Ems_Tmdb_Req *req;

   if (type != EMS_MEDIA_TYPE_VIDEO)
     return;

   INF("Grab %s of type %d", filename, type);

   tmp = ems_utils_decrapify(filename);
   search = ems_utils_escape_string(tmp);
   free(tmp);

   snprintf(url, sizeof (url), EMS_TMDB_QUERY_SEARCH,
            EMS_TMDB_API_KEY, search);
   free(search);

   DBG("Search for %s", url);
   ec_url = ecore_con_url_new(url);
   if (!ec_url)
     {
        ERR("error when creating ecore con url object.");
        return;
     }

   req = calloc(1, sizeof(Ems_Tmdb_Req));
   req->search = eina_stringshare_add(search);
   req->ec_url = ec_url;
   req->buf = NULL;
   ecore_con_url_data_set(ec_url, req);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, (Ecore_Event_Handler_Cb)_search_complete_cb, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, (Ecore_Event_Handler_Cb)_search_data_cb, NULL);
   req->state = EMS_REQUEST_STATE_SEARCH;
   if (!ecore_con_url_get(ec_url))
     {
        ERR("could not realize request.");
        _request_free(req);
        ecore_con_url_free(ec_url);
     }
}

EINA_MODULE_INIT(_grabber_tmdb_init);
EINA_MODULE_SHUTDOWN(_grabber_tmdb_shutdown);

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
