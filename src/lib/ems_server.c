#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Azy.h>

#include "EMS_Config.azy_server.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

void ems_server_init(void)
{
   Azy_Server *serv;
   Azy_Server_Module_Def **mods;

   azy_init();
   //Define the list of module used by the server.
   Azy_Server_Module_Def *modules[] = {
     EMS_Config_module_def(),
     NULL
   };

   serv = azy_server_new(EINA_FALSE);
   azy_server_addr_set(serv, "0.0.0.0");
   azy_server_port_set(serv, 2000);
   for (mods = modules; mods && *mods; mods++)
     {
        if (!azy_server_module_add(serv, *mods))
          ERR("Unable to create server\n");
     }
   INF("Start Azy server");
   azy_server_start(serv);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
