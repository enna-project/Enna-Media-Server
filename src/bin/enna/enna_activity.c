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
#include "enna_activity.h"
#include "enna_config.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static int _activity_init_count = 0;

static Eina_Hash *_activities = NULL;

typedef struct _Enna_Activity Enna_Activity;

struct _Enna_Activity
{
   const char *name;
   Evas_Object *obj;

   Enna_Activity_Add_Func activity_add;
};

static void
_activity_free_cb(Enna_Activity *act)
{
   eina_stringshare_del(act->name);
   free(act);
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

int
enna_activity_init(void)
{
   if (++_activity_init_count != 1)
     return _activity_init_count;

   _activities = eina_hash_string_superfast_new((Eina_Free_Cb)_activity_free_cb);

   return _activity_init_count;
}

void
enna_activity_shutdown(void)
{
   eina_hash_free(_activities);
}

void
enna_activity_register(const char *activity_name, Enna_Activity_Add_Func activity_add)
{
   Enna_Activity *act;

   if (!activity_name && !activity_add)
     {
        ERR("Can't register activity !");
        return;
     }

   act = calloc(1, sizeof(Enna_Activity));
   act->name = eina_stringshare_add(activity_name);
   act->activity_add = activity_add;

   eina_hash_add(_activities, act->name, act);

   DBG("New activity registred: %s", act->name);
}

Evas_Object *
enna_activity_select(Enna *enna, const char *activity_name)
{
   Enna_Activity *act;

   act = eina_hash_find(_activities, activity_name);
   if (act)
     {
        Eina_List *focus_chain = NULL;

        //Let's create our new activity
        act->obj = act->activity_add(enna, enna->naviframe);
        elm_naviframe_item_push(enna->naviframe, NULL, NULL, NULL, act->obj, "enna");

        focus_chain = eina_list_append(focus_chain, act->obj);
        elm_object_focus_custom_chain_set(enna->layout, focus_chain);
        elm_object_focus_set(act->obj, EINA_TRUE);

        return act->obj;
     }
   else
     {
        ERR("Unable to select activity : %s", activity_name);
        return NULL;
     }
}
