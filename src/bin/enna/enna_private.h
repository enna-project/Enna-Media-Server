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

#ifndef _ENNA_PRIVATE_H
#define _ENNA_PRIVATE_H

#include <Evas.h>

/* variable and macros used for the eina_log module */
extern int _enna_log_dom_global;
/*
 * Macros that are used everywhere
 *
 * the first four macros are the general macros for the lib
 */

#ifdef ENNA_DEFAULT_LOG_COLOR
# undef ENNA_DEFAULT_LOG_COLOR
#endif /* ifdef ENNA_DEFAULT_LOG_COLOR */
#define ENNA_DEFAULT_LOG_COLOR EINA_COLOR_LIGHTBLUE
#ifdef ERR
# undef ERR
#endif /* ifdef ERR */
#define ERR(...)  EINA_LOG_DOM_ERR(_enna_log_dom_global, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif /* ifdef DBG */
#define DBG(...)  EINA_LOG_DOM_DBG(_enna_log_dom_global, __VA_ARGS__)
#ifdef INF
# undef INF
#endif /* ifdef INF */
#define INF(...)  EINA_LOG_DOM_INFO(_enna_log_dom_global, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif /* ifdef WRN */
#define WRN(...)  EINA_LOG_DOM_WARN(_enna_log_dom_global, __VA_ARGS__)
#ifdef CRIT
# undef CRIT
#endif /* ifdef CRIT */
#define CRIT(...) EINA_LOG_DOM_CRIT(_enna_log_dom_global, __VA_ARGS__)


#define PRIV_DATA_GET(o, type, ptr) \
  type * ptr = evas_object_data_get(o, #type)

#define PRIV_GET_OR_RETURN(o, type, ptr)           \
  PRIV_DATA_GET(o, type, ptr);                     \
  if (!ptr)                                        \
    {                                              \
       CRIT("No private data for object %p (%s)!", \
               o, #type);                          \
       abort();                                    \
       return;                                     \
    }

#define PRIV_GET_OR_RETURN_VAL(o, type, ptr)       \
  DATA_GET(o, type, ptr);                          \
  if (!ptr)                                        \
    {                                              \
       CRIT("No private data for object %p (%s)!", \
               o, #type);                          \
       abort();                                    \
       return val;                                 \
    }

#define FREE_NULL(p) \
        if (p) { free(p); p = NULL; }

#define FREE_NULL_FUNC(fn, p) \
        if (p) { fn(p); p = NULL; }

#define ENNA_CONFIG_FILE "enna.conf"

typedef struct _Enna Enna;
struct _Enna
{
   Evas_Coord app_x_off, app_y_off, app_w, app_h;
   const char *theme_file;
   const char *config_file;
   Eina_Bool run_fullscreen;
   Evas_Object *win, *naviframe, *mainmenu, *layout;
};

#endif /* _ENNA_PRIVATE_H_ */
