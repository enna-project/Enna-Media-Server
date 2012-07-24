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
} enna_keymap[] = {
  { "Super_R",      ECORE_NONE,              ENNA_INPUT_MENU          },
  { "Meta_R",       ECORE_NONE,              ENNA_INPUT_MENU          },
  { "Hyper_R",      ECORE_NONE,              ENNA_INPUT_MENU          },
  { "Escape",       ECORE_NONE,              ENNA_INPUT_QUIT          },
  { "BackSpace",    ECORE_NONE,              ENNA_INPUT_BACK          },
  { "Return",       ECORE_NONE,              ENNA_INPUT_OK            },
  { "KP_Enter",     ECORE_NONE,              ENNA_INPUT_OK            },
  { "Super_L",      ECORE_NONE,              ENNA_INPUT_HOME          },
  { "Meta_L",       ECORE_NONE,              ENNA_INPUT_HOME          },
  { "Hyper_L",      ECORE_NONE,              ENNA_INPUT_HOME          },

  { "Left",         ECORE_NONE,              ENNA_INPUT_LEFT          },
  { "Right",        ECORE_NONE,              ENNA_INPUT_RIGHT         },
  { "Up",           ECORE_NONE,              ENNA_INPUT_UP            },
  { "KP_Up",        ECORE_NONE,              ENNA_INPUT_UP            },
  { "Down",         ECORE_NONE,              ENNA_INPUT_DOWN          },
  { "KP_Down",      ECORE_NONE,              ENNA_INPUT_DOWN          },

  { "Next",         ECORE_NONE,              ENNA_INPUT_NEXT          },
  { "Prior",        ECORE_NONE,              ENNA_INPUT_PREV          },
  { "Home",         ECORE_NONE,              ENNA_INPUT_FIRST         },
  { "KP_Home",      ECORE_NONE,              ENNA_INPUT_FIRST         },
  { "End",          ECORE_NONE,              ENNA_INPUT_LAST          },
  { "KP_End",       ECORE_NONE,              ENNA_INPUT_LAST          },

  { "f",            ECORE_CTRL,              ENNA_INPUT_FULLSCREEN    },
  { "F",            ECORE_CTRL,              ENNA_INPUT_FULLSCREEN    },
  { "i",            ECORE_CTRL,              ENNA_INPUT_INFO          },
  { "I",            ECORE_CTRL,              ENNA_INPUT_INFO          },
  { "w",            ECORE_CTRL,              ENNA_INPUT_FRAMEDROP     },
  { "W",            ECORE_CTRL,              ENNA_INPUT_FRAMEDROP     },
  { "e",            ECORE_CTRL,              ENNA_INPUT_ROTATE_CW     },
  { "E",            ECORE_CTRL,              ENNA_INPUT_ROTATE_CW     },
  { "r",            ECORE_CTRL,              ENNA_INPUT_ROTATE_CCW    },
  { "R",            ECORE_CTRL,              ENNA_INPUT_ROTATE_CCW    },

  /* Player controls */
  { "space",        ECORE_CTRL,              ENNA_INPUT_PLAY          },
  { "v",            ECORE_CTRL,              ENNA_INPUT_STOP          },
  { "V",            ECORE_CTRL,              ENNA_INPUT_STOP          },
  { "b",            ECORE_CTRL,              ENNA_INPUT_PAUSE         },
  { "B",            ECORE_CTRL,              ENNA_INPUT_PAUSE         },
  { "h",            ECORE_CTRL,              ENNA_INPUT_FORWARD       },
  { "H",            ECORE_CTRL,              ENNA_INPUT_FORWARD       },
  { "g",            ECORE_CTRL,              ENNA_INPUT_REWIND        },
  { "G",            ECORE_CTRL,              ENNA_INPUT_REWIND        },
  { "n",            ECORE_CTRL,              ENNA_INPUT_RECORD        },
  { "N",            ECORE_CTRL,              ENNA_INPUT_RECORD        },

  /* Audio controls */
  { "plus",         ECORE_NONE,              ENNA_INPUT_VOLPLUS           },
  { "plus",         ECORE_SHIFT,             ENNA_INPUT_VOLPLUS           },
  { "KP_Add",       ECORE_NONE,              ENNA_INPUT_VOLPLUS           },
  { "minus",        ECORE_NONE,              ENNA_INPUT_VOLMINUS          },
  { "KP_Subtract",  ECORE_NONE,              ENNA_INPUT_VOLMINUS          },
  { "m",            ECORE_CTRL,              ENNA_INPUT_MUTE              },
  { "M",            ECORE_CTRL,              ENNA_INPUT_MUTE              },
  { "k",            ECORE_CTRL,              ENNA_INPUT_AUDIO_PREV        },
  { "K",            ECORE_CTRL,              ENNA_INPUT_AUDIO_PREV        },
  { "l",            ECORE_CTRL,              ENNA_INPUT_AUDIO_NEXT        },
  { "L",            ECORE_CTRL,              ENNA_INPUT_AUDIO_NEXT        },
  { "p",            ECORE_CTRL,              ENNA_INPUT_AUDIO_DELAY_PLUS  },
  { "P",            ECORE_CTRL,              ENNA_INPUT_AUDIO_DELAY_PLUS  },
  { "o",            ECORE_CTRL,              ENNA_INPUT_AUDIO_DELAY_MINUS },
  { "O",            ECORE_CTRL,              ENNA_INPUT_AUDIO_DELAY_MINUS },

  /* Subtitles controls */
  { "s",            ECORE_CTRL,              ENNA_INPUT_SUBTITLES        },
  { "S",            ECORE_CTRL,              ENNA_INPUT_SUBTITLES        },
  { "g",            ECORE_CTRL,              ENNA_INPUT_SUBS_PREV        },
  { "G",            ECORE_CTRL,              ENNA_INPUT_SUBS_PREV        },
  { "y",            ECORE_CTRL,              ENNA_INPUT_SUBS_NEXT        },
  { "Y",            ECORE_CTRL,              ENNA_INPUT_SUBS_NEXT        },
  { "a",            ECORE_CTRL,              ENNA_INPUT_SUBS_ALIGN       },
  { "A",            ECORE_CTRL,              ENNA_INPUT_SUBS_ALIGN       },
  { "t",            ECORE_CTRL,              ENNA_INPUT_SUBS_POS_PLUS    },
  { "T",            ECORE_CTRL,              ENNA_INPUT_SUBS_POS_PLUS    },
  { "r",            ECORE_CTRL,              ENNA_INPUT_SUBS_POS_MINUS   },
  { "R",            ECORE_CTRL,              ENNA_INPUT_SUBS_POS_MINUS   },
  { "j",            ECORE_CTRL,              ENNA_INPUT_SUBS_SCALE_PLUS  },
  { "J",            ECORE_CTRL,              ENNA_INPUT_SUBS_SCALE_PLUS  },
  { "i",            ECORE_CTRL,              ENNA_INPUT_SUBS_SCALE_MINUS },
  { "I",            ECORE_CTRL,              ENNA_INPUT_SUBS_SCALE_MINUS },
  { "x",            ECORE_CTRL,              ENNA_INPUT_SUBS_DELAY_PLUS  },
  { "X",            ECORE_CTRL,              ENNA_INPUT_SUBS_DELAY_PLUS  },
  { "z",            ECORE_CTRL,              ENNA_INPUT_SUBS_DELAY_MINUS },
  { "Z",            ECORE_CTRL,              ENNA_INPUT_SUBS_DELAY_MINUS },

  /* TV controls */
  { "F1",           ECORE_NONE,              ENNA_INPUT_RED           },
  { "F2",           ECORE_NONE,              ENNA_INPUT_GREEN         },
  { "F3",           ECORE_NONE,              ENNA_INPUT_YELLOW        },
  { "F4",           ECORE_NONE,              ENNA_INPUT_BLUE          },
  { "c",            ECORE_CTRL,              ENNA_INPUT_CHANPREV      },
  { "C",            ECORE_CTRL,              ENNA_INPUT_CHANPREV      },
  { "F5",           ECORE_NONE,              ENNA_INPUT_SCHEDULE      },
  { "F6",           ECORE_NONE,              ENNA_INPUT_CHANNELS      },
  { "F7",           ECORE_NONE,              ENNA_INPUT_TIMERS        },
  { "F8",           ECORE_NONE,              ENNA_INPUT_RECORDINGS    },

  /* Special characters */
  { "space",        ECORE_NONE,              ENNA_INPUT_KEY_SPACE     },

  /* Number characters */
  { "0",            ECORE_NONE,              ENNA_INPUT_KEY_0         },
  { "0",            ECORE_SHIFT,             ENNA_INPUT_KEY_0         },
  { "KP_0",         ECORE_NONE,              ENNA_INPUT_KEY_0         },
  { "1",            ECORE_NONE,              ENNA_INPUT_KEY_1         },
  { "1",            ECORE_SHIFT,             ENNA_INPUT_KEY_0         },
  { "KP_1",         ECORE_NONE,              ENNA_INPUT_KEY_1         },
  { "2",            ECORE_NONE,              ENNA_INPUT_KEY_2         },
  { "2",            ECORE_SHIFT,             ENNA_INPUT_KEY_0         },
  { "KP_2",         ECORE_NONE,              ENNA_INPUT_KEY_2         },
  { "3",            ECORE_NONE,              ENNA_INPUT_KEY_3         },
  { "3",            ECORE_SHIFT,             ENNA_INPUT_KEY_0         },
  { "KP_3",         ECORE_NONE,              ENNA_INPUT_KEY_3         },
  { "4",            ECORE_NONE,              ENNA_INPUT_KEY_4         },
  { "4",            ECORE_SHIFT,             ENNA_INPUT_KEY_0         },
  { "KP_4",         ECORE_NONE,              ENNA_INPUT_KEY_4         },
  { "5",            ECORE_NONE,              ENNA_INPUT_KEY_5         },
  { "5",            ECORE_SHIFT,             ENNA_INPUT_KEY_0         },
  { "KP_5",         ECORE_NONE,              ENNA_INPUT_KEY_5         },
  { "6",            ECORE_NONE,              ENNA_INPUT_KEY_6         },
  { "6",            ECORE_SHIFT,             ENNA_INPUT_KEY_0         },
  { "KP_6",         ECORE_NONE,              ENNA_INPUT_KEY_6         },
  { "7",            ECORE_NONE,              ENNA_INPUT_KEY_7         },
  { "7",            ECORE_SHIFT,             ENNA_INPUT_KEY_0         },
  { "KP_7",         ECORE_NONE,              ENNA_INPUT_KEY_7         },
  { "8",            ECORE_NONE,              ENNA_INPUT_KEY_8         },
  { "8",            ECORE_SHIFT,             ENNA_INPUT_KEY_0         },
  { "KP_8",         ECORE_NONE,              ENNA_INPUT_KEY_8         },
  { "9",            ECORE_NONE,              ENNA_INPUT_KEY_9         },
  { "9",            ECORE_SHIFT,             ENNA_INPUT_KEY_0         },
  { "KP_9",         ECORE_NONE,              ENNA_INPUT_KEY_9         },

  /* Alphabetical characters */
  { "a",            ECORE_NONE,              ENNA_INPUT_KEY_A         },
  { "b",            ECORE_NONE,              ENNA_INPUT_KEY_B         },
  { "c",            ECORE_NONE,              ENNA_INPUT_KEY_C         },
  { "d",            ECORE_NONE,              ENNA_INPUT_KEY_D         },
  { "e",            ECORE_NONE,              ENNA_INPUT_KEY_E         },
  { "f",            ECORE_NONE,              ENNA_INPUT_KEY_F         },
  { "g",            ECORE_NONE,              ENNA_INPUT_KEY_G         },
  { "h",            ECORE_NONE,              ENNA_INPUT_KEY_H         },
  { "i",            ECORE_NONE,              ENNA_INPUT_KEY_I         },
  { "j",            ECORE_NONE,              ENNA_INPUT_KEY_J         },
  { "k",            ECORE_NONE,              ENNA_INPUT_KEY_K         },
  { "l",            ECORE_NONE,              ENNA_INPUT_KEY_L         },
  { "m",            ECORE_NONE,              ENNA_INPUT_KEY_M         },
  { "n",            ECORE_NONE,              ENNA_INPUT_KEY_N         },
  { "o",            ECORE_NONE,              ENNA_INPUT_KEY_O         },
  { "p",            ECORE_NONE,              ENNA_INPUT_KEY_P         },
  { "q",            ECORE_NONE,              ENNA_INPUT_KEY_Q         },
  { "r",            ECORE_NONE,              ENNA_INPUT_KEY_R         },
  { "s",            ECORE_NONE,              ENNA_INPUT_KEY_S         },
  { "t",            ECORE_NONE,              ENNA_INPUT_KEY_T         },
  { "u",            ECORE_NONE,              ENNA_INPUT_KEY_U         },
  { "v",            ECORE_NONE,              ENNA_INPUT_KEY_V         },
  { "w",            ECORE_NONE,              ENNA_INPUT_KEY_W         },
  { "x",            ECORE_NONE,              ENNA_INPUT_KEY_X         },
  { "y",            ECORE_NONE,              ENNA_INPUT_KEY_Y         },
  { "z",            ECORE_NONE,              ENNA_INPUT_KEY_Z         },

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
             INF("Key pressed : [%d] + %s",
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
             INF("Key pressed : %s",
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
   ecore_event_handler_add (ECORE_EVENT_KEY_DOWN, _ecore_event_key_down_cb, NULL);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

