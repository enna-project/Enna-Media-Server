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

#include <Eina.h>
#include <Evas.h>
#include <Elementary.h>
#include <Ems.h>

#include "enna_private.h"
#include "enna_view_video_list.h"
#include "enna_config.h"
#include "enna_activity.h"
#include "enna_input.h"
#include "enna_view_player_video.h"
#include "enna_keyboard.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define TIMER_VALUE 0.1

typedef struct _Enna_View_News Enna_View_News;
typedef struct _Enna_View_News_Grid_Item Enna_View_News_Grid_Item;

struct _Enna_View_News_Grid_Item
{
   Elm_Object_Item *it;
   Ems_Node *node;
   Eina_Hash *metadata;
   Enna_View_News *act;
   const char *name;
   const char *description;
   const char *cover;
   const char *backdrop;
   const char *media;
   Evas_Object *o_cover;
   Evas_Object *o_backdrop;
};


struct _Enna_View_News
{
   Evas_Object *grid;
   Evas_Object *ly;
   Ecore_Timer *show_timer;
   Enna *enna;
   Enna_Input_Listener *il;
};

static Eina_Bool _timer_cb(void *data);

static Elm_Gengrid_Item_Class itc_item;

char *
_media_text_get(void *data EINA_UNUSED, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   char buf[256];
   snprintf(buf, sizeof(buf), "Photo");
   return strdup(buf);
}


static Evas_Object *_media_content_get(void *data __UNUSED__, Evas_Object *obj, const char *part)
{
    if (!strcmp(part, "elm.swallow.icon"))
    {
        Evas_Object *ic;

        ic = evas_object_image_filled_add(evas_object_evas_get(obj));
        evas_object_image_file_set(ic, PACKAGE_DATA_DIR"/images/cover2.jpg", NULL);
        evas_object_show(ic);

        return ic;
    }
    return NULL;
}

static void
_cover_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Enna_View_News_Grid_Item *item = data;

   item->o_cover = NULL;
}

static void
_backdrop_del(void *data, Evas *e  __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Enna_View_News_Grid_Item *item = data;

   item->o_backdrop = NULL;
}

static Eina_Bool
_timer_cb(void *data)
{
   Enna_View_News_Grid_Item *item = data;

   if (!item || !item->act)
     return EINA_FALSE;

   if (item->o_cover)
     evas_object_del(item->o_cover);
   if (item->o_backdrop)
     evas_object_del(item->o_backdrop);

   if (item->cover)
     {
        item->o_cover = elm_icon_add(item->act->ly);
        elm_image_file_set(item->o_cover, item->cover, NULL);
        elm_image_preload_disabled_set(item->o_cover, EINA_FALSE);
        elm_object_part_content_set(item->act->ly, "cover.swallow", item->o_cover);

        evas_object_event_callback_add(item->o_cover, EVAS_CALLBACK_DEL, _cover_del, item);
     }

   if (item->backdrop)
     {
        item->o_backdrop = elm_icon_add(item->act->ly);
        elm_image_file_set(item->o_backdrop, item->backdrop, NULL);
        elm_image_preload_disabled_set(item->o_backdrop, EINA_FALSE);
        elm_object_part_content_set(item->act->ly, "backdrop.swallow", item->o_backdrop);

        evas_object_event_callback_add(item->o_backdrop, EVAS_CALLBACK_DEL, _backdrop_del, item);
     }

   if (item->act->show_timer)
     item->act->show_timer = NULL;
   return EINA_FALSE;
}

static void
_grid_item_selected_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Object_Item *it = event_info;
   Enna_View_News_Grid_Item *item;

   item = elm_object_item_data_get(it);
   if (!item || !item->act)
     return;

   if (item->act->show_timer)
     {
        ecore_timer_del(item->act->show_timer);
     }
   item->act->show_timer = ecore_timer_add(TIMER_VALUE, _timer_cb, item);

}

static void
_layout_object_del(void *data, Evas *e  __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Enna_View_News *act = data;

   DBG("delete Enna_View_News object (%p)", obj);

   FREE_NULL_FUNC(evas_object_del, act->grid);
   FREE_NULL_FUNC(ecore_timer_del, act->show_timer);
   FREE_NULL_FUNC(enna_input_listener_del, act->il);

   free(act);
}

static void
_grid_select_do(Evas_Object *obj, const char direction)
{
   Elm_Object_Item *it;

   /* Get the current selected item */
   it = elm_gengrid_selected_item_get(obj);
   if (it)
     {
	/* Get the previous or next item */
	if (direction)
	  it = elm_gengrid_item_next_get(it);
	else
	  it = elm_gengrid_item_prev_get(it);
	/* Item is the first one or the last one in the grid */
	if (!it)
	  {
	     /* Try to select the last or the first element */
	     if (direction)
	       it = elm_gengrid_first_item_get(obj);
	     else
	       it = elm_gengrid_last_item_get(obj);
	     if (it)
	       {
		  /* Select this item and show it */
		  elm_gengrid_item_selected_set(it, EINA_TRUE);
		  elm_gengrid_item_bring_in(it, ELM_GENGRID_ITEM_SCROLLTO_MIDDLE);
	       }
	     else
	       {
		  ERR("item can't be selected, the grid is void ?");
	       }
	  }
	/* Select previous or next item and show it */
	else
	  {
	     elm_gengrid_item_selected_set(it, EINA_TRUE);
	     elm_gengrid_item_bring_in(it, ELM_GENGRID_ITEM_SCROLLTO_MIDDLE);
	  }
     }
   /* There is no selected item, select the fist one */
   else
     {
	if (direction)
	  it = elm_gengrid_first_item_get(obj);
	else
	  it = elm_gengrid_last_item_get(obj);
	if (it)
	  {
	     /* Select this item and show it */
	     elm_gengrid_item_selected_set(it, EINA_TRUE);
	     elm_gengrid_item_bring_in(it, ELM_GENGRID_ITEM_SCROLLTO_MIDDLE);
	  }
	else
	  {
	     ERR("item can't be selected, the grid is void ?");
	  }
     }

}

