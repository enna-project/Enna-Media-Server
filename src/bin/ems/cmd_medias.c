#include <Ecore.h>
#include <Ems.h>

#include "cmds.h"

#define DETECTION_TIMEOUT 1.0

static Ecore_Timer *timer = NULL;
static const char *node_name = NULL;

static Eina_Bool
_timer_cb(void *data)
{
   timer = NULL;

   ecore_main_loop_quit();

   return EINA_FALSE;
}

static void
_media_add_cb(void *data, Ems_Node *node,
                          const char *media)
{
   printf("[%s]\n");
}

static void
_node_add_cb(void *data, Ems_Node *node)
{
   if (!strcmp(ems_node_name_get(node), node_name))
     {
        Ems_Collection *c;

        if (!ems_node_connect(node))
          {
             printf("Error connecting node %s\n", node_name);
             ecore_main_loop_quit();
          }
     }
}


static void
_node_connected_cb(void *data, Ems_Node *node)
{
   if (!strcmp(ems_node_name_get(node), node_name))
     {
        Ems_Collection *c;

        if (!ems_node_connect(node))
          {
             printf("Error connecting node %s\n", node_name);
             ecore_main_loop_quit();
          }

        c = ems_collection_new(EMS_MEDIA_TYPE_VIDEO, "films", "*", NULL);
        ems_node_media_get(node,
                           c,
                           _media_add_cb,
                           NULL);
        printf("TIMER DEL\n");
        ecore_timer_del(timer);
        timer = NULL;
     }

}


int
cmd_medias(int argc, char **argv)
{
   Eina_Bool found = EINA_FALSE;
   Ems_Node *node;
   Eina_List *l;

   //ems_avahi_start();

   argv++;
   argc--;
   node_name = argv[0];

   printf("node name %s\n", node_name);
   EINA_LIST_FOREACH(ems_node_list_get(), l, node)
     {
        if (!strcmp(ems_node_name_get(node), node_name))
          {
             ems_node_media_get(node,
                                NULL,
                                _media_add_cb,
                                NULL);
             printf("Found OK\n");
             found = EINA_TRUE;;
          }
     }

   if (!found)
     {
        ems_node_cb_set(_node_add_cb, NULL, NULL,
                        _node_connected_cb, NULL, NULL);
        timer = ecore_timer_add(DETECTION_TIMEOUT, _timer_cb, NULL);
     }

   ems_run();
   ems_shutdown();

   return EINA_TRUE;
}
