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

#include <Elementary.h>

#include "enna_private.h"
#include "enna_config.h"
#include "enna_mainmenu.h"
#include "enna_input.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

typedef struct _Enna_Mainmenu Enna_Mainmenu;
typedef enum _Enna_Menu_Selected Enna_Menu_Selected;

enum _Enna_Menu_Selected
{
  ENNA_MENU_LIST,
  ENNA_MENU_SHELF
};


struct _Enna_Mainmenu
{
   Evas_Object *ly;
   Evas_Object *list;
   Evas_Object *shelf;
   Enna_Input_Listener *il;
   Enna_Menu_Selected selected;
};


static void
_list_item_activated_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
    Elm_Object_Item *it = event_info;
    const char *label;

    label = elm_object_item_text_get(it);
    DBG("Item activated : %s", label);

    evas_object_smart_callback_call(data, "selected", label);
}

static void
_shelf_item_activated_cb(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Object *ic;
   Evas_Object *ly = data;
   char tmp[4096];

   snprintf(tmp, sizeof(tmp), PACKAGE_DATA_DIR"/images/fanart%d.jpg", rand() % 3 + 1);

   ic = evas_object_image_filled_add(evas_object_evas_get(obj));
   evas_object_image_file_set(ic, tmp, NULL);
   evas_object_show(ic);

   elm_layout_content_set(ly, "fanart.swallow", ic);
}

static void
_layout_object_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Enna_Mainmenu *mm = data;

   DBG("delete Enna_Mainmenu object (%p)", obj);

   FREE_NULL_FUNC(evas_object_del, mm->list);
   FREE_NULL_FUNC(evas_object_del, mm->shelf);
   FREE_NULL_FUNC(enna_input_listener_del, mm->il);

   free(mm);
}

static void
_lists_select_do(Enna_Mainmenu *mm, Enna_Input ei)
{
   Elm_Object_Item *it;
   Enna_Menu_Selected state;
   Evas_Object *obj, *prev_obj;
   unsigned char direction;

   switch (ei)
     {
      case ENNA_INPUT_RIGHT:
	 state = ENNA_MENU_SHELF;
	 obj = mm->shelf;
	 prev_obj = mm->list;
	 direction = 1;
	 break;
      case ENNA_INPUT_LEFT:
	 state = ENNA_MENU_SHELF;
	 obj = mm->shelf;
	 prev_obj = mm->list;
	 direction = 0;
	 break;
      case ENNA_INPUT_UP:
	 state = ENNA_MENU_LIST;
	 obj = mm->list;
	 prev_obj = mm->shelf;
	 direction = 0;
	 break;
      case ENNA_INPUT_DOWN:
	 state = ENNA_MENU_LIST;
	 obj = mm->list;
	 prev_obj = mm->shelf;
	 direction = 1;
	 break;
      default:
	 break;
     }

   /* Shelf is not selected right now */
   if (mm->selected != state)
     {
	/* Unselect item in the list */
	it = elm_list_selected_item_get(prev_obj);
	if (it)
	  elm_list_item_selected_set(it, EINA_FALSE);
	mm->selected = state;
     }


   /* Get the current selected item */
   it = elm_list_selected_item_get(obj);
   if (it)
     {
	/* Get the previous or next item */
	if (direction)
	  it = elm_list_item_next(it);
	else
	  it = elm_list_item_prev(it);
	/* Item is the first one or the last one in the list */
	if (!it)
	  {
	     /* Try to select the last or the first element */
	     if (direction)
	       it = elm_list_first_item_get(obj);
	     else
	       it = elm_list_last_item_get(obj);
	     if (it)
	       {
		  /* Select this item and show it */
		  elm_list_item_selected_set(it, EINA_TRUE);
		  elm_list_item_bring_in(it);
	       }
	     else
	       {
		  ERR("item can't be selected, the list is void ?");
	       }
	  }
	/* Select previous or next item and show it */
	else
	  {
	     elm_list_item_selected_set(it, EINA_TRUE);
	     elm_list_item_bring_in(it);
	  }
     }
   /* There is no selected item, select the fist one */
   else
     {
	if (direction)
	  it = elm_list_first_item_get(obj);
	else
	  it = elm_list_last_item_get(obj);
	if (it)
	  {
	     /* Select this item and show it */
	     elm_list_item_selected_set(it, EINA_TRUE);
	     elm_list_item_bring_in(it);
	  }
	else
	  {
	     ERR("item can't be selected, the list is void ?");
	  }
     }

}

