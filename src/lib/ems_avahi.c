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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

#include "ems_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static AvahiEntryGroup *group = NULL;
static AvahiSimplePoll *simple_poll = NULL;
static char *name = NULL;
static AvahiClient *client = NULL;

static void create_services(AvahiClient *c);

static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata)
{
   assert(g == group || group == NULL);
   group = g;

   /* Called whenever the entry group state changes */

   switch (state)
     {
      case AVAHI_ENTRY_GROUP_ESTABLISHED :
         /* The entry group has been established successfully */
         INF("Service '%s' successfully established.", name);
         break;

      case AVAHI_ENTRY_GROUP_COLLISION :
        {
           char *n;

           /* A service name collision with a remote service
            * happened. Let's pick a new name */
           n = avahi_alternative_service_name(name);
           avahi_free(name);
           name = n;

           WRN("Service name collision, renaming service to '%s'", name);

           /* And recreate the services */
           create_services(avahi_entry_group_get_client(g));
           break;
        }

      case AVAHI_ENTRY_GROUP_FAILURE :

         ERR("Entry group failure: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

         /* Some kind of failure happened while we were registering our services */
         avahi_simple_poll_quit(simple_poll);
         break;

      case AVAHI_ENTRY_GROUP_UNCOMMITED:
      case AVAHI_ENTRY_GROUP_REGISTERING:
         ;
     }
}

static void create_services(AvahiClient *c)
{
   char *n, r[128];
   int ret;
   assert(c);

   /* If this is the first time we're called, let's create a new
    * entry group if necessary */

   if (!group)
     if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL)))
       {
          ERR("avahi_entry_group_new() failed: %s", avahi_strerror(avahi_client_errno(c)));
          goto fail;
       }

   /* If the group is empty (either because it was just created, or
    * because it was reset previously, add our entries.  */

   if (avahi_entry_group_is_empty(group))
     {
        INF("Adding service '%s' on port %d", name, ems_config->port);

        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name, "_enna-jsonrpc._tcp", NULL, NULL, ems_config->port, "name=", ems_config->name, NULL)) < 0) /* TODO : port needs to come from config */
          {

             if (ret == AVAHI_ERR_COLLISION)
               goto collision;

             ERR("Failed to add _ipp._tcp service: %s", avahi_strerror(ret));
             goto fail;
          }

        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(group)) < 0)
          {
             ERR("Failed to commit entry group: %s", avahi_strerror(ret));
             goto fail;
          }
     }

   return;

 collision:

   /* A service name collision with a local service happened. Let's
    * pick a new name */
   n = avahi_alternative_service_name(name);
   avahi_free(name);
   name = n;

   ERR("Service name collision, renaming service to '%s'", name);

   avahi_entry_group_reset(group);

   create_services(c);
   return;

 fail:
   avahi_simple_poll_quit(simple_poll);
}

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata)
{
   assert(c);

   /* Called whenever the client or server state changes */

   switch (state)
     {
      case AVAHI_CLIENT_S_RUNNING:

         /* The server has startup successfully and registered its host
          * name on the network, so it's time to create our services */
         create_services(c);
         break;

      case AVAHI_CLIENT_FAILURE:

         ERR("Client failure: %s", avahi_strerror(avahi_client_errno(c)));
         avahi_simple_poll_quit(simple_poll);

         break;

      case AVAHI_CLIENT_S_COLLISION:

         /* Let's drop our registered services. When the server is back
          * in AVAHI_SERVER_RUNNING state we will register them
          * again with the new host name. */

      case AVAHI_CLIENT_S_REGISTERING:

         /* The server records are now being established. This
          * might be caused by a host name change. We need to wait
          * for our own records to register until the host name is
          * properly esatblished. */

         if (group)
           avahi_entry_group_reset(group);

         break;

      case AVAHI_CLIENT_CONNECTING:
         ;
     }
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool ems_avahi_init(void)
{
   int error;

   /* Allocate main loop object */
   if (!(simple_poll = avahi_simple_poll_new()))
     {
        ERR("Failed to create simple poll object.");
        goto fail;
     }

   name = avahi_strdup("EnnaMediaServer");

   /* Allocate a new client */
   client = avahi_client_new(avahi_simple_poll_get(simple_poll), 0, client_callback, NULL, &error);

   /* Check wether creating the client object succeeded */
   if (!client)
     {
        ERR("Failed to create client: %s", avahi_strerror(error));
        goto fail;
     }

   return EINA_TRUE;

 fail:

   /* Cleanup things */

     if (client)
       avahi_client_free(client);

     if (simple_poll)
       avahi_simple_poll_free(simple_poll);

     avahi_free(name);

   return EINA_FALSE;
}

void ems_avahi_shutdown(void)
{
   if (client)
     avahi_client_free(client);

   if (simple_poll)
     avahi_simple_poll_free(simple_poll);

   avahi_free(name);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
