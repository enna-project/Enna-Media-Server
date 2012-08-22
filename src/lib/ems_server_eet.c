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
#include "Ecore_Con_Eet.h"
#include "ems_server_protocol.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/


static Ecore_Con_Eet *ece = NULL;

static Eina_Bool
_client_connected_cb(void *data, Ecore_Con_Reply *reply, Ecore_Con_Client *conn)
{
   DBG("New client connected %s", ecore_con_client_ip_get(conn));

   return EINA_TRUE;
}

static Eina_Bool
_client_disconnected_cb(void *data, Ecore_Con_Reply *reply, Ecore_Con_Client *conn)
{
   DBG("Client disconnected %s", ecore_con_client_ip_get(conn));

   return EINA_TRUE;
}

Eina_Bool
ems_server_eet_init(void)
{
   Ecore_Con_Server *conn = NULL;

   eina_init();
   eet_init();
   ecore_init();
   ecore_con_init();

   /* Add Ems instance server */
   conn = ecore_con_server_add(ECORE_CON_REMOTE_TCP, "0.0.0.0", ems_config->port, NULL);
   if (!conn)
     return EINA_FALSE;

   INF("Ems server listening to port %d\n", ems_config->port);

   ece = ecore_con_eet_server_new(conn);
   ecore_con_eet_data_set(ece, conn);

   ecore_con_eet_client_connect_callback_add(ece, _client_connected_cb, NULL);
   //   ecore_con_eet_client_disconnect_callback_add(ece, _client_disconnected_cb, NULL);

   ems_server_protocol_init();

   ecore_con_eet_register(ece, "medias_req", ems_server_protocol_edd_get(EMS_SERVER_PROTOCOL_TYPE_MEDIAS_REQ));
   ecore_con_eet_register(ece, "medias", ems_server_protocol_edd_get(EMS_SERVER_PROTOCOL_TYPE_MEDIAS));
   ecore_con_eet_register(ece, "media_info_req", ems_server_protocol_edd_get(EMS_SERVER_PROTOCOL_TYPE_MEDIA_INFOS_REQ));
   ecore_con_eet_register(ece, "media_info", ems_server_protocol_edd_get(EMS_SERVER_PROTOCOL_TYPE_MEDIA_INFOS));

   return EINA_TRUE;
}

void ems_server_eet_shutdown(void)
{
   ecore_con_eet_client_connect_callback_del(ece, _client_connected_cb, NULL);
   ecore_con_eet_client_disconnect_callback_del(ece, _client_disconnected_cb, NULL);
   if (ece) ecore_con_eet_server_free(ece);
   ece = NULL;
}
