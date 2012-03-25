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
#include <Ecore_File.h>
#include <Eio.h>

#include "Ems.h"

#include "ems_private.h"
#include "ems_config.h"
#include "ems_avahi.h"
#include "ems_scanner.h"
#include "ems_server.h"
#include "ems_parser.h"
#include "ems_stream_server.h"
#include "ems_downloader.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static int _ems_init_count = 0;

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

int _ems_log_dom_global = -1;
Ems_Config *ems_config = NULL;

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

int ems_init(const char *config_file)
{
   Eina_List *l;
   Ems_Directory *dir;

   if (++_ems_init_count != 1)
     return _ems_init_count;

   if (!eina_init())
     return --_ems_init_count;


   _ems_log_dom_global = eina_log_domain_register("ems", EMS_DEFAULT_LOG_COLOR);
   if (_ems_log_dom_global < 0)
     {
        EINA_LOG_ERR("Enna-Server Can not create a general log domain.");
        goto shutdown_eina;
     }

   if (!eet_init())
     goto unregister_log_domain;

   if (!eio_init())
     goto shutdown_eet;

   DBG("Config Init");

   if (!ems_config_init(config_file))
     goto shutdown_eio;
   if (!ems_avahi_init())
     goto shutdown_config;
   if (!ems_server_init())
     goto shutdown_avahi;
   if (!ems_scanner_init())
     goto shutdown_server;
   if (!ems_parser_init())
     goto shutdown_parser;
   if (!ems_stream_server_init())
     goto shutdown_stream_server;
   if (!ems_downloader_init())
     goto shutdown_downloader;

   INF("Name : %s", ems_config->name);
   INF("Port : %d", ems_config->port);
   INF("Video Extensions : %s", ems_config->video_extensions);

   EINA_LIST_FOREACH(ems_config->video_directories, l, dir)
     {
        INF("%s: %s", dir->label, dir->path);
     }

   return _ems_init_count;

 shutdown_downloader:
 shutdown_stream_server:
   ems_stream_server_shutdown();
 shutdown_parser:
   ems_parser_shutdown();
 shutdown_server:
   ems_server_shutdown();
 shutdown_avahi:
   ems_avahi_shutdown();
 shutdown_config:
   ems_config_shutdown();
 shutdown_eio:
   eio_shutdown();
 shutdown_eet:
   eet_shutdown();
 unregister_log_domain:
   eina_log_domain_unregister(_ems_log_dom_global);
   _ems_log_dom_global = -1;
 shutdown_eina:
   eina_shutdown();
   return --_ems_init_count;
}


int ems_shutdown(void)
{
   if (--_ems_init_count != 0)
     return _ems_init_count;

   DBG("Shutdown");

   ems_scanner_shutdown();
   ems_server_shutdown();
   ems_avahi_shutdown();
   ems_config_shutdown();

   eio_shutdown();
   eet_shutdown();
   eina_log_domain_unregister(_ems_log_dom_global);
   _ems_log_dom_global = -1;
   eina_shutdown();

   return _ems_init_count;
}

void ems_run(void)
{
    ems_server_run();
}
