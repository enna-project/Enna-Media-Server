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

#define EMS_BETASERIES_API_KEY             "a209ae2a0357"
#define EMS_BETASERIES_QUERY_SEARCH        "http://api.betaseries.com/shows/search.xml?key=%s&title=%s"
#define EMS_BETASERIES_QUERY_INFO          "http://api.betaseries.com/shows/display/%s.xml?key=%s"
#define EMS_BETASERIES_QUERY_EPISODE_INFO  "http://api.betaseries.com/shows/episodes/%s.xml?key=%s&season=%d&episode=%d&hide_notes=1"

using namespace tinyxml2;

typedef enum _Ems_Request_State
{
   EMS_REQUEST_STATE_SEARCH,
   EMS_REQUEST_STATE_INFO,
   EMS_REQUEST_STATE_EPISODE_INFO,
} Ems_Request_State;

typedef struct _Ems_Betaseries_Req
{
   const char *filename;
   const char *search;
   Eina_Strbuf *buf;
   Ecore_Con_Url *ec_url;
   Ems_Request_State state;
   Ems_Grabber_Data *grabbed_data;
   Ems_Grabber_End_Cb end_cb;
   Ems_Grabber_Params params;
   void *data;
} Ems_Betaseries_Req;

typedef struct _Ems_Betaseries_Stats
{
   int total;
   int files_grabbed;
   int covers_grabbed;
   int backdrop_grabbed;
   int multiple_results;
} Ems_Betaseries_Stats;

static Eina_Hash *_hash_req = NULL;
static Ems_Betaseries_Stats *_stats = NULL;

#define STORE_METADATA(key, val, where)                               \
   do {                                                               \
        Eina_Value *v = eina_value_new(EINA_VALUE_TYPE_STRINGSHARE);  \
        eina_value_set(v, val);                                       \
        eina_hash_add(req->grabbed_data->where, key, v);              \
        DBG("Store metadata \"%s\" --> \"%s\"", key,                  \
            eina_value_to_string(v));                                 \
   } while(0);                                                        \

static void
_grabber_thetvdb_shutdown(void)
{
   INF("Shutdown Betaseries grabber");
   eina_hash_free(_hash_req);
   ecore_con_url_shutdown();
   ecore_con_shutdown();
}

static Eina_Bool
_search_data_cb(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Data *ev)
{
   Ems_Betaseries_Req *req = (Ems_Betaseries_Req *)eina_hash_find(_hash_req, ev->url_con);

   if (!req)
     return  ECORE_CALLBACK_PASS_ON;

   if (req->buf)
     eina_strbuf_append_length(req->buf, (char*)&ev->data[0], ev->size);

   return ECORE_CALLBACK_DONE;
}

static void
_call_end_cb(Ems_Betaseries_Req *req, Eina_Bool del)
{
   if (req && req->end_cb)
     {
        req->end_cb(req->data, req->filename, req->grabbed_data);
        if (del)
          eina_hash_del(_hash_req, req->ec_url, req);
     }
}

