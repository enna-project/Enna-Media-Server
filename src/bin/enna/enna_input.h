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
#ifndef _ENNA_INPUT_H
#define _ENNA_INPUT_H

#define ENNA_EVENT_BLOCK ECORE_CALLBACK_CANCEL
#define ENNA_EVENT_CONTINUE ECORE_CALLBACK_RENEW

typedef struct _Enna_Input_Listener Enna_Input_Listener;
typedef enum _Enna_Input Enna_Input;

enum _Enna_Input
{
    ENNA_INPUT_UNKNOWN,

    ENNA_INPUT_MENU,
    ENNA_INPUT_QUIT,
    ENNA_INPUT_BACK,
    ENNA_INPUT_OK,
    ENNA_INPUT_HOME,

    ENNA_INPUT_LEFT,
    ENNA_INPUT_RIGHT,
    ENNA_INPUT_UP,
    ENNA_INPUT_DOWN,

    ENNA_INPUT_NEXT,
    ENNA_INPUT_PREV,
    ENNA_INPUT_FIRST,
    ENNA_INPUT_LAST,

    ENNA_INPUT_FULLSCREEN,
    ENNA_INPUT_INFO,
    ENNA_INPUT_FRAMEDROP,
    ENNA_INPUT_ROTATE_CW,
    ENNA_INPUT_ROTATE_CCW,

    /* Player controls */
    ENNA_INPUT_PLAY,
    ENNA_INPUT_STOP,
    ENNA_INPUT_PAUSE,
    ENNA_INPUT_FORWARD,
    ENNA_INPUT_REWIND,
    ENNA_INPUT_RECORD,

    /* Audio controls */
    ENNA_INPUT_VOLPLUS,
    ENNA_INPUT_VOLMINUS,
    ENNA_INPUT_MUTE,
    ENNA_INPUT_AUDIO_PREV,
    ENNA_INPUT_AUDIO_NEXT,
    ENNA_INPUT_AUDIO_DELAY_PLUS,
    ENNA_INPUT_AUDIO_DELAY_MINUS,

    /* Subtitles controls */
    ENNA_INPUT_SUBTITLES,
    ENNA_INPUT_SUBS_PREV,
    ENNA_INPUT_SUBS_NEXT,
    ENNA_INPUT_SUBS_ALIGN,
    ENNA_INPUT_SUBS_POS_PLUS,
    ENNA_INPUT_SUBS_POS_MINUS,
    ENNA_INPUT_SUBS_SCALE_PLUS,
    ENNA_INPUT_SUBS_SCALE_MINUS,
    ENNA_INPUT_SUBS_DELAY_PLUS,
    ENNA_INPUT_SUBS_DELAY_MINUS,

    /* TV controls */
    ENNA_INPUT_RED,
    ENNA_INPUT_GREEN,
    ENNA_INPUT_YELLOW,
    ENNA_INPUT_BLUE,
    ENNA_INPUT_CHANPREV,
    ENNA_INPUT_CHANPLUS,
    ENNA_INPUT_CHANMINUS,
    ENNA_INPUT_SCHEDULE,
    ENNA_INPUT_CHANNELS,
    ENNA_INPUT_TIMERS,
    ENNA_INPUT_RECORDINGS,

    /* Special characters */
    ENNA_INPUT_KEY_SPACE,

    /* Number characters */
    ENNA_INPUT_KEY_0,
    ENNA_INPUT_KEY_1,
    ENNA_INPUT_KEY_2,
    ENNA_INPUT_KEY_3,
    ENNA_INPUT_KEY_4,
    ENNA_INPUT_KEY_5,
    ENNA_INPUT_KEY_6,
    ENNA_INPUT_KEY_7,
    ENNA_INPUT_KEY_8,
    ENNA_INPUT_KEY_9,

    /* Alphabetical characters */
    ENNA_INPUT_KEY_A,
    ENNA_INPUT_KEY_B,
    ENNA_INPUT_KEY_C,
    ENNA_INPUT_KEY_D,
    ENNA_INPUT_KEY_E,
    ENNA_INPUT_KEY_F,
    ENNA_INPUT_KEY_G,
    ENNA_INPUT_KEY_H,
    ENNA_INPUT_KEY_I,
    ENNA_INPUT_KEY_J,
    ENNA_INPUT_KEY_K,
    ENNA_INPUT_KEY_L,
    ENNA_INPUT_KEY_M,
    ENNA_INPUT_KEY_N,
    ENNA_INPUT_KEY_O,
    ENNA_INPUT_KEY_P,
    ENNA_INPUT_KEY_Q,
    ENNA_INPUT_KEY_R,
    ENNA_INPUT_KEY_S,
    ENNA_INPUT_KEY_T,
    ENNA_INPUT_KEY_U,
    ENNA_INPUT_KEY_V,
    ENNA_INPUT_KEY_W,
    ENNA_INPUT_KEY_X,
    ENNA_INPUT_KEY_Y,
    ENNA_INPUT_KEY_Z,
};


Eina_Bool enna_input_event_emit(Enna_Input in);
Enna_Input_Listener *enna_input_listener_add(const char *name, Eina_Bool (*func)(void *data, Enna_Input event), void *data);
void enna_input_listener_promote(Enna_Input_Listener *il);
void enna_input_listener_demote(Enna_Input_Listener *il);
void enna_input_listener_del(Enna_Input_Listener *il);

#endif /* _ENNA_INPUT_H */
