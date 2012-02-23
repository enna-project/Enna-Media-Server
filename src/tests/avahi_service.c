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
#include <avahi-client/lookup.h>
#include <avahi-client/publish.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/alternative.h>
#include <avahi-common/timeval.h>

#include <avahi-glib/glib-watch.h>
#include <avahi-glib/glib-malloc.h>

#include <Ecore.h>

#define EMS_SERVER_JSONRPC_API_NAME "_enna_server-jsonrpc._tcp"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static AvahiEntryGroup *group = NULL;
static AvahiClient *client = NULL;
static char *server_name = NULL;
static AvahiGLibPoll *glib_poll = NULL;
static int port = 0;

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
         printf("Service '%s' successfully established.\n", server_name);
         break;

      case AVAHI_ENTRY_GROUP_COLLISION :
        {
           char *n;

           /* A service name collision with a remote service
            * happened. Let's pick a new name */
           n = avahi_alternative_service_name(server_name);
           avahi_free(server_name);
           server_name = n;

           printf("Service name collision, renaming service to '%s'\n", server_name);

           /* And recreate the services */
           create_services(avahi_entry_group_get_client(g));
           break;
        }

      case AVAHI_ENTRY_GROUP_FAILURE :

         printf("Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

         /* Some kind of failure happened while we were registering our services */
         break;

      case AVAHI_ENTRY_GROUP_UNCOMMITED:
      case AVAHI_ENTRY_GROUP_REGISTERING:
         ;
     }
}

static void create_services(AvahiClient *c)
{
   char *n;
   int ret;
   assert(c);

   /* If this is the first time we're called, let's create a new
    * entry group if necessary */

   if (!group)
     if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL)))
       {
          printf("avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(c)));
          goto fail;
       }

   /* If the group is empty (either because it was just created, or
    * because it was reset previously, add our entries.  */

   if (avahi_entry_group_is_empty(group))
     {
        const char *name;

        printf("Adding service '%s' on port %d\n", server_name, port);

        name = eina_stringshare_printf("name=%s", "ThisIsATest");

        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, server_name, EMS_SERVER_JSONRPC_API_NAME, NULL, NULL, port, name, NULL)) < 0) /* TODO : port needs to come from config */
          {

             if (ret == AVAHI_ERR_COLLISION)
               goto collision;

             printf("Failed to add _ipp._tcp service: %s\n", avahi_strerror(ret));
             goto fail;
          }

        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(group)) < 0)
          {
             printf("Failed to commit entry group: %s\n", avahi_strerror(ret));
             goto fail;
          }
     }

   return;

 collision:

   /* A service name collision with a local service happened. Let's
    * pick a new name */
   n = avahi_alternative_service_name(server_name);
   avahi_free(server_name);
   server_name = n;

   printf("Service name collision, renaming service to '%s'\n", server_name);

   avahi_entry_group_reset(group);

   create_services(c);
   return;

 fail:
   return;
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
         printf("Client failure: %s\n", avahi_strerror(avahi_client_errno(c)));
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

int main(int argc, char **argv)
{
   int error;
   const AvahiPoll *poll_api;
   AvahiServiceBrowser *sb = NULL;

   if (argc < 3)
     {
        printf("Usage : %s name port\n", argv[0]);
        return EXIT_FAILURE;
     }

   eina_init();
   ecore_init();

   /* We are using avahi_glib here so we need to integrate glib mainloop into ecore */
   ecore_main_loop_glib_integrate();

   glib_poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);
   poll_api = avahi_glib_poll_get (glib_poll);


   /* Allocate main loop object */
   if (!glib_poll)
     {
        printf("Failed to create glib poll object.\n");
        goto fail;
     }

   server_name = avahi_strdup(argv[1]);
   port = atoi(argv[2]);

   /* Allocate a new client */
   client = avahi_client_new(poll_api, 0, client_callback, NULL, &error);

   /* Check wether creating the client object succeeded */
   if (!client)
     {
        printf("Failed to create client: %s\n", avahi_strerror(error));
        goto fail;
     }


   ecore_main_loop_begin();
   
   ecore_shutdown();
   eina_shutdown();
   
   return EXIT_SUCCESS;

 fail:

   /* Cleanup things */
   if (sb)
     avahi_service_browser_free(sb);

   if (client)
     avahi_client_free(client);

   if (glib_poll)
     avahi_glib_poll_free(glib_poll);

   avahi_free(server_name);

   return EXIT_FAILURE;
}
