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

#include <Eina.h>
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_Con.h>

#include "Ems.h"
#include "ems_private.h"
#include "ems_server.h"
#include "ems_server_protocol.h"
#include "Ecore_Con_Eet.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Ems_Server_Cb Ems_Server_Cb;
typedef struct _Ems_Server_Media_Get_Cb Ems_Server_Media_Get_Cb;

struct _Ems_Server_Media_Get_Cb
{
   void (*add_cb)(void *data, Ems_Server *server, const char *media);
   void *data;
};

struct _Ems_Server_Cb
{
   void (*add_cb)(void *data, Ems_Server *server);
   void (*del_cb)(void *data, Ems_Server *server);
   void (*update_cb)(void *data, Ems_Server *server);
   void (*connected_cb)(void *data, Ems_Server *server);
   void (*disconnected_cb)(void *data, Ems_Server *server);
   void *data;
};


static Ecore_Con_Server *conn = NULL;
static Ecore_Con_Eet *ece = NULL;
static Eet_Data_Descriptor *edd = NULL;

static Eina_List *_servers = NULL;
static Eina_List *_servers_cb = NULL;

static void
_get_medias_req_cb(void *data, Ecore_Con_Reply *reply __UNUSED__, const char *name __UNUSED__, void *value __UNUSED__)
{
   Ems_Server *server = data;

   Eina_List *files = ems_database_collection_get(ems_config->db, NULL);

   DBG("Get medias req cb %s", name);

   ecore_con_eet_send(server->reply, "get_medias", files);
}

static void
_get_medias_cb(void *data, Ecore_Con_Reply *reply, const char *name, void *value)
{
   Eina_List *files = value;
   const char *f;
   Eina_List *l;

   EINA_LIST_FOREACH(files, l, f)
     DBG("%s", f);

}

static Eina_Bool
_connected_to_server_cb(void *data, Ecore_Con_Reply *reply, Ecore_Con_Server *conn)
{
   Ems_Server *server = data;
   Eina_List *l_cb;
   Ems_Server_Cb *cb;

   if (!server)
     return EINA_FALSE;

   server->reply = reply;

   DBG("Connected to %s:%d (%s)", server->ip, server->port, server->name);

   if (server->is_connected)
       return EINA_TRUE;
   else
       server->is_connected = EINA_TRUE;

   EINA_LIST_FOREACH(_servers_cb, l_cb, cb)
     {
        if (cb->connected_cb)
          cb->connected_cb(cb->data, server);
     }
   return EINA_TRUE;
}

static Eina_Bool
_ems_server_connect(Ems_Server *server)
{
   if (!server)
     return EINA_FALSE;

   INF("try to connect to %s:%d", server->name, server->port);

   if (server->is_connected)
     return EINA_TRUE;

   server->ecore_conn = ecore_con_server_connect(ECORE_CON_REMOTE_TCP, server->ip, server->port, server);
   server->ece = ecore_con_eet_client_new(server->ecore_conn);
   ecore_con_eet_register(server->ece, "get_medias_req", ems_server_protocol_edd_get("get_medias_req"));
   ecore_con_eet_register(server->ece, "get_medias", ems_server_protocol_edd_get("get_medias"));
   ecore_con_eet_data_callback_add(server->ece, "get_medias_req", _get_medias_req_cb, server);
   ecore_con_eet_data_callback_add(server->ece, "get_medias", _get_medias_cb, server);
   ecore_con_eet_server_connect_callback_add(server->ece, _connected_to_server_cb, server);
   return EINA_TRUE;
}