static Eina_Bool
_search_complete_cb(void *data __UNUSED__, int type __UNUSED__, void *event_info)
{
   Ecore_Con_Event_Url_Complete *url_complete = (Ecore_Con_Event_Url_Complete *)event_info;
   Ems_Betaseries_Req *req = (Ems_Betaseries_Req *)eina_hash_find(_hash_req, url_complete->url_con);

   if (!req)
     return ECORE_CALLBACK_PASS_ON;

   DBG("download completed for %s with status code: %d", req->filename, url_complete->status);

   if (url_complete->status != 200)
     {
        _call_end_cb(req, EINA_TRUE);
        return ECORE_CALLBACK_DONE;
     }
   
   switch(req->state)
     {
      case EMS_REQUEST_STATE_SEARCH:
         if (req->buf)
           {
              int nb_results = 0;
              const char *buf = eina_strbuf_string_get(req->buf);
              char *seriesid = NULL;
              Eina_Strbuf *url;

              if (!buf)
                {
                   DBG("No data");
                   _call_end_cb(req, EINA_TRUE);

                   return ECORE_CALLBACK_DONE;
                }

              //DBG("Search request data : %s", buf);

              //Parse the returned xml, we are in C++ here, do it that way
              XMLDocument doc;
              if (doc.Parse(buf) != XML_NO_ERROR)
                {
                   DBG("Unable to parse, (%s) (%s)", doc.GetErrorStr1(), doc.GetErrorStr2());
                   _call_end_cb(req, EINA_TRUE);

                   return ECORE_CALLBACK_DONE;
                }

              XMLElement *node = doc.FirstChildElement("root");
              if (node)
                node = node->FirstChildElement("shows");
              if (node)
                node = node->FirstChildElement("show");
              for(;node;node = node->NextSiblingElement())
                {
                   nb_results++;
                   if (node->FirstChildElement("url") && !seriesid)
                     {
                        XMLText* textNode = node->FirstChildElement("url")->FirstChild()->ToText();
                        seriesid = strdup(textNode->Value());
                        DBG("Found series id %s", seriesid);
                     }
                }

              if (!seriesid)
                {
                   DBG("In %d results, can't find url tag", nb_results);
                   _call_end_cb(req, EINA_TRUE);

                   return ECORE_CALLBACK_DONE;
                }

              url = eina_strbuf_new();
              eina_strbuf_append_printf(url,
                                        EMS_BETASERIES_QUERY_INFO,
                                        seriesid,
                                        EMS_BETASERIES_API_KEY);

              ecore_con_url_url_set(req->ec_url, eina_strbuf_string_get(url));
              eina_strbuf_free(url);
              free(seriesid);
              eina_strbuf_reset(req->buf);

              req->state = EMS_REQUEST_STATE_INFO;

              if (!ecore_con_url_get(req->ec_url))
                {
                   ERR("could not realize request.");
                   _call_end_cb(req, EINA_TRUE);
                   ecore_con_url_free(req->ec_url);
                }
                                        
           }
         break;
      case EMS_REQUEST_STATE_INFO:
         if (req->buf)
           {
              const char *buf = eina_strbuf_string_get(req->buf);
              Eina_Strbuf *url;
              char *seriesid = NULL;

              if (!buf)
                {
                   DBG("No data");
                   _call_end_cb(req, EINA_TRUE);

                   return ECORE_CALLBACK_DONE;
                }

              //DBG("Search request data : %s", buf);

              XMLDocument doc;
              if (doc.Parse(buf) != XML_NO_ERROR)
                {
                   DBG("Unable to parse, (%s) (%s)", doc.GetErrorStr1(), doc.GetErrorStr2());
                   _call_end_cb(req, EINA_TRUE);

                   return ECORE_CALLBACK_DONE;
                }

              XMLElement *node = doc.FirstChildElement("root");
              if (node)
                node = node->FirstChildElement("show");
              if (node)
                node = node->FirstChildElement();
              for(;node;node = node->NextSiblingElement())
                {
                   if (!strcmp(node->Value(), "description"))
                     {
                        XMLText* textNode = node->FirstChild()->ToText();
                        STORE_METADATA("overview", textNode->Value(), data);
                     }
                   else if (!strcmp(node->Value(), "title"))
                     {
                        XMLText* textNode = node->FirstChild()->ToText();
                        STORE_METADATA("title", textNode->Value(), data);
                     }
                   else if (!strcmp(node->Value(), "Status"))
                     {
                        XMLText* textNode = node->FirstChild()->ToText();
                        STORE_METADATA("status", textNode->Value(), data);
                     }
                   else if (!strcmp(node->Value(), "network"))
                     {
                        XMLText* textNode = node->FirstChild()->ToText();
                        STORE_METADATA("network", textNode->Value(), data);
                     }
                   else if (!strcmp(node->Value(), "url"))
                     {
                        XMLText* textNode = node->FirstChild()->ToText();
                        STORE_METADATA("betaseries_id", textNode->Value(), data);
                        seriesid = strdup(textNode->Value());
                     }
                }

              if (!seriesid)
                {
                   _call_end_cb(req, EINA_TRUE);
                   return ECORE_CALLBACK_DONE;
                }

              url = eina_strbuf_new();
              eina_strbuf_append_printf(url,
                                        EMS_BETASERIES_QUERY_EPISODE_INFO,
                                        seriesid,
                                        EMS_BETASERIES_API_KEY,
                                        req->params.season,
                                        req->params.episode);

              ecore_con_url_url_set(req->ec_url, eina_strbuf_string_get(url));
              eina_strbuf_free(url);
              free(seriesid);
              eina_strbuf_reset(req->buf);

              req->state = EMS_REQUEST_STATE_EPISODE_INFO;

              if (!ecore_con_url_get(req->ec_url))
                {
                   ERR("could not realize request.");
                   _call_end_cb(req, EINA_TRUE);
                   ecore_con_url_free(req->ec_url);
                }

            }
         break;
      case EMS_REQUEST_STATE_EPISODE_INFO:
         if (req->buf)
           {
              const char *buf = eina_strbuf_string_get(req->buf);

              if (!buf)
                {
                   DBG("No data");
                   _call_end_cb(req, EINA_TRUE);

                   return ECORE_CALLBACK_DONE;
                }

              //DBG("Search request data : %s", buf);

              XMLDocument doc;
              if (doc.Parse(buf) != XML_NO_ERROR)
                {
                   DBG("Unable to parse, (%s) (%s)", doc.GetErrorStr1(), doc.GetErrorStr2());
                   _call_end_cb(req, EINA_TRUE);

                   return ECORE_CALLBACK_DONE;
                }

              XMLElement *node = doc.FirstChildElement("root");
              if (node)
                node = node->FirstChildElement("seasons");
              if (node)
                node = node->FirstChildElement("season");
              if (node)
                node = node->FirstChildElement("episodes");
              if (node)
                node = node->FirstChildElement("episode");
              if (node)
                node = node->FirstChildElement();
              for(;node;node = node->NextSiblingElement())
                {
                   if (!strcmp(node->Value(), "title"))
                     {
                        XMLText* textNode = node->FirstChild()->ToText();
                        STORE_METADATA("title", textNode->Value(), episode_data);
                     }
                   else if (!strcmp(node->Value(), "description"))
                     {
                        XMLText* textNode = node->FirstChild()->ToText();
                        STORE_METADATA("overview", textNode->Value(), episode_data);
                     }
                   else if (!strcmp(node->Value(), "screen"))
                     {
                        //TODO: download images
                     }
                }

              _call_end_cb(req, EINA_TRUE);
            }
         break;
      default:
         break;
     }

   return ECORE_CALLBACK_DONE;
}

