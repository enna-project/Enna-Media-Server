#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.h>
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_File.h>
#include <Eio.h>

#include "ems_private.h"
#include "ems_config.h"
#include "ems_avahi.h"
#include "ems_scanner.h"
#include "ems_server.h"

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

int ems_init(void)
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

   ems_config_init();
   ems_avahi_init();
   ems_server_init();
   ems_scanner_init();
   ems_scanner_start();


   INF("Name : %s", ems_config->name);
   INF("Port : %d", ems_config->port);
   INF("Video Extensions : %s", ems_config->video_extensions);

   EINA_LIST_FOREACH(ems_config->video_directories, l, dir)
     {
        INF("%s: %s", dir->label, dir->path);
     }

   return _ems_init_count;

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

   ems_config_shutdown();
   ems_scanner_shutdown();

   eio_shutdown();
   eet_shutdown();
   eina_shutdown();

   return _ems_init_count;
}

void ems_run(void)
{
    ems_server_run();
}
