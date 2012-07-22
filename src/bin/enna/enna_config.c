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
#include <Ecore_File.h>
#include <Elementary.h>

#include "enna_private.h"
#include "enna_config.h"

static const char *_theme = NULL;

Eina_Bool
enna_config_init(void)
{
    const char *config;

    config = enna_config_theme_get();
    enna_config_config_get();

    elm_theme_overlay_add(NULL, enna_config_theme_get());

    return EINA_TRUE;
}

void
enna_config_shutdown(void)
{
    const char *theme;
    const char *config;

    theme = enna_config_theme_get();
    config = enna_config_config_get();

    eina_stringshare_del(theme);
    eina_stringshare_del(config);
}


const char *
enna_config_config_get(void)
{
   static const char *config = NULL;
   char tmp[PATH_MAX];
   char cfg[PATH_MAX];

   if (config)
     return config;

   snprintf(tmp, sizeof(tmp), "%s/.config/enna/", getenv("HOME"));
   if (!ecore_file_is_dir(tmp))
      ecore_file_mkpath(tmp);

   snprintf(cfg, sizeof(cfg), "%s/%s", tmp, ENNA_CONFIG_FILE);
   INF("Config : %s", cfg);
   config = eina_stringshare_add(cfg);
   return config;
}

void
enna_config_theme_set(char *theme)
{
   if (_theme)
     eina_stringshare_del(_theme);

   _theme = eina_stringshare_add(theme);

   INF("Theme : %s", _theme);
}

const char *
enna_config_theme_get(void)
{
   char tmp[PATH_MAX];

   if (_theme)
     return _theme;

   snprintf(tmp, sizeof(tmp),
            PACKAGE_DATA_DIR"/theme/default.edj");

   INF("Theme : %s", tmp);

   _theme = eina_stringshare_add(tmp);
   return _theme;
}
