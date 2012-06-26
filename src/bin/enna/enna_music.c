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
#include "enna_view_music_grid.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static int _music_init_count = 0;

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

int
enna_music_init(void)
{
   Evas_Object *obj;

   if (++_music_init_count != 1)
     return _music_init_count;

   enna_activity_init();

   obj = enna_view_music_grid_add(enna->ly);

   enna_activity_add("Music", obj);

   return _music_init_count;
}

void
enna_music_shutdown(void)
{
}
