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

#include <uuid/uuid.h>

#include <Eina.h>
#include <Ecore_File.h>

#include "ems_private.h"
#include "ems_database.h"
#include "ems_config.h"

#define EMS_DATABASE_FILE "database.eet"
#define DATABASES_SECTION "databases"
#define VIDEOS_HASH_SECTION "videos_hash"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/


typedef struct _Ems_Db_Databases_Item Ems_Db_Databases_Item;
typedef struct _Ems_Db_Databases Ems_Db_Databases;
typedef struct _Ems_Db Ems_Db;
typedef struct _Ems_Db_Places Ems_Db_Places;
typedef struct _Ems_Db_Places_Item Ems_Db_Places_Item;
typedef struct _Ems_Db_Place_Videos Ems_Db_Place_Videos;
typedef struct _Ems_Db_Video_Item Ems_Db_Video_Item;
typedef struct _Ems_Db_Videos Ems_Db_Videos;
typedef struct _Ems_Db_Video_Infos Ems_Db_Video_Infos;
typedef struct _Ems_Db_Videos_Hash Ems_Db_Videos_Hash;
typedef struct _Ems_Db_Metadata Ems_Db_Metadata;

struct _Ems_Db_Metadata
{
   const char *value;
};

struct _Ems_Db
{
   const char *filename;
   Eet_File *ef; /* The Eet file */
   Ems_Db_Databases *databases; /* List of all database known*/
   Eina_List *video_places; /* List containing section of each video place */
   Ems_Db_Videos_Hash *videos_hash; /* One big hash table to store all video medias informations */
};

struct _Ems_Db_Databases
{
   Eina_List *list;
};

struct _Ems_Db_Databases_Item
{
   const char *uuid;
   Eina_Bool is_local;
   Eina_List *places;
};

struct _Ems_Db_Places
{
   int rev;
   Eina_List *list;
};

struct _Ems_Db_Places_Item
{
   const char *path;
   const char *label;
   int type;
   int video_rev;
   Eina_List *video_list;
};

struct _Ems_Db_Place_Videos
{
   int rev;
   Eina_List *list;
};

struct _Ems_Db_Video_Item
{
   const char *hash_key;
   const char *title;
};

struct _Ems_Db_Videos_Hash
{
   Eina_Hash *hash;
};

struct _Ems_Db_Video_Infos
{
   int rev;
   uint64_t mtime;
   double start_time;
   Eina_Hash *metadatas;
};

static Ems_Db *_db;
static int _ems_init_count = 0;

static Eet_Data_Descriptor *_edd_databases;
static Eet_Data_Descriptor *_edd_databases_item;
static Eet_Data_Descriptor *_edd_places_item;
static Eet_Data_Descriptor *_edd_video_item;
static Eet_Data_Descriptor *_edd_video_infos;
static Eet_Data_Descriptor *_edd_videos_hash;
static Eet_Data_Descriptor *_edd_metadata;


static void
_init_edd(void)
{
   Eet_Data_Descriptor_Class eddc;
   Eet_Data_Descriptor *edd;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Video_Item);
   edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Video_Item, "hash_key", hash_key, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Video_Item, "title", title, EET_T_STRING);

   _edd_video_item = edd;
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Places_Item);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Places_Item, "path", path, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Places_Item, "label", label, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Places_Item, "type", type, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Places_Item, "videos_rev", video_rev, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Ems_Db_Places_Item, "video_list", video_list, _edd_video_item);

   _edd_places_item = edd;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Databases_Item);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Databases_Item, "uuid", uuid, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Databases_Item, "is_local", is_local, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Ems_Db_Databases_Item, "places", places, _edd_places_item);

   _edd_databases_item = edd;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Databases);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Ems_Db_Databases, "list", list, _edd_databases_item);

   _edd_databases = edd;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Metadata);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Metadata, "value", value, EET_T_STRING);

   _edd_metadata = edd;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Video_Infos);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Video_Infos, "rev", rev, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Video_Infos, "mtime", mtime, EET_T_LONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Video_Infos, "start_time", mtime, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_HASH(edd, Ems_Db_Video_Infos, "metadatas", metadatas, _edd_metadata);

   _edd_video_infos = edd;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Videos_Hash);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_HASH(edd, Ems_Db_Videos_Hash, "hash", hash, _edd_video_infos);

   _edd_videos_hash = edd;

}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

