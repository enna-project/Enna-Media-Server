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

#include <Evas.h>
#include <Elementary.h>
#include <Ems.h>

#include "enna_private.h"
#include "enna_activity.h"
#include "enna_config.h"


/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enna_Activity_Group Enna_Activity_Group;
typedef struct _Enna_Activity_Item Enna_Activity_Item;
typedef struct _Enna_Activity Enna_Activity;

struct _Enna_Activity_Group
{
    Elm_Object_Item *it;
    Ems_Server *server;
    Enna_Activity *act;
};

struct _Enna_Activity_Item
{
   Elm_Object_Item *it;
   Ems_Server *server;
   Enna_Activity *act;
   const char *name;
   const char *description;
   const char *cover;
   const char *fanart;
};

struct _Enna_Activity
{
   Eina_Hash *servers;
   Evas_Object *list;
   Evas_Object *ly;
   Evas_Object *btn_trailer;
   Evas_Object *btn_play;
   Ecore_Timer *show_timer;
};


static Elm_Genlist_Item_Class itc_group;
static Elm_Genlist_Item_Class itc_item;

static char *_genlist_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   Enna_Activity_Group *gr = data;

   return strdup(ems_server_name_get(gr->server));
}

static char *_media_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part)
{
   Enna_Activity_Item *it = data;

   if (!strcmp(part, "name"))
        return strdup(it->name);
   else if (!strcmp(part, "description"))
     return strdup(it->description);
   else
     return NULL;
}

static void _group_del(Enna_Activity_Group *gr)
{
   if (!gr)
     return;

   DBG("Free GROUP");

   free(gr);
}

static void
_server_add_cb(void *data, Ems_Server *server)
{
   Enna_Activity *act = data;

   DBG("Server %s added", ems_server_name_get(server));

   if (!ems_server_connect(server))
     DBG("An error occured during connection with server %s",
         ems_server_name_get(server));
   else
     {
        Enna_Activity_Group *gr;

        gr = calloc(1, sizeof(Enna_Activity_Group));
        gr->server = server;
        gr->act = act;
        eina_hash_add(act->servers, server, gr);
     }

}

static void
_server_del_cb(void *data, Ems_Server *server)
{
   DBG("Server %s deleted", ems_server_name_get(server));
}


static void
_server_update_cb(void *data, Ems_Server *server)
{
   DBG("Server %s updated", ems_server_name_get(server));
}

#define ADD_ITEM(n, d, c, f)                                            \
  it = calloc(1, sizeof (Enna_Activity_Item));                          \
  it->server = server;                                                  \
  it->act = act;                                                        \
  it->name = eina_stringshare_add(n);                                   \
  it->description = eina_stringshare_add(d);                            \
  it->cover = eina_stringshare_printf(c);                               \
  it->fanart = eina_stringshare_printf(f);                              \
  it->it = elm_genlist_item_append(act->list, &itc_item,                \
                                   it,                                  \
                                   gr->it/* parent */,                  \
                                   ELM_GENLIST_ITEM_NONE,               \
                                   NULL,                                \
                                   NULL)

