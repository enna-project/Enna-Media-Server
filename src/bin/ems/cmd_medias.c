#include <Ecore.h>
#include <Ems.h>

#include "cmds.h"

static Ecore_Timer *timer = NULL;
static const char *node_name = NULL;
static const char *sha1 = NULL;
static int nb_medias = 0;

static Eina_Bool
_timer_cb(void *data)
{
   timer = NULL;

   printf("\n----------\n\n%d medias found\n\n", nb_medias);
   ecore_main_loop_quit();

   return EINA_FALSE;
}

static void
_media_add_cb(void *data, Ems_Node *node,
                          Ems_Video *media)
{
   const char *uuid;

   uuid = ems_video_hash_key_get(media);

   printf("[%s] %s\n\t%s\n", uuid,
          ems_video_title_get(media),
          ems_node_media_stream_url_get(node, uuid));
   nb_medias++;
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
        ecore_timer_reset(timer);
     }

}
static void _info_add_cb(void *data, Ems_Node *node,
                            const char *value)
{
   printf("%s\n", value);
}

static void
_node_info_connected_cb(void *data, Ems_Node *node)
{

   if (!strcmp(ems_node_name_get(node), node_name))
     {
        Ems_Collection *c;

        if (!ems_node_connect(node))
          {
             printf("Error connecting node %s\n", node_name);
             ecore_main_loop_quit();
          }
        ems_node_media_info_get(node, sha1, "clean_name",
                                _info_add_cb, NULL, NULL, NULL);
        ecore_timer_reset(timer);
     }

}


int
cmd_medias(int argc, char **argv)
{
   Eina_Bool found = EINA_FALSE;
   Ems_Node *node;
   Eina_List *l;

   argv++;
   argc--;
   node_name = argv[0];

   ems_node_cb_set(_node_add_cb, NULL, NULL,
                   _node_connected_cb, NULL, NULL);
   timer = ecore_timer_add(DETECTION_TIMEOUT, _timer_cb, NULL);

   return EINA_TRUE;
}

int
cmd_media_info(int argc, char **argv)
{
   argv++;
   argc--;
   sha1 = argv[0];

   argv++;
   argc--;
   node_name = argv[0];

   ems_node_cb_set(_node_add_cb, NULL, NULL,
                   _node_info_connected_cb, NULL, NULL);
   timer = ecore_timer_add(DETECTION_TIMEOUT, _timer_cb, NULL);

   return EINA_TRUE;
}