static Eina_Bool
_input_event(void *data, Enna_Input event)
{
   Enna_View_News *act = data;
   const char *tmp;

   tmp = enna_keyboard_input_name_get(event);
   INF("view video grid input event %s", tmp);

   switch(event)
     {
      case ENNA_INPUT_DOWN:
         _grid_select_do(act->grid, 1);
         return ENNA_EVENT_BLOCK;
      case ENNA_INPUT_UP:
         _grid_select_do(act->grid, 0);
         return ENNA_EVENT_BLOCK;
      default:
         break;
     }

   return ENNA_EVENT_CONTINUE;
}

static Eina_Bool _print_hash_cb(const Eina_Hash *hash __UNUSED__, const void *key,
                  void *data, void *fdata __UNUSED__)
{
   Eina_Value *v = data;

   printf("Func data: Hash entry: [%s] -> %s\n", (char*)key, eina_value_to_string(v));

   return 1;
}

static void
_grabber_movie_end_cb(void *data __UNUSED__, const char *filename, Ems_Grabber_Data *grabbed_data)
{
    Eina_Hash *h;
    Eina_List *l;
    printf("End grab movie %s %p\n", filename, grabbed_data->data);
    EINA_LIST_FOREACH(grabbed_data->data, l, h)
    {
        eina_hash_foreach(h, _print_hash_cb, NULL);
    }
}




static void
_grabber_end_cb(void *data __UNUSED__, const char *filename, Ems_Grabber_Data *grabbed_data)
{
   Enna_View_News *act = data;
   Eina_Hash *h;

   DBG("End Grab %s", filename);
   DBG("Grabbed data : %p", grabbed_data);
   INF("Grabber language : %s", grabbed_data->lang);
   INF("Grabber grabbed date : %lu", grabbed_data->date);
   INF("Data:");
   printf("gengrid add %p\n", act->grid);
   EINA_LIST_FREE(grabbed_data->data, h)
   {
       Enna_View_News_Grid_Item *it;
       char *search;
       char *tmp;

       it = calloc(1, sizeof(Enna_View_News_Grid_Item));
       it->metadata = h;


       tmp = eina_value_to_string(eina_hash_find(h, "name"));
       search = ems_utils_decrapify(tmp, NULL, NULL);
       printf("Search : %s\n", search);
       ems_grabber_module_grab(search, search, EMS_MEDIA_TYPE_VIDEO, -1, -1, "tmdb",
                               _grabber_movie_end_cb, it);

       it->it = elm_gengrid_item_append(act->grid, &itc_item, it, NULL, NULL);
   }
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Evas_Object *enna_view_news_add(Enna *enna, Evas_Object *parent)
{
   Evas_Object *ly, *grid;
   Enna_View_News *act;

   act = calloc(1, sizeof(Enna_View_News));
   if (!act)
     return NULL;

   act->il = enna_input_listener_add("enna_view_news", _input_event, act);

   ly = elm_layout_add(parent);
   if (!ly)
       goto err1;
   elm_layout_file_set(ly, enna_config_theme_get(), "activity/layout/news");
   evas_object_data_set(ly, "view_news", act);

   act->enna = enna;

   grid = elm_gengrid_add(ly);
   if (!grid)
       goto err2;

   elm_object_part_content_set(ly, "list.swallow", grid);
   elm_scroller_bounce_set(grid, EINA_TRUE, EINA_FALSE);
   elm_gengrid_item_size_set(grid, 256, 450);

   evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(grid, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_style_set(grid, "shelf");
   evas_object_show(grid);

   itc_item.item_style        = "shelf";
   itc_item.func.text_get     = _media_text_get;
   itc_item.func.content_get  = _media_content_get;
   itc_item.func.state_get    = NULL;
   itc_item.func.del          = NULL;

   act->grid = grid;
   printf("grid : %p\n", act->grid);
   act->ly = ly;

   elm_object_focus_set(grid, EINA_TRUE);

   evas_object_event_callback_add(ly, EVAS_CALLBACK_DEL, _layout_object_del, act);

   ems_grabber_module_grab("top/100", "top/100", EMS_MEDIA_TYPE_VIDEO, -1, -1, "t411",
                           _grabber_end_cb, act);
   evas_object_show(ly);

   return ly;
err2:
   evas_object_del(ly);
 err1:
   free(act);
   return NULL;
}
