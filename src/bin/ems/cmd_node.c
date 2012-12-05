#include <Ecore.h>
#include <Ems.h>

#include "cmds.h"

#define DETECTION_TIMEOUT 1.0

typedef enum _Print_Value
{
  PRINT_ALL,
  PRINT_IP,
  PRINT_PORT
}Print_Value;

static Ecore_Timer *timer = NULL;
static const char *node_name = NULL;
static Print_Value pval = PRINT_ALL;


static Eina_Bool
_timer_cb(void *data)
{
   timer = NULL;

   ecore_main_loop_quit();

   return EINA_FALSE;
}

static void
_node_add_cb(void *data, Ems_Node *node)
{
   switch(pval)
     {
      case PRINT_ALL : 
         printf("- %-40s[%s:%d]\n", ems_node_name_get(node),
                ems_node_ip_get(node),
                ems_node_port_get(node));
       
         ecore_timer_reset(timer);
         break;
      case PRINT_IP:
         printf("%s\n", ems_node_ip_get(node));
         ecore_main_loop_quit();
         break;
      case PRINT_PORT:
         printf("%d\n", ems_node_port_get(node));
         ecore_main_loop_quit();
         break;
      default:
         break;
     }
}

int
cmd_list(int argc, char **argv)
{
   Ems_Node *node;
   Eina_List *l;
   
   ems_avahi_start();

   timer = ecore_timer_add(DETECTION_TIMEOUT, _timer_cb, NULL); 

   printf("Nodes Detected:\n");

   EINA_LIST_FOREACH(ems_node_list_get(), l, node)
     {
        printf("- %*s[%s:%d]\n", ems_node_name_get(node),
               ems_node_ip_get(node),
               ems_node_port_get(node));
     }


   ems_node_cb_set(_node_add_cb, NULL, NULL,
                     NULL, NULL, NULL);
 
   ems_run();
   ems_shutdown();

   return EINA_TRUE;
}


int
cmd_ip(int argc, char **argv)
{
   const char *cmd;
   Ems_Node *node;
   Eina_List *l;
     
   
   argv++;
   argc--;
   node_name = argv[0];
       
   if (argc < 0)
     return EINA_FALSE;
   
   pval = PRINT_IP;

   ems_avahi_start();

   timer = ecore_timer_add(DETECTION_TIMEOUT, _timer_cb, NULL); 

   EINA_LIST_FOREACH(ems_node_list_get(), l, node)
     {
        if (!strcmp(ems_node_name_get(node), node_name))
          {
             printf("%d\n", ems_node_ip_get(node));
             return EINA_TRUE;
          }        
     }


   ems_node_cb_set(_node_add_cb, NULL, NULL,
                     NULL, NULL, NULL);
 
   ems_run();
   ems_shutdown();

   return EINA_TRUE;
}

int
cmd_port(int argc, char **argv)
{
   const char *cmd;
   Ems_Node *node;
   Eina_List *l;
     
   
   argv++;
   argc--;
   node_name = argv[0];
       
   if (argc < 0)
     return EINA_FALSE;
   
   pval = PRINT_PORT;

   ems_avahi_start();

   timer = ecore_timer_add(DETECTION_TIMEOUT, _timer_cb, NULL); 

   EINA_LIST_FOREACH(ems_node_list_get(), l, node)
     {
        if (!strcmp(ems_node_name_get(node), node_name))
          {
             printf("%d\n", ems_node_port_get(node));
             return EINA_TRUE;
          }        
     }


   ems_node_cb_set(_node_add_cb, NULL, NULL,
                     NULL, NULL, NULL);
 
   ems_run();
   ems_shutdown();

   return EINA_TRUE;
}