static Eina_Bool
_input_event(void *data, Enna_Input event)
{
   const char *tmp;
   Elm_Object_Item *it;
   Enna_Mainmenu *mm = data;

   tmp = enna_keyboard_input_name_get(event);
   INF("Mainmenu input event %s", tmp);

   switch(event)
     {
      case ENNA_INPUT_DOWN:
      case ENNA_INPUT_UP:
      case ENNA_INPUT_RIGHT:
      case ENNA_INPUT_LEFT:
	 _lists_select_do(mm, event);
	 break;
      case ENNA_INPUT_OK:
        {
           const char *label;
           Elm_Object_Item *it;
           if(mm->selected == ENNA_MENU_LIST)
             it = elm_list_selected_item_get(mm->list);
           else
             it = elm_list_selected_item_get(mm->shelf);

           if (it)
             {
                label = elm_object_item_text_get(it);
                DBG("Item activated : %s", label);
                evas_object_smart_callback_call(mm->ly, "selected", label);
             }
        }
         break;
       default:
         break;
     }

}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Evas_Object *
enna_mainmenu_add(Enna *enna __UNUSED__, Evas_Object *parent)
{
    Elm_Object_Item *it;
    Enna_Mainmenu *mm;
    Evas_Object *ic;

    mm = calloc(1, sizeof(Enna_Mainmenu));
    if (!mm)
      return NULL;

    mm->il = enna_input_listener_add("enna_mainmenu", _input_event, mm);

    mm->ly = elm_layout_add(parent);
    elm_layout_file_set(mm->ly, enna_config_theme_get(), "mainmenu/layout");
    evas_object_event_callback_add(mm->ly, EVAS_CALLBACK_DEL, _layout_object_del, mm);

    mm->list = elm_list_add(mm->ly);
    elm_object_style_set(mm->list, "mainmenu");

    it = elm_list_item_append(mm->list, "Videos", NULL, NULL, NULL, NULL);

    elm_list_item_append(mm->list, "TV Shows", NULL, NULL, NULL, NULL);
    elm_list_item_append(mm->list, "Music", NULL, NULL, NULL, NULL);
    elm_list_item_append(mm->list, "Photos", NULL, NULL, NULL, NULL);
    elm_list_item_append(mm->list, "Settings", NULL, NULL, NULL, NULL);
    elm_list_item_append(mm->list, "Exit", NULL, NULL, NULL, NULL);
    elm_object_part_content_set(mm->ly, "list.swallow", mm->list);
    evas_object_size_hint_align_set(mm->list, -1, 0.5);
    elm_list_bounce_set(mm->list, EINA_FALSE, EINA_FALSE);
    evas_object_smart_callback_add(mm->list, "activated", _list_item_activated_cb, mm->ly);
    elm_list_go(mm->list);
    evas_object_show(mm->list);

    /* Selected the first item of the list*/
    elm_list_item_selected_set(it, EINA_TRUE);
    mm->selected = ENNA_MENU_LIST;

    mm->shelf = elm_list_add(mm->ly);

    ic = evas_object_image_filled_add(evas_object_evas_get(mm->shelf));
    evas_object_image_file_set(ic, PACKAGE_DATA_DIR"/images/cover1.jpg", NULL);
    evas_object_show(ic);
    elm_object_style_set(mm->shelf, "shelf");
    elm_list_item_append(mm->shelf, "Test", ic, NULL, _shelf_item_activated_cb, mm->ly);

    ic = evas_object_image_filled_add(evas_object_evas_get(mm->shelf));
    evas_object_image_file_set(ic, PACKAGE_DATA_DIR"/images/cover2.jpg", NULL);
    evas_object_show(ic);
    elm_list_item_append(mm->shelf, "Test2", ic, NULL,_shelf_item_activated_cb, mm->ly);

    ic = evas_object_image_filled_add(evas_object_evas_get(mm->shelf));
    evas_object_image_file_set(ic, PACKAGE_DATA_DIR"/images/cover3.jpg", NULL);
    evas_object_show(ic);
    elm_list_item_append(mm->shelf, "Test3", ic, NULL, _shelf_item_activated_cb, mm->ly);

    elm_object_part_content_set(mm->ly, "shelf.swallow", mm->shelf);
    evas_object_size_hint_align_set(mm->shelf, -1, 0.5);
    elm_list_horizontal_set(mm->shelf, EINA_TRUE);

    elm_list_go(mm->shelf);
    evas_object_show(mm->shelf);

    return mm->ly;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
