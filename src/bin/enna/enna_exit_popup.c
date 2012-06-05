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

#include <stdio.h>
#include <stdlib.h>

#include <Evas.h>
#include <Elementary.h>

#include "enna_private.h"
#include "enna_config.h"
#include "enna_exit_popup.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static Evas_Object *popup = NULL;

static void
_yes_clicked_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   elm_exit();
}

static void
_no_clicked_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_del(popup);
   popup = NULL;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

void enna_exit_popup_show(Evas_Object *parent)
{
   Evas_Object *btn, *btn2;
   Eina_List *focus_chain = NULL;
   Eina_List *l;
   Evas_Object *obj;

   if (popup)
     {
        evas_object_del(popup);
        printf("WTF?\n");
     }
   popup = elm_popup_add(parent);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_part_text_set(popup, "title,text", "Are you sure you want to quit enna ?");
   btn = elm_button_add(popup);
   elm_object_text_set(btn, "Yes I'm sure !");
   elm_object_part_content_set(popup, "button1", btn);
   elm_object_style_set(btn, "enna");
   evas_object_smart_callback_add(btn, "clicked", _yes_clicked_cb, popup);
   //  evas_object_smart_callback_add(popup, "block,clicked", _block_clicked_cb,
   //                               NULL);

   btn2 = elm_button_add(popup);
   elm_object_text_set(btn2, "No, Thanks.");
   elm_object_part_content_set(popup, "button2", btn2);
   elm_object_style_set(btn2, "enna");
   evas_object_smart_callback_add(btn2, "clicked", _no_clicked_cb, popup);

   evas_object_show(popup);

   focus_chain = eina_list_append(focus_chain, btn);
   focus_chain = eina_list_append(focus_chain, btn2);
   elm_object_focus_custom_chain_set(enna->ly, focus_chain);
   elm_object_focus_set(btn2, EINA_TRUE);
}

void enna_exit_popup_hide(void)
{
   if (popup)
     evas_object_hide(popup);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

