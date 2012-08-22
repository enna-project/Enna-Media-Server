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
#include "ems_server_eet.h"
#include "ems_server_protocol.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Ems_Server_Cb Ems_Server_Cb;
typedef struct _Ems_Server_Media_Get_Cb Ems_Server_Media_Get_Cb;
typedef struct _Ems_Server_Media_Infos_Get_Cb Ems_Server_Media_Infos_Get_Cb;

struct _Ems_Server_Media_Get_Cb
{
   void (*add_cb)(void *data, Ems_Server *server, const char *media);
   void *data;
};

struct _Ems_Server_Media_Infos_Get_Cb
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

static Eina_List *_servers = NULL;
static Eina_List *_servers_cb = NULL;



static Eina_Bool
_server_connected_cb(void *data, Ecore_Con_Reply *reply, Ecore_Con_Server *conn)
{
   Ems_Server *server = data;
   Eina_List *l_cb;
   Ems_Server_Cb *cb;
   DBG("Connected to %s:%d (%s)", server->ip, server->port, server->name);

   if (server->is_connected)
     return EINA_TRUE;
   else
     server->is_connected = EINA_TRUE;

   server->reply = reply;
   EINA_LIST_FOREACH(_servers_cb, l_cb, cb)
     {
        if (cb->add_cb)
          cb->connected_cb(cb->data, server);
     }
   return EINA_TRUE;

}

static Eina_Bool
_ems_client_disconnected(void *data, int type, Ecore_Con_Event_Server_Del *ev)
{
}

static Eina_Bool
_server_disconnected_cb(void *data, Ecore_Con_Reply *reply, Ecore_Con_Server *conn)
{

   Ems_Server *server = data;
   Eina_List *l_cb;
   Ems_Server_Cb *cb;

   DBG("Disconnected from  %s:%d (%s)", server->ip, server->port, server->name);

   server->reply = NULL;

   server->is_connected = EINA_FALSE;
   EINA_LIST_FOREACH(_servers_cb, l_cb, cb)
     {
        if (cb->disconnected_cb)
          cb->disconnected_cb(cb->data, server);
     }
   return EINA_TRUE;

}

static Eina_Bool
_ems_server_connect(Ems_Server *server)
{
   Ecore_Con_Server *conn;

   if (!server)
     return EINA_FALSE;

   INF("try to connect to %s:%d", server->name, server->port);

   if (server->is_connected)
     return EINA_TRUE;

   conn = ecore_con_server_connect(ECORE_CON_REMOTE_TCP, server->ip, server->port, server);
   server->ece = ecore_con_eet_client_new(conn);
   ecore_con_eet_data_set(server->ece, conn);
   ecore_con_eet_server_connect_callback_add(server->ece, _server_connected_cb, server);
   ecore_con_eet_server_disconnect_callback_add(server->ece, _server_disconnected_cb, server);

   return EINA_TRUE;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool ems_server_init(void)
{
   ems_server_eet_init();

   return EINA_TRUE;
}

void ems_server_shutdown(void)
{
   ems_server_eet_shutdown();
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
	    {
	       cb->add_cb(cb->data, server);
	    }
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

   DBG("");

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
   if (!server)
     return NULL;

   return server->name;
}

Eina_Bool
ems_server_is_local(Ems_Server *server)
{
   if (server)
     return server->is_local;
   else
     return EINA_TRUE;
}

Eina_Bool
ems_server_is_connected(Ems_Server *server)
{
   if (server)
     return server->is_connected;
   else
     return EINA_TRUE;
}

Eina_Bool
ems_server_connect(Ems_Server *server)
{
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

   return NULL;
}

Ems_Observer *
ems_server_media_get(Ems_Server *server,
                     Ems_Collection *collection,
                     Ems_Media_Add_Cb media_add,
                     void *data)
{
   Medias_Req *req;

   DBG("");

   req->collection = collection;

   ecore_con_eet_send(server->reply, "media_req", req);

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

   return NULL;
}
