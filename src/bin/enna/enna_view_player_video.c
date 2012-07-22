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

#include <Evas.h>
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
};

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

   priv = calloc(1, sizeof(Enna_View_Player_Video_Data));
   if (!priv)
       return NULL;

   layout = elm_layout_add(parent);
   elm_layout_file_set(layout, enna_config_theme_get(), "activity/layout/player/video");

   priv->video = elm_video_add(parent);
   elm_object_part_content_set(layout, "video.swallow", priv->video);
   elm_object_style_set(priv->video, "enna");

   evas_object_data_set(layout, "Enna_View_Player_Video_Data", priv);

   return layout;
}

void enna_view_player_video_uri_set(Evas_Object *o, const char *uri)
{
   PRIV_GET_OR_RETURN(o, Enna_View_Player_Video_Data, priv);

   elm_video_file_set(priv->video, uri);
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
