#include <Eina.h>
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_File.h>
#include <Eio.h>

#include "ems_private.h"
#include "ems_config.h"

int _ems_log_dom_global = -1;
Ems_Config *ems_config = NULL;


int main(int argc, char **argv)
{
   Eina_List *l;
   Ems_Directory *dir;
   Ems_Extension *ext;
   eina_init();
   eet_init();
   eio_init();

   _ems_log_dom_global = eina_log_domain_register("enna-media-server", EMS_DEFAULT_LOG_COLOR);

   if (_ems_log_dom_global < 0)
     {
        EINA_LOG_ERR("Enna-Server Can not create a general log domain.");
        return -1;
     }

   DBG("Config Init");

   ems_config_init();

   INF("Video Extensions :");
   EINA_LIST_FOREACH(ems_config->video_extensions, l, ext)
     {
        INF("%s", ext->ext);
     }

   INF("Video Directories :");
   EINA_LIST_FOREACH(ems_config->video_directories, l, dir)
     {
        INF("%s: %s", dir->label, dir->path);
     }

   ecore_main_loop_begin();

   DBG("Shutdown");

   ems_config_shutdown();
   eio_shutdown();
   eet_shutdown();
   eina_shutdown();

   return EXIT_SUCCESS;
}
