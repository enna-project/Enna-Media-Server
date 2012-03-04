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
#include <Ecore_Con.h>

#include "Ems.h"
#include "ems_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static Ecore_Con_Server *_server = NULL;

static Ecore_Event_Handler *_handler_add = NULL;
static Ecore_Event_Handler *_handler_del= NULL;
static Ecore_Event_Handler *_handler_data = NULL;

static Eina_Bool
_client_add(void *data, int type, Ecore_Con_Event_Client_Add *ev)
{
   if (ev && (_server != ecore_con_client_server_get(ev->client)))
     return ECORE_CALLBACK_PASS_ON;

   //TODO

   INF("New client from :%s\n", ecore_con_client_ip_get(ev->client));

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_client_del(void *data, int type, Ecore_Con_Event_Client_Del *ev)
{
   if (ev && (_server != ecore_con_client_server_get(ev->client)))
     return ECORE_CALLBACK_PASS_ON;

   //TODO

   INF("Connection from %s closed by remote host\n", ecore_con_client_ip_get(ev->client));

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_client_data(void *data, int type, Ecore_Con_Event_Client_Data *ev)
{
   if (ev && (_server != ecore_con_client_server_get(ev->client)))
     return ECORE_CALLBACK_PASS_ON;

   //TODO

   DBG("Got data from client :%s\n%s\n", ecore_con_client_ip_get(ev->client), ev->data);

   return ECORE_CALLBACK_RENEW;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
ems_stream_server_init(void)
{
   _server = ecore_con_server_add(ECORE_CON_REMOTE_TCP, "0.0.0.0", ems_config->port_stream, NULL);

   _handler_add = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_ADD, 
                           (Ecore_Event_Handler_Cb)_client_add, NULL);
   _handler_del = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DEL,
                           (Ecore_Event_Handler_Cb)_client_del, NULL);
   _handler_data = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA,
                           (Ecore_Event_Handler_Cb)_client_data, NULL);

   INF("Listening to port %d\n", ems_config->port_stream);

   return EINA_TRUE;
}

void
ems_stream_server_shutdown(void)
{
   ecore_con_server_del(_server);

   ecore_event_handler_del(_handler_add);
   ecore_event_handler_del(_handler_del);
   ecore_event_handler_del(_handler_data);
}