static void
_request_free_cb(void *data)
{
   Ems_Betaseries_Req *req = (Ems_Betaseries_Req *)data;
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
   INF("Init Betaseries grabber");
   ecore_con_init();
   ecore_con_url_init();

   _hash_req = eina_hash_pointer_new((Eina_Free_Cb)_request_free_cb);

   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, (Ecore_Event_Handler_Cb)_search_complete_cb, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, (Ecore_Event_Handler_Cb)_search_data_cb, NULL);

   _dom = eina_log_domain_register("ems_grabber_betaseries", ENNA_DEFAULT_LOG_COLOR);

   _stats = (Ems_Betaseries_Stats *)calloc(1, sizeof(Ems_Betaseries_Stats));

   return EINA_TRUE;
}


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

EAPI void
ems_grabber_grab(const char *filename, Ems_Media_Type type, Ems_Grabber_Params params, Ems_Grabber_End_Cb end_cb, void *data)
{
   Eina_Strbuf *url = NULL;
   Ecore_Con_Url *ec_url = NULL;
   char *tmp;
   char *search = NULL;
   Ems_Betaseries_Req *req;

   if (type != EMS_MEDIA_TYPE_TVSHOW)
     {
        DBG("Not for me: %d != %d", type, EMS_MEDIA_TYPE_TVSHOW);
        return;
     }

   DBG("Grab %s of type %d (episode:%d season:%d)", filename, type, params.episode, params.season);

   search = ems_utils_escape_string(filename);

   url = eina_strbuf_new();
   eina_strbuf_append_printf(url,
                             EMS_BETASERIES_QUERY_SEARCH,
                             EMS_BETASERIES_API_KEY,
                             search);

   DBG("Search for %s", eina_strbuf_string_get(url));

   ec_url = ecore_con_url_new(eina_strbuf_string_get(url));
   eina_strbuf_free(url);
   if (!ec_url)
     {
        ERR("error when creating ecore con url object.");
        return;
     }

   req = (Ems_Betaseries_Req *)calloc(1, sizeof(Ems_Betaseries_Req));
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
   req->grabbed_data->episode_data = eina_hash_string_superfast_new(NULL);
   req->grabbed_data->lang = "fr"; //FIXME
   req->params = params;
   req->state = EMS_REQUEST_STATE_SEARCH;

   eina_hash_add(_hash_req, ec_url, req);

   _stats->total++;

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
   INF("Stats for Betaseries module");
   INF("Total files grabbed : %d", _stats->total);
   INF("Files grabbed : %d (%3.3f%%)", _stats->files_grabbed,  _stats->files_grabbed * 100.0 / _stats->total);
   INF("Covers grabbed : %d (%3.3f%%)", _stats->covers_grabbed,  _stats->files_grabbed * 100.0 / _stats->total);
   INF("Backdrop grabbed : %d (%3.3f%%)", _stats->backdrop_grabbed,  _stats->backdrop_grabbed * 100.0 / _stats->total);
   INF("Multipled results : %d (%3.3f%%)", _stats->multiple_results,  _stats->multiple_results * 100.0 / _stats->total);
   INF("End of stats");
}

EINA_MODULE_INIT(_grabber_thetvdb_init);
EINA_MODULE_SHUTDOWN(_grabber_thetvdb_shutdown);

#ifdef __cplusplus
}
#endif /* ifdef __cplusplus */

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
