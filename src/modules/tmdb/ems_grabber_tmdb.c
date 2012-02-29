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

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

#define EMS_TMDB_API_KEY "ba2eed549e5c84e712931fef2b69bfb1"
#define EMS_TMDB_QUERY_SEARCH "http://api.themoviedb.org/2.1/Movie.search/en/xml/%s/%s"
#define EMS_TMDB_QUERY_INFO   "http://api.themoviedb.org/2.1/Movie.getInfo?id=%s&api_key=%s"
#define VH_ISALNUM(c) isalnum ((int) (unsigned char) (c))
#define VH_ISGRAPH(c) isgraph ((int) (unsigned char) (c))
#define VH_ISSPACE(c) isspace ((int) (unsigned char) (c))
#define IS_TO_DECRAPIFY(c)                \
 ((unsigned) (c) <= 0x7F                  \
  && (c) != '\''                          \
  && !VH_ISSPACE (c)                      \
  && !VH_ISALNUM (c))

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
   Eina_Strbuf *buf = ecore_con_url_data_get(ev->url_con);

   //DBG("Received %i bytes of lyric: %s", ev->size, ecore_con_url_url_get(ev->url_con));
   if (buf)
     eina_strbuf_append_length(buf, (char*)&ev->data[0], ev->size);
   else
     {
        buf = eina_strbuf_new();
        eina_strbuf_append_length(buf, (char*)&ev->data[0], ev->size);
     }
   ecore_con_url_data_set(ev->url_con, buf);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_search_complete_cb(void *data, int type, void *event_info)
{
   Ecore_Con_Event_Url_Complete *url_complete = event_info;

   Eina_Strbuf *buf = ecore_con_url_data_get(url_complete->url_con);

   DBG("download completed with status code: %d", url_complete->status);

   DBG("Received Data : %s", eina_strbuf_string_get(buf));

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
   //ecore_con_url_data_set(ec_url, eina_stringshare_add(filename));
   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, (Ecore_Event_Handler_Cb)_search_complete_cb, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, (Ecore_Event_Handler_Cb)_search_data_cb, NULL);
   if (!ecore_con_url_get(ec_url))
     {
        ERR("could not realize request.");
        //eina_stringshare_del(filename);
        ecore_con_url_free(ec_url);
     }
}

EINA_MODULE_INIT(_grabber_tmdb_init);
EINA_MODULE_SHUTDOWN(_grabber_tmdb_shutdown);

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
