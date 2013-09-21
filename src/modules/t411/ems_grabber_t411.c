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

#define EMS_T411_API_KEY "ba2eed549e5c84e712931fef2b69bfb1"
#define EMS_T411_AUTH_URL "https://api.t411.me/auth"
#define EMS_T411_AUTH_PARAMS "username=%s&password=%s"
#define EMS_T411_QUERY_SEARCH "/torrents/search/%s"
#define EMS_T411_QUERY_INFO   "http://api.themoviedb.org/2.1/Movie.getInfo?id=%s&api_key=%s"
#define VH_ISALNUM(c) isalnum ((int) (unsigned char) (c))
#define VH_ISGRAPH(c) isgraph ((int) (unsigned char) (c))
#define VH_ISSPACE(c) isspace ((int) (unsigned char) (c))
#define IS_TO_DECRAPIFY(c)                      \
  ((unsigned) (c) <= 0x7F                       \
   && (c) != '\''                               \
   && !VH_ISSPACE (c)                           \
   && !VH_ISALNUM (c))

typedef struct _Ems_T411_Req Ems_T411_Req;
typedef enum _Ems_Request_State Ems_Request_State;
typedef struct _Ems_T411_Stats Ems_T411_Stats;

enum _Ems_Request_State
  {
    EMS_REQUEST_STATE_AUTH,
    EMS_REQUEST_STATE_SEARCH,
    EMS_REQUEST_STATE_INFO
  };

struct _Ems_T411_Req
{
   const char *filename;
   const char *search;
   Eina_Strbuf *buf;
   Ecore_Con_Url *ec_url;
   Ecore_Con_Url *next_ec_url;
   Ems_T411_Req *next_req;
   Ems_Request_State state;
   Ems_Grabber_Data *grabbed_data;
   Ems_Grabber_End_Cb end_cb;
   void *data;
};

struct _Ems_T411_Stats
{
   int total;
   int files_grabbed;
   int covers_grabbed;
   int backdrop_grabbed;
   int multiple_results;
};

static Eina_Hash *_hash_req = NULL;
static Ems_T411_Stats *_stats = NULL;
static const char *auth_token = NULL;

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
_grabber_t411_shutdown(void)
{
   INF("Shutdown T411 grabber");
   eina_hash_free(_hash_req);
   free(_stats);
   ecore_con_url_shutdown();
   ecore_con_shutdown();
}


