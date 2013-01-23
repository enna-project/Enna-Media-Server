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
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_Con.h>

#include "Ecore_Con_Eet.h"
#include "ems_database.h"
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
#define EMS_CONFIG_VERSION 2
#define EMS_DATABASE_VERSION "3"
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
typedef struct _Ems_Collection_Filter Ems_Collection_Filter;
typedef struct _Ems_Grabber_Data Ems_Grabber_Data;
typedef struct _Ems_Grabber_Params Ems_Grabber_Params;



/* Function typedef, called when grabber ends its work */
typedef void (*Ems_Grabber_End_Cb)(void *data,
                                   const char *filename,
                                   Ems_Grabber_Data *grabbed_data);

/* Function exported by grabber modules */
typedef void (*Ems_Grabber_Grab)(Ems_Grabber_Params *params,
				 Ems_Grabber_End_Cb end_cb,
	                         void *data);

struct _Ems_Observer
{
   int dummy;
};

struct _Ems_Collection_Filter
{
   const char *metadata;
   const char *value;
};

struct _Ems_Collection
{
   Ems_Media_Type type;
   Eina_List *filters;
};

struct _Ems_Media
{
   int dummy;
};

struct _Ems_Video
{
   const char *hash_key;
   const char *title;
   int64_t time;
};

struct _Ems_Media_Info
{
   int dummy;
};

struct _Ems_Node
{
   const char *name;
   const char *ip;
   Ecore_Con_Eet *ece;
   Ecore_Con_Reply *reply;
   unsigned int port;
   Eina_Bool is_ipv6;
   Eina_Bool is_local;
   Eina_Bool is_connected;
   const char *uuid;
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
   short port_stream;
   const char *name;
   const char *video_extensions;
   const char *music_extensions;
   const char *photo_extensions;
   const char *blacklist;
   const char *cache_path;
   Eina_List *places;
   unsigned int scan_period;
   const char *uuid;
};

extern Ems_Config *ems_config;

struct _Ems_Grabber_Data
{
   Eina_Hash *data;
   Eina_Hash *episode_data; //episode specific data
   time_t date;
   const char *lang;
};

struct _Ems_Grabber_Params
{
   const char *filename;
   const char *search;
   Ems_Media_Type type;
   //for tvshow grabbers
   unsigned int season;
   unsigned int episode;
};

#endif /* _EMS_PRIVATE_H_ */
