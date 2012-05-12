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
#include <Ecore.h>
#include <Azy.h>

#include "Ems.h"
#include "ems_private.h"
#include "ems_server.h"

#include "ems_rpc_Config.azy_server.h"
#include "ems_rpc_Browser.azy_server.h"
#include "ems_rpc_Medias.azy_server.h"
#include "ems_rpc_Browser.azy_client.h"
#include "ems_rpc_Medias.azy_client.h"

#define CALL_CHECK(X) \
   do \
     { \
        if (!azy_client_call_checker(server->cli, err, ret, X, __PRETTY_FUNCTION__)) \
          { \
             WRN("%s", azy_content_error_message_get(err)); \
             exit(1); \
          } \
     } while (0)

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Ems_Server_Media_Get_Req Ems_Server_Media_Get_Req;
typedef struct _Ems_Server_Media_Info_Get_Req Ems_Server_Media_Info_Get_Req;
typedef struct _Ems_Server_Cb Ems_Server_Cb;

struct _Ems_Server_Media_Get_Req
{
   Ems_Server *server;
   Ems_Collection *collection;
   void (*add_cb)(void *data, Ems_Server *server, const char *media);
   void *data_cb;
};

struct _Ems_Server_Media_Info_Get_Req
{
   Ems_Server *server;
   const char *info;
   void (*add_cb)(void *data, Ems_Server *server, const char *media);
   void *data_cb;
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

static Azy_Server *_ems_server_serv = NULL;
static Eina_List *_servers = NULL;
static Eina_List *_servers_cb = NULL;

static Eina_Bool
_ems_connected_cb(void *data __UNUSED__, int type __UNUSED__, Azy_Client *cli)
{
   Ems_Server *server = azy_client_data_get(cli);
   Eina_List *l_cb;
   Ems_Server_Cb *cb;

   INF("Info connected to %s:%d", azy_client_addr_get(cli), azy_client_port_get(cli));

   if (server->is_connected)
       return EINA_TRUE;
   else
       server->is_connected = EINA_TRUE;

   server->cli = cli;

   EINA_LIST_FOREACH(_servers_cb, l_cb, cb)
     {
        if (cb->add_cb)
          cb->connected_cb(cb->data, server);
     }
   return EINA_TRUE;
}

static Eina_Bool
_ems_disconnected_cb(void *data __UNUSED__, int type __UNUSED__, Azy_Client *cli)
{
   Ems_Server *server = azy_client_data_get(cli);
   Eina_List *l_cb;
   Ems_Server_Cb *cb;

   INF("Info disconnected from %s:%d", azy_client_addr_get(cli), azy_client_port_get(cli));

   server->is_connected = EINA_FALSE;
   server->cli = NULL;

   EINA_LIST_FOREACH(_servers_cb, l_cb, cb)
     {
        if (cb->add_cb)
          cb->disconnected_cb(cb->data, server);
     }
   return EINA_TRUE;
}

static Eina_Bool
_ems_server_connect(Ems_Server *server)
{
   Azy_Client *cli;

   INF("try to connect to %s:%d\n", server->name, server->port);

   if (server->is_connected)
     return EINA_TRUE;

   cli = azy_client_new();
   azy_client_data_set(cli, server);
   if (!azy_client_host_set(cli, server->ip, server->port))
     {
        ERR("Unable to set host %s:%d", server->ip, server->port);
        return EINA_FALSE;
     }

   ecore_event_handler_add(AZY_CLIENT_CONNECTED, (Ecore_Event_Handler_Cb)_ems_connected_cb, server);
   ecore_event_handler_add(AZY_CLIENT_DISCONNECTED, (Ecore_Event_Handler_Cb)_ems_disconnected_cb, server);

   if (!azy_client_connect(cli, EINA_FALSE))
     {
        ERR("Unable to connect to %s:%d", server->ip, server->port);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool ems_server_init(void)
{
   //Define the list of module used by the server.
   Azy_Server_Module_Def *modules[] = {
     ems_rpc_Config_module_def(),
     ems_rpc_Browser_module_def(),
     ems_rpc_Medias_module_def(),
     NULL
   };
   Azy_Server_Module_Def **mods;

   if (!azy_init())
     return EINA_FALSE;

   _ems_server_serv = azy_server_new(EINA_FALSE);
   if (!_ems_server_serv)
     goto shutdown_azy;

   if (!azy_server_addr_set(_ems_server_serv, "0.0.0.0"))
     goto free_server;
   if (!azy_server_port_set(_ems_server_serv, ems_config->port))
     goto free_server;

   for (mods = modules; mods && *mods; mods++)
     {
        if (!azy_server_module_add(_ems_server_serv, *mods))
          ERR("Unable to create server\n");
     }

   return EINA_TRUE;

 free_server:
   azy_server_free(_ems_server_serv);
 shutdown_azy:
   azy_shutdown();

   return EINA_FALSE;
}

void ems_server_shutdown()
{

  /* FIXME: something else to do ? */
   azy_server_free(_ems_server_serv);
   azy_shutdown();
}

void ems_server_run(void)
{
   INF("Start Azy server");
   azy_server_run(_ems_server_serv);
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

static Eina_Error
_ems_server_media_get_ret(Azy_Client *client, Azy_Content *content, void *response)
{
   Eina_List *files = response;
   Eina_List *l;
   const char *f;
   Ems_Server_Media_Get_Req *media_req;

   if (azy_content_error_is_set(content))
     {
        DBG("Error encountered: %s", azy_content_error_message_get(content));
        return azy_content_error_code_get(content);
     }

   media_req = azy_content_data_get(content);

   EINA_LIST_FOREACH(files, l, f)
     {
        if (media_req->add_cb)
          media_req->add_cb(media_req->data_cb, media_req->server, f);
     }
   free(media_req);
   //response is automaticaly free
   return AZY_ERROR_NONE;
}


Ems_Observer *
ems_server_media_get(Ems_Server *server,
                     Ems_Collection *collection,
                     Ems_Media_Add_Cb media_add,
                     void *data)
{
   Azy_Content *err;
   unsigned int ret;
   Ems_Server_Media_Get_Req *media_req;

    if (!server || !server->cli || !collection)
     return NULL;

    if (!server->is_connected)
      _ems_server_connect(server);

   err = azy_content_new(NULL);
   media_req = calloc(1, sizeof(Ems_Server_Media_Get_Req));
   media_req->server = server;
   media_req->collection = collection;
   media_req->add_cb = media_add;
   media_req->data_cb = data;


   ret = ems_rpc_Medias_GetMedias(server->cli, collection->type, collection->filters, err, media_req);
   CALL_CHECK(_ems_server_media_get_ret);
   return NULL;
}

static Eina_Error
_ems_server_media_info_get_ret(Azy_Client *client, Azy_Content *content, void *response)
{
   const char *info = response;
   Ems_Server_Media_Info_Get_Req *media_req;

   if (azy_content_error_is_set(content))
     {
        DBG("Error encountered: %s", azy_content_error_message_get(content));
        return azy_content_error_code_get(content);
     }

   media_req = azy_content_data_get(content);

   if (media_req->add_cb)
     media_req->add_cb(media_req->data_cb, media_req->server, info);

   eina_stringshare_del(media_req->info);
   free(media_req);

   //response is automaticaly free
   return AZY_ERROR_NONE;
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
   Azy_Content *err;
   unsigned int ret;
   Ems_Server_Media_Info_Get_Req *media_req;

    if (!server || !server->cli || !info)
     return NULL;

    if (!server->is_connected)
      _ems_server_connect(server);

   err = azy_content_new(NULL);
   media_req = calloc(1, sizeof(Ems_Server_Media_Info_Get_Req));
   media_req->server = server;
   media_req->info = eina_stringshare_add(info);
   media_req->add_cb = info_add;
   media_req->data_cb = data;

   ret = ems_rpc_Medias_GetMediaInfo(server->cli, uuid+37, info, err, media_req);
   CALL_CHECK(_ems_server_media_info_get_ret);
   return NULL;
}