static Eina_Bool
_search_data_cb(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Data *ev)
{
   Ems_T411_Req *req = eina_hash_find(_hash_req, ev->url_con);

   if (!req)
     return  ECORE_CALLBACK_PASS_ON;

   if (req->buf)
     eina_strbuf_append_length(req->buf, (char*)&ev->data[0], ev->size);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_search_complete_cb(void *data __UNUSED__, int type __UNUSED__, void *event_info)
{
   Ecore_Con_Event_Url_Complete *url_complete = event_info;

   Ems_T411_Req *req = eina_hash_find(_hash_req, url_complete->url_con);

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
      case EMS_REQUEST_STATE_AUTH:
        {
           char *token;
           char *tmp;
           const char *buf = eina_strbuf_string_get(req->buf);
           
           token = strstr(buf, "token\":\"");
           tmp = token + 8;
           token = tmp;
           tmp = strstr(tmp, "\"}");
           *tmp = '\0';
           printf("token : %s\n", token);
           auth_token = eina_stringshare_add(token);
           eina_hash_add(_hash_req, req->next_ec_url, req->next_req);
           ecore_con_url_additional_header_add(req->next_ec_url, "content-type", "application/x-www-form-urlencoded");
           ecore_con_url_additional_header_add(req->next_ec_url, "authorization", auth_token);
           if (!ecore_con_url_post(req->next_ec_url, NULL, -1, "application/x-www-form-urlencoded"))
             {
                ERR("could not realize request.");
                eina_hash_del(_hash_req, req->ec_url, req);
             }

           break;
        }
      case EMS_REQUEST_STATE_SEARCH:
         if (req->buf)
           {
              cJSON *root;
              cJSON *m;
              int size = 0;
              int i;
              const char *buf = eina_strbuf_string_get(req->buf);

              if (!buf)
                {
                   if (req->end_cb)
                     req->end_cb(req->data, req->filename, req->grabbed_data);
                   //ecore_con_url_free(req->ec_url);
                   eina_hash_del(_hash_req, req->ec_url, req);

                   return ECORE_CALLBACK_DONE;
                }

              DBG("Search request data : %s", eina_strbuf_string_get(req->buf));
              root = cJSON_Parse(buf);
              if (root)
                size = cJSON_GetArraySize(root);
              //DBG("%s", cJSON_Print(root));
              //DBG("Size %d", size);

              if (!size)
                {
                   DBG("No result found");
                   if (req->end_cb)
                       req->end_cb(req->data, req->filename, req->grabbed_data);
                   //ecore_con_url_free(req->ec_url);
                   eina_hash_del(_hash_req, req->ec_url, req);
                   return ECORE_CALLBACK_DONE;
                }

              for (i = 0; i < size; i++)
              {
                  Eina_Hash *hash;

                  hash = eina_hash_string_superfast_new(NULL);
                  req->grabbed_data->data = eina_list_append(req->grabbed_data->data, hash);
                  m = cJSON_GetArrayItem(root, i);
                  if (!m)
                  {
                      ERR("Unable to get movie info");
                      break;
                  }
                  _stats->files_grabbed++;
                  PUTVAL("id", "id", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("name", "name", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("category", "category", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("rewritename", "rewritename", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("seeders", "seeders", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("leechers", "leechers", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("comments", "comments", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("isVerified", "isVerified", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("added", "added", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("size", "size", valuedouble, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("times_completed", "times_completed", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("owner", "owner", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("categoryname", "categoryname", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("categoryimage", "categoryimage", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("username", "username", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
                  PUTVAL("privacy", "privacy", valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              }

              if (req->end_cb)
                  req->end_cb(req->data, req->filename, req->grabbed_data);
              cJSON_Delete(root);
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
_request_free_cb(Ems_T411_Req *req)
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
_grabber_t411_init(void)
{
   INF("Init T411 grabber");
   ecore_con_init();
   ecore_con_url_init();

   _hash_req = eina_hash_pointer_new((Eina_Free_Cb)_request_free_cb);

   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, (Ecore_Event_Handler_Cb)_search_complete_cb, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, (Ecore_Event_Handler_Cb)_search_data_cb, NULL);

   _dom = eina_log_domain_register("ems_grabber_t411", ENNA_DEFAULT_LOG_COLOR);

   _stats = calloc(1, sizeof(Ems_T411_Stats));

   return EINA_TRUE;
}


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

EAPI void
ems_grabber_grab(Ems_Grabber_Params *params, Ems_Grabber_End_Cb end_cb, void *data)
{
   Ecore_Con_Url *ec_url = NULL;
   
   char *s;
   Ems_T411_Req *req;

   if (!params)
       return;

   DBG("Grab %s of type %d", params->filename, params->type);
   _stats->total++;

   req = calloc(1, sizeof(Ems_T411_Req));
   req->filename = eina_stringshare_add(params->filename);
   printf("params->search : %s\n", params->filename);
   if (!strncmp(params->filename, "top/", 4))
     {
        char url[PATH_MAX];
        snprintf(url, sizeof(url), "https://api.t411.me/torrents/%s", params->filename);
        printf("Search for : %s\n", url);
        ec_url = ecore_con_url_new(url);
     }
   else
     {
        char url[PATH_MAX];
        s = ems_utils_escape_string(params->search);
        snprintf(url, sizeof(url), "https://api.t411.me/torrents/search/%s", s);
        ec_url = ecore_con_url_new(url);
        req->search = eina_stringshare_add(s);
        free(s);
     }
   if (!ec_url)
     {
        ERR("error when creating ecore con url object.");
        return;
     }

   req->ec_url = ec_url;
   req->end_cb = end_cb;
   req->data = data;
   req->buf = eina_strbuf_new();
   req->grabbed_data = calloc(1, sizeof(Ems_Grabber_Data));
   req->grabbed_data->data = NULL;
   req->state = EMS_REQUEST_STATE_SEARCH;

   /* printf("Auth token : %s\n", auth_token); */
   if (!auth_token)
     {
        Ems_T411_Req *auth_req;
        Ecore_Con_Url *ec_auth_url = NULL;
        char auth_data[PATH_MAX];

        ec_auth_url = ecore_con_url_new(EMS_T411_AUTH_URL);
        if (!ec_auth_url)
          {
             ERR("error when creating ecore con url object.");
             return;
          }

        auth_req = calloc(1, sizeof(Ems_T411_Req));
        auth_req->filename = NULL;
        auth_req->search = NULL;
        auth_req->next_ec_url = req->ec_url;
        auth_req->next_req = req;
        auth_req->ec_url = ec_auth_url;
        auth_req->end_cb = NULL;
        auth_req->data = NULL;
        auth_req->buf = eina_strbuf_new();
        auth_req->state = EMS_REQUEST_STATE_AUTH;
        printf("%d\n",  auth_req->state);
        eina_hash_add(_hash_req, ec_auth_url, auth_req);
 
        snprintf(auth_data, sizeof(auth_data), EMS_T411_AUTH_PARAMS, "captain1gloo", "g4prpd3");
        ecore_con_url_additional_header_add(ec_auth_url, "content-type", "application/x-www-form-urlencoded");
        DBG("Post authentication");
        if (!ecore_con_url_post(ec_auth_url, auth_data, strlen(auth_data), "application/x-www-form-urlencoded"))
          {
             ERR("could not realize request.");
             eina_hash_del(_hash_req, ec_auth_url, auth_req);
             return;
          }
     }
   else
     {
        eina_hash_add(_hash_req, ec_url, req);
        ecore_con_url_additional_header_add(ec_url, "content-type", "application/x-www-form-urlencoded");
        ecore_con_url_additional_header_add(ec_url, "authorization", auth_token);
        if (!ecore_con_url_post(ec_url, NULL, -1, NULL))
          {
             ERR("could not realize request.");
             eina_hash_del(_hash_req, ec_url, req);
          }
     }
}

EAPI void
ems_grabber_stats(void)
{
   INF("Stats for T411 module");
   INF("Total files grabbed : %d", _stats->total);
   INF("Files grabbed : %d (%3.3f%%)", _stats->files_grabbed,  _stats->files_grabbed * 100.0 / _stats->total);
   INF("Covers grabbed : %d (%3.3f%%)", _stats->covers_grabbed,  _stats->files_grabbed * 100.0 / _stats->total);
   INF("Backdrop grabbed : %d (%3.3f%%)", _stats->backdrop_grabbed,  _stats->backdrop_grabbed * 100.0 / _stats->total);
   INF("Multipled results : %d (%3.3f%%)", _stats->multiple_results,  _stats->multiple_results * 100.0 / _stats->total);
   INF("End of stats");
}

EINA_MODULE_INIT(_grabber_t411_init);
EINA_MODULE_SHUTDOWN(_grabber_t411_shutdown);

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
