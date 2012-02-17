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
