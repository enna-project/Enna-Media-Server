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
#include <Ecore_Input.h>

#include "enna_private.h"
#include "enna_input.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static const struct
{
   const char *keyname;
   Ecore_Event_Modifier modifier;
   Enna_Input input;
   const char *enna_input_name
} enna_keymap[] = {
  { "Super_R",      ECORE_NONE,              ENNA_INPUT_MENU          , "ENNA_INPUT_MENU"},
  { "Meta_R",       ECORE_NONE,              ENNA_INPUT_MENU          , "ENNA_INPUT_MENU"},
  { "Hyper_R",      ECORE_NONE,              ENNA_INPUT_MENU          , "ENNA_INPUT_MENU"},
  { "Escape",       ECORE_NONE,              ENNA_INPUT_QUIT          , "ENNA_INPUT_QUIT"},
  { "BackSpace",    ECORE_NONE,              ENNA_INPUT_BACK          , "ENNA_INPUT_BACK"},
  { "Return",       ECORE_NONE,              ENNA_INPUT_OK            , "ENNA_INPUT_OK"},
  { "KP_Enter",     ECORE_NONE,              ENNA_INPUT_OK            , "ENNA_INPUT_OK"},
  { "Super_L",      ECORE_NONE,              ENNA_INPUT_HOME          , "ENNA_INPUT_HOME"},
  { "Meta_L",       ECORE_NONE,              ENNA_INPUT_HOME          , "ENNA_INPUT_HOME"},
  { "Hyper_L",      ECORE_NONE,              ENNA_INPUT_HOME          , "ENNA_INPUT_HOME"},

  { "Left",         ECORE_NONE,              ENNA_INPUT_LEFT          , "ENNA_INPUT_LEFT"},
  { "Right",        ECORE_NONE,              ENNA_INPUT_RIGHT         , "ENNA_INPUT_RIGHT"},
  { "Up",           ECORE_NONE,              ENNA_INPUT_UP            , "ENNA_INPUT_UP"},
  { "KP_Up",        ECORE_NONE,              ENNA_INPUT_UP            , "ENNA_INPUT_UP"},
  { "Down",         ECORE_NONE,              ENNA_INPUT_DOWN          , "ENNA_INPUT_DOWN"},
  { "KP_Down",      ECORE_NONE,              ENNA_INPUT_DOWN          , "ENNA_INPUT_DOWN"},

  { "Next",         ECORE_NONE,              ENNA_INPUT_NEXT          , "ENNA_INPUT_NEXT"},
  { "Prior",        ECORE_NONE,              ENNA_INPUT_PREV          , "ENNA_INPUT_PREV"},
  { "Home",         ECORE_NONE,              ENNA_INPUT_FIRST         , "ENNA_INPUT_FIRST"},
  { "KP_Home",      ECORE_NONE,              ENNA_INPUT_FIRST         , "ENNA_INPUT_FIRST"},
  { "End",          ECORE_NONE,              ENNA_INPUT_LAST          , "ENNA_INPUT_LAST"},
  { "KP_End",       ECORE_NONE,              ENNA_INPUT_LAST          , "ENNA_INPUT_LAST"},

  { "f",            ECORE_CTRL,              ENNA_INPUT_FULLSCREEN    , "ENNA_INPUT_FULLSCREEN"},
  { "F",            ECORE_CTRL,              ENNA_INPUT_FULLSCREEN    , "ENNA_INPUT_FULLSCREEN"},
  { "i",            ECORE_CTRL,              ENNA_INPUT_INFO          , "ENNA_INPUT_INFO"},
  { "I",            ECORE_CTRL,              ENNA_INPUT_INFO          , "ENNA_INPUT_INFO"},

  /* Player controls */
  { "space",        ECORE_CTRL,              ENNA_INPUT_PLAY          , "ENNA_INPUT_PLAY"},
  { "s",            ECORE_CTRL,              ENNA_INPUT_STOP          , "ENNA_INPUT_STOP"},
  { "S",            ECORE_CTRL,              ENNA_INPUT_STOP          , "ENNA_INPUT_STOP"},
  { "p",            ECORE_CTRL,              ENNA_INPUT_PAUSE         , "ENNA_INPUT_PAUSE"},
  { "P",            ECORE_CTRL,              ENNA_INPUT_PAUSE         , "ENNA_INPUT_PAUSE"},
  { "n",            ECORE_CTRL,              ENNA_INPUT_FORWARD       , "ENNA_INPUT_FORWARD"},
  { "N",            ECORE_CTRL,              ENNA_INPUT_FORWARD       , "ENNA_INPUT_FORWARD"},
  { "p",            ECORE_CTRL,              ENNA_INPUT_REWIND        , "ENNA_INPUT_REWIND"},
  { "P",            ECORE_CTRL,              ENNA_INPUT_REWIND        , "ENNA_INPUT_REWIND"},

  /* Audio controls */
  { "plus",         ECORE_NONE,              ENNA_INPUT_VOLPLUS           , "ENNA_INPUT_VOLPLUS"},
  { "plus",         ECORE_SHIFT,             ENNA_INPUT_VOLPLUS           , "ENNA_INPUT_VOLPLUS"},
  { "KP_Add",       ECORE_NONE,              ENNA_INPUT_VOLPLUS           , "ENNA_INPUT_VOLPLUS"},
  { "minus",        ECORE_NONE,              ENNA_INPUT_VOLMINUS          , "ENNA_INPUT_VOLMINUS"},
  { "KP_Subtract",  ECORE_NONE,              ENNA_INPUT_VOLMINUS          , "ENNA_INPUT_VOLMINUS"},
  { "m",            ECORE_CTRL,              ENNA_INPUT_MUTE              , "ENNA_INPUT_MUTE"},
  { "M",            ECORE_CTRL,              ENNA_INPUT_MUTE              , "ENNA_INPUT_MUTE"},

  /* Special characters */

  { NULL,           ECORE_NONE,              ENNA_INPUT_UNKNOWN       }
};

