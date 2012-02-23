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

#ifndef _EMS_PRIVATE_H
#define _EMS_PRIVATE_H

#include <Eina.h>

/*
 * variable and macros used for the eina_log module
 */
extern int _ems_log_dom_global;

/*
 * Macros that are used everywhere
 *
 * the first four macros are the general macros for the lib
 */
#ifdef EMS_DEFAULT_LOG_COLOR
# undef EMS_DEFAULT_LOG_COLOR
#endif /* ifdef EMS_DEFAULT_LOG_COLOR */
#define EMS_DEFAULT_LOG_COLOR EINA_COLOR_CYAN
#ifdef ERR
# undef ERR
#endif /* ifdef ERR */
#define ERR(...)  EINA_LOG_DOM_ERR(_ems_log_dom_global, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif /* ifdef DBG */
#define DBG(...)  EINA_LOG_DOM_DBG(_ems_log_dom_global, __VA_ARGS__)
#ifdef INF
# undef INF
#endif /* ifdef INF */
#define INF(...)  EINA_LOG_DOM_INFO(_ems_log_dom_global, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif /* ifdef WRN */
#define WRN(...)  EINA_LOG_DOM_WARN(_ems_log_dom_global, __VA_ARGS__)
#ifdef CRIT
# undef CRIT
#endif /* ifdef CRIT */
#define CRIT(...) EINA_LOG_DOM_CRIT(_ems_log_dom_global, __VA_ARGS__)



#define EMS_CONFIG_FILE "enna-media-server.conf"
#define EMS_DEFAULT_PORT 1337
#define EMS_DEFAULT_NAME "Enna Media Server"
#define EMS_CONFIG_VERSION 1

#define EMS_SERVER_JSONRPC_API_NAME "_enna_server-jsonrpc._tcp"

#define ENNA_CONFIG_DD_NEW(str, typ)            \
  ems_config_descriptor_new(str, sizeof(typ))
#define ENNA_CONFIG_DD_FREE(eed) if (eed) { eet_data_descriptor_free((eed)); (eed) = NULL; }
#define ENNA_CONFIG_VAL(edd, type, member, dtype) EET_DATA_DESCRIPTOR_ADD_BASIC(edd, type, #member, member, dtype)
#define ENNA_CONFIG_SUB(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_SUB(edd, type, #member, member, eddtype)
#define ENNA_CONFIG_LIST(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_LIST(edd, type, #member, member, eddtype)
#define ENNA_CONFIG_HASH(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_HASH(edd, type, #member, member, eddtype)


typedef struct _Ems_Config Ems_Config;
typedef struct _Ems_Directory Ems_Directory;

struct _Ems_Observer
{
   int dummy;
};

struct _Ems_Collection
{
   int dummy;
};

struct _Ems_Media
{
   int dummy;
};

struct _Ems_Media_Info
{
   int dummy;
};

struct _Ems_Server
{
   const char *name;
   const char *ip;
   unsigned int port;
   Eina_Bool is_ipv6;
   Eina_Bool is_local;
};

enum _Ems_Media_Type
{
  EMS_MEDIA_TYPE_VIDEO = 1 << 0,
  EMS_MEDIA_TYPE_MUSIC = 1 << 1,
  EMS_MEDIA_TYPE_PHOTO = 1 << 2,
};

struct _Ems_Directory
{
   const char *path;
   const char *label;
   int type;
};

struct _Ems_Config
{
   unsigned int version;
   short port;
   const char *name;
   const char *video_extensions;
   const char *music_extensions;
   const char *photo_extensions;
   Eina_List *video_directories;
   Eina_List *tvshow_directories;
   Eina_List *music_directories;
   Eina_List *photo_directories;
   unsigned int scan_period;
};

extern Ems_Config *ems_config;

#endif /* _EMS_PRIVATE_H_ */
