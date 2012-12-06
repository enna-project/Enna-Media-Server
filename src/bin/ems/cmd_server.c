#include <Ecore.h>

#include <Ems.h>

#include "cmds.h"

static void
_server_add_cb(void *data, Ems_Server *server)
{
   printf("%s [%s:%d]\n", ems_server_name_get(server),
          ems_server_ip_get(server),
          ems_server_port_get(server));
}

int
cmd_servers(int argc, char **argv)
{
   Ems_Server *server;
   Eina_List *l;
   
   printf("Servers :\n");

   EINA_LIST_FOREACH(ems_server_list_get(), l, server)
     {
        printf("%s [%s:%d]\n", ems_server_name_get(server),
               ems_server_ip_get(server),
               ems_server_port_get(server));
     }


   ems_server_cb_set(_server_add_cb, _server_del_cb, NULL, NULL
                     NULL, NULL, NULL);
 
   return EINA_TRUE;
}
