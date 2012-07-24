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

struct _Enna_Mainmenu
{
   Evas_Object *ly;
   Evas_Object *list;
   Evas_Object *shelf;
   Enna_Input_Listener *il;
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
static Eina_Bool
_input_event(void *data, Enna_Input event)
{
   INF("Mainmenu input event %d", event);
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
    elm_list_item_selected_set(it, EINA_TRUE);
    elm_list_item_append(mm->list, "TV Shows", NULL, NULL, NULL, NULL);
    elm_list_item_append(mm->list, "Music", NULL, NULL, NULL, NULL);
    elm_list_item_append(mm->list, "Photos", NULL, NULL, NULL, NULL);
    elm_list_item_append(mm->list, "Settings", NULL, NULL, NULL, NULL);
    elm_list_item_append(mm->list, "Exit", NULL, NULL, NULL, NULL);
    elm_object_part_content_set(mm->ly, "list.swallow", mm->list);
    evas_object_size_hint_align_set(mm->list, -1, 0.5);
    elm_list_bounce_set(mm->list, EINA_FALSE, EINA_FALSE);
    evas_object_smart_callback_add(mm->list, "activated", _list_item_activated_cb, mm->ly);
    evas_object_show(mm->list);


    mm->shelf = elm_list_add(mm->ly);

    ic = evas_object_image_filled_add(evas_object_evas_get(mm->shelf));
    evas_object_image_file_set(ic, PACKAGE_DATA_DIR"/images/cover1.jpg", NULL);
    evas_object_show(ic);
    elm_object_style_set(mm->shelf, "shelf");
    it = elm_list_item_append(mm->shelf, "Test", ic, NULL, _shelf_item_activated_cb, mm->ly);
    elm_list_item_selected_set(it, EINA_TRUE);
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
    return mm->ly;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