int
ems_database_init(void)
{
   char path[PATH_MAX];
   Eina_Bool exists = EINA_FALSE;

   if (++_ems_init_count != 1)
     return _ems_init_count;

   _db = calloc(1, sizeof(Ems_Db));

   snprintf(path, sizeof(path), "%s/"EMS_DATABASE_FILE, ems_config_cache_dirname_get());
   _db->filename = eina_stringshare_add(path);
   DBG("Try to load database stored in%s", path);
   if (ecore_file_exists(path))
     {
        DBG("Database exists");
        exists = EINA_TRUE;
     }
   else
     {
        DBG("Database doesn't exists");
        exists = EINA_FALSE;
     }

   _db->ef = eet_open(path, EET_FILE_MODE_READ_WRITE);
   if (!_db->ef)
     {
        /* FIXME : is this necessary ? */
        _db->ef = eet_open(path, EET_FILE_MODE_WRITE);
        if (!_db->ef)
          return 0;
     }

   _init_edd();

   /* Database already exists, found the local one */
   if (exists)
     {
        _db->databases = eet_data_read(_db->ef, _edd_databases, DATABASES_SECTION);
        DBG("Nb databases : %d", eina_list_count(_db->databases->list));

        _db->videos_hash = eet_data_read(_db->ef, _edd_videos_hash, VIDEOS_HASH_SECTION);
        if (!_db->videos_hash->hash)
          _db->videos_hash->hash = eina_hash_string_superfast_new(NULL);
     }
   /* Database is virgin, create the section 'databases', and add our local database */
   else
     {
        Ems_Db_Databases_Item *db;
        Ems_Db_Places_Item *place_item;
        Ems_Directory *dir;
        Eina_List *l;
        char uuid[37];
        uuid_t u ;

        /* Create Section Databases index*/

        _db->databases = calloc(1, sizeof(Ems_Db_Databases));
        uuid_generate(u);
        uuid_unparse(u, uuid);
        INF("Generate UUID for database : %s", uuid);

        db = calloc(1, sizeof (Ems_Db_Databases_Item));
        db->uuid = eina_stringshare_add(uuid);
        db->is_local = EINA_TRUE;
        /* For each place, create the media structure */
        EINA_LIST_FOREACH(ems_config->places, l, dir)
          {
             place_item = calloc(1, sizeof(Ems_Db_Places_Item));
             place_item->path = eina_stringshare_add(dir->path);
             place_item->label = eina_stringshare_add(dir->label);
             place_item->type = dir->type;
             place_item->video_rev = 1;
             db->places = eina_list_append(db->places, place_item);
          }
        _db->databases->list = eina_list_append(_db->databases->list, db);
        eet_data_write(_db->ef, _edd_databases, DATABASES_SECTION, _db->databases, EINA_TRUE);

        _db->videos_hash = calloc(1, sizeof(Ems_Db_Videos_Hash));
        _db->videos_hash->hash = eina_hash_string_superfast_new(NULL);

        eet_data_write(_db->ef, _edd_videos_hash, VIDEOS_HASH_SECTION, _db->videos_hash, EINA_TRUE);

        eet_sync(_db->ef);
     }

   return _ems_init_count;
}

int
ems_database_shutdown(void)
{
   if (--_ems_init_count != 0)
     return _ems_init_count;

   ems_database_flush();

   eina_stringshare_del(_db->filename);
   eet_close(_db->ef);

   free(_db);
   _db = NULL;

   return _ems_init_count;
}

/* void */
/* ems_database_table_create(Ems_Database *db) */
/* { */

/* } */

/* void */
/* ems_database_prepare(Ems_Database *db) */
/* { */

/* } */

/* void */
/* ems_database_release(Ems_Database *db) */
/* { */

/* } */

