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

#include <time.h>

#include <Evas.h>
#include <Emotion.h>
#include <Elementary.h>
#include <Ems.h>

#include "enna_private.h"
#include "enna_view_player_video.h"
#include "enna_config.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

typedef struct _Enna_View_Player_Video_Data Enna_View_Player_Video_Data;

struct _Enna_View_Player_Video_Data
{
    Evas_Object *video;
    Evas_Object *layout;
};

void
_update_time_part(Evas_Object *obj, const char *part, double t)
{
   Eina_Strbuf *str;
   double s;
   int h, m;

   s = t;
   h = (int)(s / 3600.0);
   s -= h * 3600;
   m = (int)(s / 60.0);
   s -= m * 60;

   str = eina_strbuf_new();
   eina_strbuf_append_printf(str, "%02d:%02d:%02d", h, m, (int)s);

   elm_object_part_text_set(obj, part, eina_strbuf_string_get(str));

   eina_strbuf_free(str);
}

static void 
_emotion_position_update_cb(void *data, Evas_Object *obj, void *event_info)
{
   Enna_View_Player_Video_Data *priv = data;
   Evas_Object *emotion;
   Evas_Object *edje;
   double v;
   time_t timestamp;
   struct tm *t;
   Eina_Strbuf *str;

   emotion = elm_video_emotion_get(priv->video);

   _update_time_part(priv->layout, "time_current.text", 
                     emotion_object_position_get(emotion));
   _update_time_part(priv->layout, "time_duration.text", 
                     emotion_object_play_length_get(emotion));

   timestamp = time(NULL);
   timestamp += emotion_object_play_length_get(emotion) -
                emotion_object_position_get(emotion);
   t = localtime(&timestamp);
   str = eina_strbuf_new();
   eina_strbuf_append_printf(str, "End at %02dh%02d", t->tm_hour, t->tm_min);
   elm_object_part_text_set(priv->layout, "time_end_at.text", eina_strbuf_string_get(str));
   eina_strbuf_free(str);

   v = emotion_object_position_get(emotion) /
       emotion_object_play_length_get(emotion);

   edje = elm_layout_edje_get(priv->layout);
   edje_object_part_drag_value_set(edje, "time.slider", v, v);
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

Evas_Object *
enna_view_player_video_add(Enna *enna __UNUSED__, Evas_Object *parent)
{
   Evas_Object *layout;
   Enna_View_Player_Video_Data *priv;
   Evas_Object *emotion;

   priv = calloc(1, sizeof(Enna_View_Player_Video_Data));
   if (!priv)
       return NULL;

   layout = elm_layout_add(parent);
   elm_layout_file_set(layout, enna_config_theme_get(), "activity/layout/player/video");
   priv->layout = layout;

   priv->video = elm_video_add(parent);
   elm_object_part_content_set(layout, "video.swallow", priv->video);
   elm_object_style_set(priv->video, "enna");

   evas_object_data_set(layout, "Enna_View_Player_Video_Data", priv);

   emotion = elm_video_emotion_get(priv->video);
   evas_object_smart_callback_add(emotion, "position_update", _emotion_position_update_cb, priv);

   return layout;
}

void enna_view_player_video_uri_set(Evas_Object *o, const char *uri, const char *title)
{
   PRIV_GET_OR_RETURN(o, Enna_View_Player_Video_Data, priv);

   elm_video_file_set(priv->video, uri);

   elm_object_part_text_set(priv->layout, "title.text", title);
}

void enna_view_player_video_play(Evas_Object *o)
{
   PRIV_GET_OR_RETURN(o, Enna_View_Player_Video_Data, priv);

   elm_video_play(priv->video);
}

void enna_view_player_video_pause(Evas_Object *o)
{
   PRIV_GET_OR_RETURN(o, Enna_View_Player_Video_Data, priv);

   elm_video_pause(priv->video);
}

void enna_view_player_video_stop(Evas_Object *o)
{
   PRIV_GET_OR_RETURN(o, Enna_View_Player_Video_Data, priv);

   elm_video_stop(priv->video);
}