static void
_server_connected_cb(void *data, Ems_Server *server)
{
   int i;
   Enna_Activity_Group *gr;
   Enna_Activity_Item *it;
   Enna_Activity *act = data;

   DBG("Server %s connected", ems_server_name_get(server));

   gr = eina_hash_find(act->servers, server);
   gr->it = elm_genlist_item_append(act->list, &itc_group,
                                    gr,
                                    NULL/* parent */,
                                    ELM_GENLIST_ITEM_GROUP,
                                    NULL,
                                    NULL);
   elm_genlist_item_select_mode_set(gr->it, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

   ADD_ITEM("Crazy stupid love",
            "by Glenn Ficarra, John Requa",
            PACKAGE_DATA_DIR"/images/cover1.jpg",
            PACKAGE_DATA_DIR"/images/fanart1.jpg");

   ADD_ITEM("Iron Man",
            "by John Favreau",
            PACKAGE_DATA_DIR"/images/cover2.jpg",
            PACKAGE_DATA_DIR"/images/fanart2.jpg");

   ADD_ITEM("Star Wars Episode 1",
            "by George Lucas",
            PACKAGE_DATA_DIR"/images/cover3.jpg",
            PACKAGE_DATA_DIR"/images/fanart3.jpg");

   ADD_ITEM("Crazy stupid love",
            "by Glenn Ficarra, John Requa",
            PACKAGE_DATA_DIR"/images/cover1.jpg",
            PACKAGE_DATA_DIR"/images/fanart1.jpg");

   ADD_ITEM("Iron Man",
            "by John Favreau",
            PACKAGE_DATA_DIR"/images/cover2.jpg",
            PACKAGE_DATA_DIR"/images/fanart2.jpg");

   ADD_ITEM("Star Wars Episode 1",
            "by George Lucas",
            PACKAGE_DATA_DIR"/images/cover3.jpg",
            PACKAGE_DATA_DIR"/images/fanart3.jpg");

   ADD_ITEM("Crazy stupid love",
            "by Glenn Ficarra, John Requa",
            PACKAGE_DATA_DIR"/images/cover1.jpg",
            PACKAGE_DATA_DIR"/images/fanart1.jpg");

   ADD_ITEM("Iron Man",
            "by John Favreau",
            PACKAGE_DATA_DIR"/images/cover2.jpg",
            PACKAGE_DATA_DIR"/images/fanart2.jpg");

   ADD_ITEM("Star Wars Episode 1",
            "by George Lucas",
            PACKAGE_DATA_DIR"/images/cover3.jpg",
            PACKAGE_DATA_DIR"/images/fanart3.jpg");

   ADD_ITEM("Crazy stupid love",
            "by Glenn Ficarra, John Requa",
            PACKAGE_DATA_DIR"/images/cover1.jpg",
            PACKAGE_DATA_DIR"/images/fanart1.jpg");

   ADD_ITEM("Iron Man",
            "by John Favreau",
            PACKAGE_DATA_DIR"/images/cover2.jpg",
            PACKAGE_DATA_DIR"/images/fanart2.jpg");

   ADD_ITEM("Star Wars Episode 1",
            "by George Lucas",
            PACKAGE_DATA_DIR"/images/cover3.jpg",
            PACKAGE_DATA_DIR"/images/fanart3.jpg");

   ADD_ITEM("Crazy stupid love",
            "by Glenn Ficarra, John Requa",
            PACKAGE_DATA_DIR"/images/cover1.jpg",
            PACKAGE_DATA_DIR"/images/fanart1.jpg");

   ADD_ITEM("Iron Man",
            "by John Favreau",
            PACKAGE_DATA_DIR"/images/cover2.jpg",
            PACKAGE_DATA_DIR"/images/fanart2.jpg");

   ADD_ITEM("Star Wars Episode 1",
            "by George Lucas",
            PACKAGE_DATA_DIR"/images/cover3.jpg",
            PACKAGE_DATA_DIR"/images/fanart3.jpg");

}

static void
_server_disconnected_cb(void *data, Ems_Server *server)
{
   Enna_Activity_Group *gr;
   Enna_Activity *act = data;

   DBG("Server %s disconnected", ems_server_name_get(server));

   gr = eina_hash_find(act->servers, server);
   if (!gr)
     DBG("gt is null");
   else
     {
        elm_object_item_del(gr->it);
        eina_hash_del(act->servers, server, gr);
     }
}

static Eina_Bool
_timer_cb(void *data)
{
   Enna_Activity_Item *item = data;
   Evas_Object *cover;
   Evas_Object *fanart;

   cover = evas_object_image_filled_add(evas_object_evas_get(item->act->ly));
   evas_object_image_file_set(cover, item->cover, NULL);
   evas_object_show(cover);

   elm_object_part_content_set(item->act->ly, "cover.swallow", cover);

   fanart = evas_object_image_filled_add(evas_object_evas_get(item->act->ly));
   evas_object_image_file_set(fanart, item->fanart, NULL);
   evas_object_show(fanart);

   elm_object_part_content_set(item->act->ly, "fanart.swallow", fanart);
   item->act->show_timer = NULL;
   return EINA_FALSE;
}

static void
_list_item_selected_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Object_Item *it = event_info;
   Enna_Activity_Item *item;

   item = elm_object_item_data_get(it);
   if (item->act->show_timer)
     {
        ecore_timer_del(item->act->show_timer);
     }
   item->act->show_timer = ecore_timer_add(0.3, _timer_cb, item);
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Evas_Object *enna_activity_add(Evas_Object *parent)
{
   Evas_Object *ly;
   Evas_Object *list;
   Ems_Server *server;
   Eina_List *l;
   Enna_Activity *act;

   act = calloc(1, sizeof(Enna_Activity));
   if (!act)
       return NULL;

   ly = elm_layout_add(parent);
   if (!ly)
       goto err1;

   elm_layout_file_set(ly, enna_config_theme_get(), "activity/layout/list");
   evas_object_data_set(ly, "activity", act);

   list = elm_genlist_add(ly);
   if (!list)
       goto err2;

   elm_object_part_content_set(ly, "list.swallow", list);
   elm_genlist_bounce_set(list, EINA_FALSE, EINA_TRUE);
   elm_object_style_set(list, "media");

   itc_group.item_style       = "group_index_media";
   itc_group.func.text_get    = _genlist_text_get;
   itc_group.func.content_get = NULL;
   itc_group.func.state_get   = NULL;
   itc_group.func.del         = NULL;

   itc_item.item_style        = "media";
   itc_item.func.text_get     = _media_text_get;
   itc_item.func.content_get  = NULL;
   itc_item.func.state_get    = NULL;
   itc_item.func.del          = NULL;

   act->list = list;
   act->ly = ly;

   act->servers = eina_hash_pointer_new((Eina_Free_Cb)_group_del);
   ems_server_cb_set(_server_add_cb, _server_del_cb, _server_update_cb,
                     _server_connected_cb, _server_disconnected_cb,
                     act);

   EINA_LIST_FOREACH(ems_server_list_get(), l, server)
   {

      if (!ems_server_connect(server))
        DBG("An error occured during connection with server %s",
            ems_server_name_get(server));
      else
        {
           Enna_Activity_Group *gr;

           gr = calloc(1, sizeof(Enna_Activity_Group));
           gr->server = server;
           gr->act = act;
           eina_hash_add(act->servers, server, gr);
        }
   }

   elm_object_focus_set(list, EINA_TRUE);

   /* Create Trailer and Play buttons */
   act->btn_play = elm_button_add(ly);
   elm_object_text_set(act->btn_play, "Play");
   evas_object_show(act->btn_play);
   elm_object_part_content_set(ly, "play.swallow", act->btn_play);
   elm_object_style_set(act->btn_play, "enna");

   act->btn_trailer = elm_button_add(ly);
   elm_object_text_set(act->btn_trailer, "Trailer");
   evas_object_show(act->btn_trailer);
   elm_object_part_content_set(ly, "trailer.swallow", act->btn_trailer);
   elm_object_style_set(act->btn_trailer, "enna");

   evas_object_smart_callback_add(act->list, "selected", _list_item_selected_cb, act);

   return ly;

err2:
   evas_object_del(ly);
err1:
   free(act);
   return NULL;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