/* Insert a new file into db it it does not exist or update it otherwise */
void
ems_database_file_insert(const char *hash, const char *place, const char *title, int64_t mtime, double start_time)
{
   Ems_Db_Video_Infos *info;
   Ems_Db_Databases_Item *db;
   Ems_Db_Places_Item *item;
   Ems_Db_Video_Item *video_item;
   Eina_List *l1, *l2;

   if (!hash || !place || !_db)
     return;

   info = eina_hash_find(_db->videos_hash->hash, hash);
   if (!info)
     {
        Ems_Db_Metadata *meta;
        /* This file doesn't exist in database, add it */
        info = calloc(1, sizeof(Ems_Db_Video_Infos));
        info->rev = 1;
        info->mtime = mtime;
        info->start_time = start_time;
        info->metadatas = eina_hash_string_superfast_new(NULL);
        meta = calloc(1, sizeof(Ems_Db_Metadata));
        meta->value = eina_stringshare_add(title);
        eina_hash_add(info->metadatas, "title", meta);
     }
   else
     info->rev++;

   info->mtime = mtime;
   info->start_time = start_time;

   eina_hash_add(_db->videos_hash->hash, hash, info);

   /* Search for the right place to add the file*/
   EINA_LIST_FOREACH(_db->databases->list, l1, db)
     {
        EINA_LIST_FOREACH(db->places, l2, item)
          {
             if (!strcmp(place, item->label))
               {
                  video_item = calloc(1, sizeof(Ems_Db_Video_Item));
                  video_item->hash_key = eina_stringshare_add(hash);
                  video_item->title = eina_stringshare_add(title);
                  item->video_list = eina_list_append(item->video_list, video_item);
                  return;
               }
          }
     }


}

void
ems_database_flush(void)
{
   if (!_db)
     return;

   eet_data_write(_db->ef, _edd_databases, DATABASES_SECTION, _db->databases, EINA_TRUE);
   eet_data_write(_db->ef, _edd_videos_hash, VIDEOS_HASH_SECTION, _db->videos_hash, EINA_FALSE);
   eet_sync(_db->ef);
}

/* void */
/* ems_database_file_update(Ems_Database *db, const char *filename, int64_t mtime, Ems_Media_Type type __UNUSED__, int64_t magic) */
/* { */

/* } */

void
ems_database_meta_insert(const char *hash, const char *meta, const char *value)
{
   Ems_Db_Video_Infos *info;
   Ems_Db_Metadata *m;

   if (!hash || !meta || !value)
     return;

   info = eina_hash_find(_db->videos_hash->hash, hash);
   if (info)
     {
        if (!info->metadatas)
          info->metadatas = eina_hash_string_superfast_new(NULL);
        m = calloc(1, sizeof(Ems_Db_Metadata));
        m->value = eina_stringshare_add(value);
        eina_hash_add(info->metadatas, "title", m);
     }
   else
     ERR("I can't found %s in the database", hash);
}

/* void */
/* ems_database_transaction_begin(Ems_Database *db) */
/* { */

/* } */

/* void */
/* ems_database_transaction_end(Ems_Database *db) */
/* { */

/* } */

/* Eina_List * */
/* ems_database_files_get(Ems_Database *db) */
/* { */
/*    return NULL; */
/* } */

/* const char * */
/* ems_database_file_get(Ems_Database *db, int item_id) */
/* { */
/*    return file; */
/* } */

/* const char * */
/* ems_database_file_uuid_get(Ems_Database *db, char *media_uuid) */
/* { */
/*    return NULL; */
/* } */

int64_t
ems_database_file_mtime_get(const char *hash)
{
   Ems_Db_Video_Infos *it;

   if (!hash || !_db)
     return -1;

   it = eina_hash_find(_db->videos_hash->hash, hash);
   if (it)
        return it->mtime;
   else
        return -1;
  return -1;
}

/* const char * */
/* ems_database_file_hash_get(Ems_Database *db, const char *filename) */
/* { */
/*    return NULL; */
/* } */

/* int64_t */
/* ems_database_file_delete(Ems_Database *db, const char *filename) */
/* { */
/*    return -1; */
/* } */

/* void */
/* ems_database_deleted_files_remove(Ems_Database *db, int64_t magic) */
/* { */
/*    return; */
/* } */

/* const char * */
/* ems_database_uuid_get(Ems_Database *db) */
/* { */
/*    return NULL; */
/* } */

/* Eina_List * */
/* ems_database_collection_get(Ems_Database *db, Ems_Collection *collection) */
/* { */
/*    return NULL; */
/* } */

/* const char * */
/* ems_database_info_get(Ems_Database *db, const char *uuid, const char *metadata) */
/* { */
/*    return val; */
/* } */