static Eina_Bool
_new_client_connected_cb(void *data, Ecore_Con_Reply *reply, Ecore_Con_Server *conn)
{
   Ecore_Con_Eet *ece;

   DBG("New connection");
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool ems_server_init(void)
{
   eina_init();
   eet_init();
   ecore_init();
   ecore_con_init();

   /* Add Ems instance server */
   conn = ecore_con_server_add(ECORE_CON_REMOTE_TCP, "0.0.0.0", ems_config->port, NULL);
   if (!conn)
     return EINA_FALSE;

   /* Create ecore con eet server */
   ece = ecore_con_eet_server_new(conn);

   /* Register protocol function */
   ecore_con_eet_register(ece, "get_medias_req", ems_server_protocol_edd_get("get_medias_req"));
   ecore_con_eet_register(ece, "get_medias", ems_server_protocol_edd_get("get_medias"));
   ecore_con_eet_data_callback_add(ece, "get_medias_req", _get_medias_req_cb, NULL);
   ecore_con_eet_data_callback_add(ece, "get_medias", _get_medias_cb, NULL);

   /* Add a callback to be informed when a new client is connected to our server */
   ecore_con_eet_server_connect_callback_add(ece, (Ecore_Con_Eet_Server_Cb)_new_client_connected_cb, NULL);

   return EINA_TRUE;
}

void ems_server_shutdown()
{
   ecore_con_eet_server_free(ece);
}

void ems_server_add(Ems_Server *server)
{
   Eina_List *l_cb;
   Ems_Server_Cb *cb;

   if (!server || !server->name)
     return;

   INF("Adding %s to the list of detected server", server->name);

   EINA_LIST_FOREACH(_servers_cb, l_cb, cb)
     {
        if (cb->add_cb)
          cb->add_cb(cb->data, server);
     }

   _servers = eina_list_append(_servers, server);
}

void ems_server_del(const char *name)
{
   Eina_List *l;
   Ems_Server *server;

   if (!_servers || !name)
     return;

   EINA_LIST_FOREACH(_servers, l, server)
     {
        if (!server) continue;
        if (!strcmp(name, server->name))
          {
             Eina_List *l_cb;
             Ems_Server_Cb *cb;

             INF("Remove %s from the list of detected servers", server->name);

             EINA_LIST_FOREACH(_servers_cb, l_cb, cb)
               {
                  if (cb->del_cb)
                    cb->del_cb(cb->data, server);
               }

             _servers = eina_list_remove(_servers, server);
             eina_stringshare_del(server->name);
             if (server->ip) eina_stringshare_del(server->ip);
             free(server);
          }
     }
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
Eina_List *
ems_server_list_get(void)
{
   return _servers;
}

void
ems_server_cb_set(Ems_Server_Add_Cb server_add_cb,
                  Ems_Server_Del_Cb server_del_cb,
                  Ems_Server_Update_Cb server_update_cb,
                  Ems_Server_Connected_Cb server_connected_cb,
                  Ems_Server_Disconnected_Cb server_disconnected_cb,
                  void *data)
{
   Ems_Server_Cb *cb;

   cb = calloc(1, sizeof(Ems_Server_Cb));
   if (!cb)
     return;

   cb->add_cb = server_add_cb;
   cb->del_cb = server_del_cb;
   cb->update_cb = server_update_cb;
   cb->connected_cb = server_connected_cb;
   cb->disconnected_cb = server_disconnected_cb;
   cb->data = data;

   _servers_cb = eina_list_append(_servers_cb, cb);
}

void
ems_server_cb_del(Ems_Server_Add_Cb server_add_cb,
                  Ems_Server_Del_Cb server_del_cb,
                  Ems_Server_Update_Cb server_update_cb)
{
   Eina_List *l;
   Ems_Server_Cb *cb;

   EINA_LIST_FOREACH(_servers_cb, l, cb)
     {
        int cnt = 0;
        if (server_add_cb == cb->add_cb)
          {
             cb->add_cb = NULL;
             cnt++;
          }
        if (server_del_cb == cb->del_cb)
          {
             cb->del_cb = NULL;
             cnt++;
          }
        if (server_update_cb == cb->update_cb)
          {
             cb->add_cb = NULL;
             cnt++;
          }
        if (cnt == 3)
          {
             _servers_cb = eina_list_remove(_servers_cb, cb);
             free(cb);
          }
     }
}

const char *
ems_server_name_get(Ems_Server *server)
{
   DBG("");
   if (!server)
     return NULL;

   return server->name;
}

Eina_Bool
ems_server_is_local(Ems_Server *server)
{
   DBG("");

   if (server)
     return server->is_local;
   else
     return EINA_TRUE;
}

Eina_Bool
ems_server_is_connected(Ems_Server *server)
{
   DBG("");

   if (server)
     return server->is_connected;
   else
     return EINA_TRUE;
}

Eina_Bool
ems_server_connect(Ems_Server *server)
{
   DBG("");
   if (!server)
     return EINA_FALSE;

   if (server->is_connected)
     return EINA_TRUE;

    return _ems_server_connect(server);
}

char *
ems_server_media_stream_url_get(Ems_Server *server, const char *media_uuid)
{
   Eina_Strbuf *uri;
   char *str;

   if (!server || !media_uuid)
     return NULL;

   uri = eina_strbuf_new();
   eina_strbuf_append_printf(uri, "http://%s:%d/item/%s", server->ip, ems_config->port_stream, media_uuid);
   DBG("url get: %s", eina_strbuf_string_get(uri));
   str = strdup(eina_strbuf_string_get(uri));
   eina_strbuf_free(uri);

   return str;
}

Ems_Observer *
ems_server_dir_get(Ems_Server *server,
                   const char *path,
                   Ems_Media_Type type __UNUSED__,
                   Ems_Media_Add_Cb media_add __UNUSED__,
                   Ems_Media_Del_Cb media_del __UNUSED__,
                   Ems_Media_Done_Cb media_done __UNUSED__,
                   Ems_Media_Error_Cb media_error __UNUSED__,
                   void *data __UNUSED__)
{
   if (!server || !path)
     return NULL;

   if (!server->is_connected)
     _ems_server_connect(server);

   return NULL;

}

Ems_Observer *
ems_server_media_get(Ems_Server *server,
                     Ems_Collection *collection,
                     Ems_Media_Add_Cb media_add,
                     void *data)
{
   Get_Medias_Req *req;
   DBG("");

   req = calloc(1, sizeof(Get_Medias_Req));
   req->collection = collection;

   ecore_con_eet_send(server->reply, "get_medias_req", req);

   return NULL;
}

Ems_Observer *
ems_server_media_info_get(Ems_Server *server,
                          const char *uuid,
                          const char *info,
                          Ems_Media_Info_Add_Cb info_add,
                          Ems_Media_Info_Add_Cb info_del,
                          Ems_Media_Info_Update_Cb info_update,
                          void *data)
{
   DBG("");

   return NULL;
}
