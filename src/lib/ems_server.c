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
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Ems_Server_Cb Ems_Server_Cb;

struct _Ems_Server_Cb
{
   void (*add_cb)(void *data, Ems_Server *server);
   void (*del_cb)(void *data, Ems_Server *server);
   void (*update_cb)(void *data, Ems_Server *server);
   void (*connected_cb)(void *data, Ems_Server *server);
   void (*disconnected_cb)(void *data, Ems_Server *server);
   void *data;
};

typedef struct _Ems_Server_Data_Media_Get Ems_Server_Data_Media_Get;

struct _Ems_Server_Data_Media_Get
{
   int one;
   const char *str;
};


static Ecore_Con_Server *conn = NULL;
static Eet_Connection *econn = NULL;
static Eet_Data_Descriptor *edd = NULL;

static Eina_List *_servers = NULL;
static Eina_List *_servers_cb = NULL;

static Eina_Bool
_ems_client_connected(void *data, int type, Ecore_Con_Event_Server_Add *ev)
{
   Ems_Server *server;
   Eina_List *l_cb;
   Ems_Server_Cb *cb;

   server = ecore_con_server_data_get(ev->server);
   DBG("Connected to %s:%d (%s)", server->ip, server->port, server->name);

   if (server->is_connected)
       return EINA_TRUE;
   else
       server->is_connected = EINA_TRUE;

   EINA_LIST_FOREACH(_servers_cb, l_cb, cb)
     {
        if (cb->add_cb)
          cb->connected_cb(cb->data, server);
     }
   return EINA_TRUE;

   return EINA_TRUE;
}

static Eina_Bool
_ems_client_disconnected(void *data, int type, Ecore_Con_Event_Server_Del *ev)
{
   Ems_Server *server;
   Eina_List *l_cb;
   Ems_Server_Cb *cb;

   server = ecore_con_server_data_get(ev->server);

   DBG("Disconnected from  %s:%d (%s)", server->ip, server->port, server->name);

   server->is_connected = EINA_FALSE;
   EINA_LIST_FOREACH(_servers_cb, l_cb, cb)
     {
        if (cb->add_cb)
          cb->disconnected_cb(cb->data, server);
     }
   return EINA_TRUE;
}

static Eina_Bool
_ems_client_data(void *data, int type, Ecore_Con_Event_Server_Data *ev)
{
   Ems_Server *server;

   server = ecore_con_server_data_get(ev->server);
   DBG("Received data from %s",server->ip );
   eet_connection_received(server->eet_conn, ev->data, ev->size);

   return EINA_TRUE;
}


static Eina_Bool
_ems_server_disconnected(void *data, int type, Ecore_Con_Event_Client_Del *ev)
{
   DBG("Disconnected %s", ecore_con_client_ip_get(ev->client));

   return EINA_TRUE;
}

static Eina_Bool
_ems_server_data(void *data, int type, Ecore_Con_Event_Client_Data *ev)
{
   DBG("");

   eet_connection_received(econn, ev->data, ev->size);

   return EINA_TRUE;
}

static Eina_Bool
_ems_server_read_cb(const void *eet_data, size_t size, void *user_data)
{

   int i;
   Ems_Server_Protocol *prot = eet_data_descriptor_decode(edd, eet_data, size);

   for (i = 0; match_type[i].name != NULL; i++)
     {
        if (match_type[i].type == prot->type)
          {
             DBG("Received %s [%p]", match_type[i].name, &prot->type);
             if (prot->type == EMS_SERVER_PROTOCOL_TYPE_GET_MEDIAS_REQ)
               {
                  Eina_List *files = ems_database_collection_get(ems_config->db, NULL);
                  Ems_Server_Protocol *prot;

                  prot = calloc(1, sizeof (Ems_Server_Protocol));
                  prot->type = EMS_SERVER_PROTOCOL_TYPE_GET_MEDIAS;
                  prot->data.get_medias.files = files;

                  eet_connection_send(econn, edd, prot, NULL);
               }
             break;
          }
     }
   return EINA_TRUE;
}

static Eina_Bool
_ems_server_write_cb(const void *data, size_t size, void *user_data)
{
   DBG("");

   if (ecore_con_client_send(user_data, data, size) != (int) size)
     {
        ERR("Error sending data (%d)", size);
        return EINA_FALSE;
     }

   DBG("Sent");
   return EINA_TRUE;
}

static Eina_Bool
_ems_server_connected(void *data, int type, Ecore_Con_Event_Client_Add *ev)
{
   DBG("New connection from %s", ecore_con_client_ip_get(ev->client));

   econn = eet_connection_new(_ems_server_read_cb, _ems_server_write_cb, ev->client);

   return EINA_TRUE;
}

static Eina_Bool
_ems_client_read_cb(const void *eet_data, size_t size, void *user_data)
{

   int i;
   Ems_Server_Protocol *prot = eet_data_descriptor_decode(edd, eet_data, size);
   DBG("");
   for (i = 0; match_type[i].name != NULL; i++)
     {
        if (match_type[i].type == prot->type)
          {
             DBG("Received %s [%p]", match_type[i].name, &prot->type);
             if (prot->type == EMS_SERVER_PROTOCOL_TYPE_GET_MEDIAS)
               {
                  Eina_List *files = prot->data.get_medias.files;
                  Eina_List *l;
                  const char *f;
                  EINA_LIST_FOREACH(files, l, f)
                    DBG("%s",f);
               }
             break;
          }
     }
   return EINA_TRUE;

}

static Eina_Bool
_ems_client_write_cb(const void *data, size_t size, void *user_data)
{
   Ems_Server *server = user_data;

   DBG("Data to send of size %d", size);

   if (ecore_con_server_send(server->ecore_conn, data, size) != (int) size)
     return EINA_FALSE;

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

   server->eet_conn = eet_connection_new(_ems_client_read_cb, _ems_client_write_cb, server);

   return EINA_TRUE;
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

   ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_ADD, (Ecore_Event_Handler_Cb)_ems_server_connected, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DEL, (Ecore_Event_Handler_Cb)_ems_server_disconnected, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA, (Ecore_Event_Handler_Cb)_ems_server_data, NULL);

   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, (Ecore_Event_Handler_Cb)_ems_client_connected, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)_ems_client_disconnected, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)_ems_client_data, NULL);

   /* Add Ems instance server */
   conn = ecore_con_server_add(ECORE_CON_REMOTE_TCP, "0.0.0.0", ems_config->port, NULL);
   if (!conn)
     return EINA_FALSE;

   edd = ems_server_protocol_init();

   return EINA_TRUE;
}

void ems_server_shutdown()
{
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
   Ems_Server_Protocol *prot;

   DBG("");

   prot = calloc(1, sizeof (Ems_Server_Protocol));
   prot->type = EMS_SERVER_PROTOCOL_TYPE_GET_MEDIAS_REQ;
   prot->data.get_medias_req.collection = collection;

   eet_connection_send(server->eet_conn, edd, prot, NULL);
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
