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

#include <Azy.h>

#include "ems_rpc_Config.azy_server.h"
#include "ems_rpc_Browser.azy_server.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static Azy_Server *_ems_server_serv = NULL;

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

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
