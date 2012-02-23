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

#include "Ems.h"
#include "ems_private.h"
#include "ems_server.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static AvahiEntryGroup *group = NULL;
static AvahiClient *client = NULL;
char *server_name = NULL;
static AvahiGLibPoll *glib_poll = NULL;

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
         DBG("Service '%s' successfully established.", server_name);
         break;

      case AVAHI_ENTRY_GROUP_COLLISION :
        {
           char *n;

           /* A service name collision with a remote service
            * happened. Let's pick a new name */
           n = avahi_alternative_service_name(server_name);
           avahi_free(server_name);
           server_name = n;

           WRN("Service name collision, renaming service to '%s'", server_name);

           /* And recreate the services */
           create_services(avahi_entry_group_get_client(g));
           break;
        }

      case AVAHI_ENTRY_GROUP_FAILURE :

         ERR("Entry group failure: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

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
          ERR("avahi_entry_group_new() failed: %s", avahi_strerror(avahi_client_errno(c)));
          goto fail;
       }

   /* If the group is empty (either because it was just created, or
    * because it was reset previously, add our entries.  */

   if (avahi_entry_group_is_empty(group))
     {
        const char *name;

        DBG("Adding service '%s' on port %d", server_name, ems_config->port);

        name = eina_stringshare_printf("name=%s", ems_config->name);

        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, server_name, EMS_SERVER_JSONRPC_API_NAME, NULL, NULL, ems_config->port, name, NULL)) < 0) /* TODO : port needs to come from config */
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
   n = avahi_alternative_service_name(server_name);
   avahi_free(server_name);
   server_name = n;

   ERR("Service name collision, renaming service to '%s'", server_name);

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
         ERR("Client failure: %s", avahi_strerror(avahi_client_errno(c)));
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

static void resolve_callback(
                             AvahiServiceResolver *r,
                             AVAHI_GCC_UNUSED AvahiIfIndex interface,
                             AVAHI_GCC_UNUSED AvahiProtocol protocol,
                             AvahiResolverEvent event,
                             const char *name,
                             const char *type,
                             const char *domain,
                             const char *host_name,
                             const AvahiAddress *address,
                             uint16_t port,
                             AvahiStringList *txt,
                             AvahiLookupResultFlags flags,
                             AVAHI_GCC_UNUSED void* userdata
                             )
{

   assert(r);

   /* Called whenever a service has been resolved successfully or timed out */

   switch (event)
     {
      case AVAHI_RESOLVER_FAILURE:
         ERR("(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
         break;

      case AVAHI_RESOLVER_FOUND:
        {
           Ems_Server *server;
           char a[AVAHI_ADDRESS_STR_MAX], *t;

           DBG("Service '%s' of type '%s' in domain '%s':", name, type, domain);

           avahi_address_snprint(a, sizeof(a), address);
           t = avahi_string_list_to_string(txt);
           DBG("\t%s:%u (%s)\n"
               "\tTXT=%s\n"
               "\tcookie is %u\n"
               "\tis_local: %i\n"
               "\tour_own: %i\n"
               "\twide_area: %i\n"
               "\tmulticast: %i\n"
               "\tcached: %i",
               host_name, port, a,
               t,
               avahi_string_list_get_service_cookie(txt),
               !!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
               !!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN),
               !!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
               !!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
               !!(flags & AVAHI_LOOKUP_RESULT_CACHED));

           /* TODO: only ipv4 address for now, let other people with ipv6 knowledge play with it */
           if (address->proto == AVAHI_PROTO_INET)
             {
                server = calloc(1, sizeof(Ems_Server));
                server->name = eina_stringshare_add(name);
                server->ip = eina_stringshare_add(a);
                server->port = port;
                server->is_local = !!(flags & AVAHI_LOOKUP_RESULT_LOCAL);
                ems_server_add(server);
             }

           avahi_free(t);
           break;
        }
      default:
         DBG("Unknown case");
         break;
     }

   avahi_service_resolver_free(r);
}


static void browse_callback(
                            AvahiServiceBrowser *b,
                            AvahiIfIndex interface,
                            AvahiProtocol protocol,
                            AvahiBrowserEvent event,
                            const char *name,
                            const char *type,
                            const char *domain,
                            AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
                            void* userdata)
{

   AvahiClient *c = userdata;;

   assert(b);

   /* Called whenever a new services becomes available on the LAN or is removed from the LAN */

   switch (event)
     {
      case AVAHI_BROWSER_FAILURE:

         ERR("(Browser) %s", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
         return;

      case AVAHI_BROWSER_NEW:
         DBG("(Browser) NEW: service '%s' of type '%s' in domain '%s'", name, type, domain);
         /* We ignore the returned resolver object. In the callback
            function we free it. If the server is terminated before
            the callback function is called the server will free
            the resolver for us. */

         if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, NULL)))
           ERR("Failed to resolve service '%s': %s", name, avahi_strerror(avahi_client_errno(c)));

         break;

      case AVAHI_BROWSER_REMOVE:
         DBG("(Browser) REMOVE: service '%s' of type '%s' in domain '%s'", name, type, domain);
         ems_server_del(name);
         break;

      case AVAHI_BROWSER_ALL_FOR_NOW:
      case AVAHI_BROWSER_CACHE_EXHAUSTED:
         DBG("(Browser) %s", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
         break;
     }
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool ems_avahi_init(void)
{
   int error;
   const AvahiPoll *poll_api;
   AvahiServiceBrowser *sb = NULL;

   /* We are using avahi_glib here so we need to integrate glib mainloop into ecore */
   ecore_main_loop_glib_integrate();

   glib_poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);
   poll_api = avahi_glib_poll_get (glib_poll);


   /* Allocate main loop object */
   if (!glib_poll)
     {
        ERR("Failed to create glib poll object.");
        goto fail;
     }

   server_name = avahi_strdup(ems_config->name);

   /* Allocate a new client */
   client = avahi_client_new(poll_api, 0, client_callback, NULL, &error);

   /* Check wether creating the client object succeeded */
   if (!client)
     {
        ERR("Failed to create client: %s", avahi_strerror(error));
        goto fail;
     }
   /* Create the service browser */
   if (
       !(sb = avahi_service_browser_new(client,
                                        AVAHI_IF_UNSPEC,
                                        AVAHI_PROTO_UNSPEC,
                                        EMS_SERVER_JSONRPC_API_NAME,
                                        NULL, 0, browse_callback, client))
       )
     {
        ERR("Failed to create service browser: %s",
            avahi_strerror(avahi_client_errno(client)));
        goto fail;
     }
   return EINA_TRUE;

 fail:

   /* Cleanup things */
   if (sb)
     avahi_service_browser_free(sb);

   if (client)
     avahi_client_free(client);

   if (glib_poll)
     avahi_glib_poll_free(glib_poll);

   avahi_free(server_name);

   return EINA_FALSE;
}

void ems_avahi_shutdown(void)
{
   if (client)
     avahi_client_free(client);

   if (glib_poll)
     avahi_glib_poll_free(glib_poll);

   avahi_free(server_name);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