static Enna_Input
_input_event_modifier(Ecore_Event_Key *ev)
{
   int i;

   for (i = 0; enna_keymap[i].keyname; i++)
     {
        if (ev->modifiers == enna_keymap[i].modifier &&
            !strcmp(enna_keymap[i].keyname, ev->key))
          {
             DBG("Key pressed : [%d] + %s",
                 enna_keymap[i].modifier, enna_keymap[i] );
             return enna_keymap[i].input;
          }
     }

   return ENNA_INPUT_UNKNOWN;
}

static Enna_Input
_input_event(Ecore_Event_Key *ev)
{
   int i;

   DBG("ev->key : %s", ev->key);

   for (i = 0; enna_keymap[i].keyname; i++)
     {
        if ((enna_keymap[i].modifier == ECORE_NONE) &&
            ev->key && !strcmp(enna_keymap[i].keyname, ev->key))
          {
             DBG("Key pressed : %s",
                 enna_keymap[i].keyname );
             return enna_keymap[i].input;
          }
     }

   return ENNA_INPUT_UNKNOWN;
}

static Enna_Input
_get_input_from_event(Ecore_Event_Key *ev)
{
    if (!ev)
        return ENNA_INPUT_UNKNOWN;

    /* discard some modifiers */
    if (ev->modifiers >= ECORE_EVENT_LOCK_CAPS)
        ev->modifiers -= ECORE_EVENT_LOCK_CAPS;
    if (ev->modifiers >= ECORE_EVENT_LOCK_NUM)
        ev->modifiers -= ECORE_EVENT_LOCK_NUM;
    if (ev->modifiers >= ECORE_EVENT_LOCK_SCROLL)
        ev->modifiers -= ECORE_EVENT_LOCK_SCROLL;

    return (ev->modifiers && ev->modifiers < ECORE_LAST) ?
      _input_event_modifier(ev) : _input_event(ev);
}

static Eina_Bool
_ecore_event_key_down_cb(void *data, int type, void *event)
{
    Ecore_Event_Key *e = event;
    Enna_Input in;

    in = _get_input_from_event(e);
    if (in != ENNA_INPUT_UNKNOWN)
        enna_input_event_emit(in);

    return ECORE_CALLBACK_CANCEL;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void
enna_keyboard_init(void)
{
   ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _ecore_event_key_down_cb, NULL);
}

const char *
enna_keyboard_input_name_get(Enna_Input *ei)
{
   int i;
   for (i = 0; enna_keymap[i].keyname; i++)
     {
        if (enna_keymap[i].input == ei)
          {
             return enna_keymap[i].enna_input_name;
          }
     }
   return NULL;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

