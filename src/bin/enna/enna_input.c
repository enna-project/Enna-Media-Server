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

#include <Ecore.h>

#include "enna_private.h"
#include "enna_input.h"

struct _Enna_Input_Listener {
   const char *name;
   Eina_Bool (*func)(void *data, Enna_Input event);
   void *data;
};

/* Local Globals */
static Eina_List *_listeners = NULL;

/* Public Functions */
Eina_Bool
enna_input_event_emit(Enna_Input in)
{
   Enna_Input_Listener *il;
   Eina_List *l;
   Eina_Bool ret;

   DBG("Input emit: %d (listeners: %d)", in, eina_list_count(_listeners));

   EINA_LIST_FOREACH(_listeners, l, il)
     {
        DBG("  emit to: %s", il->name);
        if (!il->func) continue;

        ret = il->func(il->data, in);
        if (ret == ENNA_EVENT_BLOCK)
          {
             DBG("  emission stopped by: %s", il->name);
             return EINA_TRUE;
          }
     }

   return EINA_TRUE;
}

Enna_Input_Listener *
enna_input_listener_add(const char *name,
                        Eina_Bool(*func)(void *data, Enna_Input event),
                        void *data)
{
   Enna_Input_Listener *il;

   DBG("listener add: %s", name);
   il = calloc(1, sizeof(Enna_Input_Listener));
   if (!il) return NULL;
   il->name = eina_stringshare_add(name);
   il->func = func;
   il->data = data;

   _listeners = eina_list_prepend(_listeners, il);
   return il;
}

void
enna_input_listener_promote(Enna_Input_Listener *il)
{
   Eina_List *l;

   l = eina_list_data_find_list(_listeners, il);
   if (!l) return;

   _listeners  = eina_list_promote_list(_listeners, l);
}

void
enna_input_listener_demote(Enna_Input_Listener *il)
{
   Eina_List *l;

   l = eina_list_data_find_list(_listeners, il);
   if (!l) return;

   _listeners  = eina_list_demote_list(_listeners, l);
}

void
enna_input_listener_del(Enna_Input_Listener *il)
{
   if (!il) return;
   DBG("listener del: %s", il->name);
   _listeners = eina_list_remove(_listeners, il);
   eina_stringshare_del(il->name);
   if (il) free(il);
}
