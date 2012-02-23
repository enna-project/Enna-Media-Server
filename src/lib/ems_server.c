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
#include <Azy.h>

#include "Ems.h"
#include "ems_private.h"
#include "ems_server.h"

#include "ems_rpc_Config.azy_server.h"
#include "ems_rpc_Browser.azy_server.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

typedef struct _Ems_Server_Cb Ems_Server_Cb;

struct _Ems_Server_Cb
{
   void (*add_cb)(void *data, Ems_Server *server);
   void (*del_cb)(void *data, Ems_Server *server);
   void (*update_cb)(void *data, Ems_Server *server);
   void *data;
};

static Azy_Server *_ems_server_serv = NULL;
static Eina_List *_servers = NULL;
static Eina_List *_servers_cb = NULL;

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool ems_server_init(void)
{
   //Define the list of module used by the server.
   Azy_Server_Module_Def *modules[] = {
     ems_rpc_Config_module_def(),
     ems_rpc_Browser_module_def(),
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
                  void *data)
{
   Ems_Server_Cb *cb;

   cb = calloc(1, sizeof(Ems_Server_Cb));
   if (!cb)
     return;

   cb->add_cb = server_add_cb;
   cb->del_cb = server_del_cb;
   cb->update_cb = server_update_cb;
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
