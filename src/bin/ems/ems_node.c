#include <Ecore.h>
#include <Ems.h>

#include "cmds.h"

#define DETECTION_TIMEOUT 5.0

static Ecore_Timer *timer = NULL;

static Eina_Bool
_timer_cb(void *data)
{
   timer = NULL;

   ecore_main_loop_quit();

   return EINA_FALSE;
}

static void
_server_add_cb(void *data, Ems_Server *server)
{
   printf("%s [%s:%d]\n", ems_server_name_get(server),
          ems_server_ip_get(server),
          ems_server_port_get(server));

   timer = ecore_timer_reset(timer);
}

int
cmd_nodes(int argc, char **argv)
{
   Ems_Server *node;
   Eina_List *l;
   
   ems_scanner_start();
   ems_avahi_start();

   timer = ecore_timer_add(DETECTION_TIMEOUT, _timer_cb, NULL); 

   printf("Nodes :\n");

   

   EINA_LIST_FOREACH(ems_server_list_get(), l, node)
     {
        printf("%s [%s:%d]\n", ems_server_name_get(node),
               ems_server_ip_get(node),
               ems_server_port_get(node));
     }


   ems_server_cb_set(_server_add_cb, _server_del_cb, NULL, NULL
                     NULL, NULL, NULL);
 
   ems_run();
   ems_shutdown();

   return EINA_TRUE;
}
