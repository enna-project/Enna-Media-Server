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
#define INFOS_SECTION     "infos"
#define DATABASES_SECTION "databases"
#define METADATAS_SECTION "metadatas"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

typedef struct _Ems_Db Ems_Db;
typedef struct _Ems_Db_Infos Ems_Db_Infos;
typedef struct _Ems_Db_Place Ems_Db_Place;
typedef struct _Ems_Db_Media Ems_Db_Media;
typedef struct _Ems_Db_Metadatas Ems_Db_Metadatas;
typedef struct _Ems_Db_Metadata Ems_Db_Metadata;
typedef struct _Ems_Db_Metadatas_Cont Ems_Db_Metadatas_Cont;
typedef struct _Ems_Db_Databases_Cont Ems_Db_Databases_Cont;

typedef struct _Ems_Db_Database_Req Ems_Db_Database_Req;

Eet_Data_Descriptor *ems_edd_infos;
Eet_Data_Descriptor *ems_edd_databases_cont;
Eet_Data_Descriptor *ems_edd_metadatas_cont;
Eet_Data_Descriptor *ems_edd_metadatas;
Eet_Data_Descriptor *ems_edd_metadata;
Eet_Data_Descriptor *ems_edd_media;
Eet_Data_Descriptor *ems_edd_place;
Eet_Data_Descriptor *ems_edd_database;

Eet_Data_Descriptor *ems_edd_database_req;

struct _Ems_Db_Infos
{
   const char *uuid;
};

struct _Ems_Db_Database
{
   const char  *uuid;
   Eina_List   *places;
};

struct _Ems_Db_Place
{
   const char  *path;
   const char  *label;
   int          type;
   Eina_List   *medias;
};

struct _Ems_Db_Media
{
   const char  *sha1;
   const char  *path;
   int64_t      time;
};

struct _Ems_Db_Metadatas
{
   const char  *sha1;
   int          rev;
   Eina_Hash   *metadatas;
};

struct _Ems_Db_Metadata
{
   const char  *meta;
   const char  *value;
};

struct _Ems_Db_Databases_Cont
{
   Eina_List *list;
};

struct _Ems_Db_Metadatas_Cont
{
   Eina_Hash *hash;
};

struct _Ems_Db
{
   const char  *filename;
   Eet_File    *ef; /* The Eet file */
   Ems_Db_Databases_Cont   *databases; /* List of all database known*/
   Ems_Db_Metadatas_Cont   *metadatas; /* One big hash table to store all video medias informations */
   Ems_Db_Infos  *infos;
   Eina_Lock    mutex; /* Mutex to lock/unlock access to the database, and let the use of db in multiple threads */
};

static Ems_Db *_db;
static int _ems_init_count = 0;

static void
_init_edd(void)
{
   Eet_Data_Descriptor_Class eddc;
   Eet_Data_Descriptor *edd;


   /* Data Descriptor for medias */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Media);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Media, "sha1", sha1, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Media, "path", path, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Media, "time", time, EET_T_LONG_LONG);

   ems_edd_media = edd;

   /* Data Descriptor for place */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Place);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Place, "path", path, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Place, "label", label, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Place, "type", type, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Ems_Db_Place, "medias", medias, ems_edd_media);
   ems_edd_place = edd;

   /* Data Descriptor for database */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Database);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Database, "uuid", uuid, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Ems_Db_Database, "places", places, ems_edd_place);

   ems_edd_database = edd;

   /* Data Descriptor for list of medias */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Databases_Cont);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Ems_Db_Databases_Cont, "list", list, ems_edd_database);

   ems_edd_databases_cont = edd;

   /* Data Descritptors for a metadata item */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Metadata);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Metadata, "meta", meta, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Metadata, "value", value, EET_T_STRING);

   ems_edd_metadata = edd;

   /* Data Descritptors for Content of Metadatas Hash */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Metadatas);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Metadatas, "sha1", sha1, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Metadatas, "rev", rev, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_HASH(edd, Ems_Db_Metadatas, "metadatas", metadatas, ems_edd_metadata);

   ems_edd_metadatas = edd;

   /* Data Descritptors for Metadatas Hash */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Metadatas_Cont);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_HASH(edd, Ems_Db_Metadatas_Cont, "hash", hash, ems_edd_metadatas);

   ems_edd_metadatas_cont = edd;

   /* Data Descriptors for Database Infos */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Db_Infos);
   edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Ems_Db_Infos, "uuid", uuid, EET_T_STRING);

   ems_edd_infos = edd;

}

static void
_metadatas_cont_hash_free(void *data)
{
   Ems_Db_Metadatas *meta = data;

   if (!meta)
     return;

   eina_stringshare_del(meta->sha1);
   eina_hash_free(meta->metadatas);
   free(meta);
}

static void
_metadata_hash_free(void *data)
{
   Ems_Db_Metadata *meta = data;

   if (!meta)
     return;


   eina_stringshare_del(meta->meta);
   eina_stringshare_del(meta->value);
   free(meta);
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
   char uuid[37];
   uuid_t u ;

   if (++_ems_init_count != 1)
     return _ems_init_count;

   DBG("Database init");

   _db = calloc(1, sizeof(Ems_Db));

   eina_lock_new(&_db->mutex);

   /* Uuid is not set in the config file, set it and save config file */
   if (!ems_config->uuid)
     {
        uuid_generate(u);
        uuid_unparse(u, uuid);
        INF("Generate UUID for database : %s", uuid);
        ems_config->uuid = eina_stringshare_add(uuid);
        ems_config_save();
     }

   snprintf(path, sizeof(path), "%s/"EMS_DATABASE_FILE, ems_config_cache_dirname_get());
   _db->filename = eina_stringshare_add(path);
   DBG("Try to load database stored in %s", path);
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
        _db->databases = eet_data_read(_db->ef, ems_edd_databases_cont, DATABASES_SECTION);
        if (!_db->databases)
          _db->databases = calloc(1, sizeof(Ems_Db_Databases_Cont));
        DBG("Nb databases : %d", eina_list_count(_db->databases->list));

        _db->metadatas = eet_data_read(_db->ef, ems_edd_metadatas_cont, METADATAS_SECTION);
        if (!_db->metadatas)
          {
             _db->metadatas = calloc(1, sizeof(Ems_Db_Metadatas_Cont));
             _db->metadatas->hash = eina_hash_string_superfast_new(_metadatas_cont_hash_free);
          }
        _db->infos = eet_data_read(_db->ef, ems_edd_infos, INFOS_SECTION);
        if (!_db->infos)
          {
             /* Can't read uuid, should not happen */
             ERR("Can't read database informations\n");
          }
     }
   /* Database is virgin, create the section 'databases', and add our local database */
   else
     {
        Ems_Db_Database *db;
        Ems_Db_Place *place;
        Ems_Directory *dir;
        Eina_List *l;


        /* Create Section Databases index */
        _db->databases = calloc(1, sizeof(Ems_Db_Databases_Cont));
        _db->metadatas = calloc(1, sizeof(Ems_Db_Metadatas_Cont));
        _db->metadatas->hash = eina_hash_string_superfast_new(_metadatas_cont_hash_free);

       

        _db->infos = calloc(1, sizeof(Ems_Db_Infos));
        _db->infos->uuid = eina_stringshare_add(uuid);

        db = calloc(1, sizeof (Ems_Db_Database));
        db->uuid = eina_stringshare_add(uuid);
        /* For each place, create the media structure */
        EINA_LIST_FOREACH(ems_config->places, l, dir)
          {
             place = calloc(1, sizeof(Ems_Db_Place));
             place->path = eina_stringshare_add(dir->path);
             place->label = eina_stringshare_add(dir->label);
             place->type = dir->type;
             db->places = eina_list_append(db->places, place);
          }
        _db->databases->list = eina_list_append(_db->databases->list, db);

        eet_data_write(_db->ef, ems_edd_databases_cont, DATABASES_SECTION, _db->databases, EINA_FALSE);
        eet_data_write(_db->ef, ems_edd_metadatas_cont, METADATAS_SECTION, _db->metadatas, EINA_FALSE);
        eet_data_write(_db->ef, ems_edd_infos, INFOS_SECTION, _db->infos, EINA_FALSE);
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

   eina_list_free(_db->databases->list);
   free(_db->databases);
   eina_hash_free(_db->metadatas->hash);
   free(_db->metadatas);

   eina_lock_free(&_db->mutex);

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
ems_database_file_insert(const char *hash, const char *place, const char *title, int64_t mtime, int64_t time)
{
   Ems_Db_Metadatas *infos;
   Ems_Db_Database *db;
   Ems_Db_Place *item;
   Ems_Video *video_item;
   Eina_List *l1, *l2, *l3;

   if (!hash || !place || !_db)
     return;

   /* db lock */
   eina_lock_take(&_db->mutex);

   infos = eina_hash_find(_db->metadatas->hash, hash);

   if (!infos)
     {
        Ems_Db_Metadata *meta;

        /* This file doesn't exist in database, add it */
        infos = calloc(1, sizeof(Ems_Db_Metadatas));
        infos->rev = 1;
        infos->metadatas = eina_hash_string_superfast_new(_metadata_hash_free);
        meta = calloc(1, sizeof(Ems_Db_Metadata));
        meta->value = eina_stringshare_add(title);
        meta = eina_hash_set(infos->metadatas, "title", meta);
     }
   else
     infos->rev++;

   eina_hash_set(_db->metadatas->hash, hash, infos);

   /* Search for the right place to add the file*/
   EINA_LIST_FOREACH(_db->databases->list, l1, db)
     {
        EINA_LIST_FOREACH(db->places, l2, item)
          {
             /* Play only with files of the right place */
             if (!strcmp(place, item->label))
               {
                  /* Search if the file exists, if yes update the scann time and return */
                  EINA_LIST_FOREACH(item->medias, l3, video_item)
                    if (!strcmp(video_item->hash_key, hash))
                      {
                         video_item->time = time;
                         eina_lock_release(&_db->mutex);
                         /* db unlock */
                         return;
                      }
                  /* File does not exist, add it to the list */
                  video_item = calloc(1, sizeof(Ems_Video));
                  video_item->hash_key = eina_stringshare_add(hash);
                  video_item->title = eina_stringshare_add(title);
                  video_item->time = time;
                  item->medias = eina_list_append(item->medias, video_item);
                  eina_lock_release(&_db->mutex);
                  /* db unlock */
                  return;
               }
          }
     }
   eina_lock_release(&_db->mutex);
   /* db unlock */
}

void
ems_database_flush(void)
{
   if (!_db)
     return;

   /* db lock */
   eina_lock_take(&_db->mutex);
   eet_data_write(_db->ef, ems_edd_databases_cont, DATABASES_SECTION, _db->databases, EINA_FALSE);
   eet_data_write(_db->ef, ems_edd_metadatas_cont, METADATAS_SECTION, _db->metadatas, EINA_FALSE);
   eet_data_write(_db->ef, ems_edd_infos, INFOS_SECTION, _db->infos, EINA_FALSE);
   eet_sync(_db->ef);
   eina_lock_release(&_db->mutex);
   /* db unlock */
}


/* void */
/* ems_database_file_update(Ems_Database *db, const char *filename, int64_t mtime, Ems_Media_Type type __UNUSED__, int64_t magic) */
/* { */

/* } */

void
ems_database_meta_insert(const char *hash, const char *meta, const char *value)
{
   Ems_Db_Metadatas *infos;

   DBG(" %s %s %s", hash, meta, value);

   if (!hash || !meta || !value)
     return;

   /* db lock */
   eina_lock_take(&_db->mutex);
   infos = eina_hash_find(_db->metadatas->hash, hash);
   printf("search %s %s\n", meta, value);
   if (infos)
     {
        Ems_Db_Metadata *m;

        if (!infos->metadatas)
          infos->metadatas = eina_hash_string_superfast_new(_metadata_hash_free);
        m = eina_hash_find(infos->metadatas, meta);
        if (!m)
          m = calloc(1, sizeof(Ems_Db_Metadata));
        m->value = eina_stringshare_add(value);
        m->meta = eina_stringshare_add(meta);
        printf("Add %s %s\n", meta, value);
        m = eina_hash_set(infos->metadatas, meta, m);
     }
   else
     ERR("I can't found %s in the database", hash);
   eina_lock_release(&_db->mutex);
   /* db unlock */
}

Eina_List *
ems_database_files_get(Ems_Db_Database *db)
{
   Eina_List *l1, *l2;
   Eina_List *ret = NULL;
   Ems_Db_Place *item;
   Ems_Video *video_item;

   /* db lock */
   eina_lock_take(&_db->mutex);

   EINA_LIST_FOREACH(db->places, l1, item)
   {
       EINA_LIST_FOREACH(item->medias, l2, video_item)
       {
           ret = eina_list_append(ret,  video_item);
       }
   }

   eina_lock_release(&_db->mutex);
   /* db unlock */
   return ret;
}

/* const char * */
/* ems_database_file_get(Ems_Database *db, int item_id) */
/* { */
/*    return file; */
/* } */

const char *
ems_database_file_uuid_get(char *filename)
{
   Eina_List *l1, *l2, *l3;
   Ems_Db_Database *db;
   Ems_Db_Place *item;
   Ems_Video *video_item;

   /* db lock */
   eina_lock_take(&_db->mutex);

   EINA_LIST_FOREACH(_db->databases->list, l1, db)
     {
        EINA_LIST_FOREACH(db->places, l2, item)
          {
             /* Search if the file exists, if yes update the scann time and return */
             EINA_LIST_FOREACH(item->medias, l3, video_item)
               if (!strcmp(video_item->hash_key, filename))
                 {
                    eina_lock_release(&_db->mutex);
                    /* db unlock */
                    return eina_stringshare_add(video_item->title);
                 }
          }
     }
   eina_lock_release(&_db->mutex);
   /* db unlock */
   return NULL;
}

int64_t
ems_database_file_mtime_get(const char *hash)
{
   Ems_Db_Metadatas *it;

   if (!hash || !_db)
     return -1;

   /* db lock */
   eina_lock_take(&_db->mutex);
   it = eina_hash_find(_db->metadatas->hash, hash);
   eina_lock_release(&_db->mutex);
   /* db unlock */

   if (it)
        return -1;
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

void
ems_database_deleted_files_remove(int64_t time, const char *place)
{
   Ems_Db_Database *db;
   Ems_Db_Place *item;
   Eina_List *l1, *l2, *l3;

   /* db lock */
   eina_lock_take(&_db->mutex);
   /* Search for the right place to add the file*/
   EINA_LIST_FOREACH(_db->databases->list, l1, db)
     {
        EINA_LIST_FOREACH(db->places, l2, item)
          {
             if (!strcmp(place, item->label))
               {
                  Ems_Video *video_item;
                  EINA_LIST_FOREACH(item->medias, l3, video_item)
                    {
                       if (time != video_item->time)
                         {
                            INF("This file %s has been remove from disk (%lld / %lld)", video_item->title, time, video_item->time);
                            item->medias = eina_list_remove(item->medias, video_item);
                            eina_stringshare_del(video_item->hash_key);
                            eina_stringshare_del(video_item->title);
                            free(video_item);
                         }
                    }
               }
          }
     }
   eina_lock_release(&_db->mutex);
   /* db unlock */
}

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

const char *
ems_database_info_get(const char *sha1, const char *meta)
{
   Ems_Db_Metadatas *infos;

   if (!sha1 || !meta)
     return NULL;

   /* db lock */
   eina_lock_take(&_db->mutex);
   
   infos = eina_hash_find(_db->metadatas->hash, sha1);
   if (infos)
     {
        Ems_Db_Metadata *m;

        m = eina_hash_find(infos->metadatas, meta);
        if (m)
          {
             printf("Return %s\n", m->value);
             eina_lock_release(&_db->mutex);
        /* db unlock */
             return m->value;
          }
        else
          {
             eina_lock_release(&_db->mutex);
             return NULL;
          }
     }
   else
     ERR("I can't found %s in the database", sha1);
   eina_lock_release(&_db->mutex);
   /* db unlock */
   return NULL;
}

const char *
ems_database_uuid_get(void)
{
   if (!_db && !_db->infos)
     return NULL;

   return _db->infos->uuid;
}

Ems_Db_Database *
ems_database_get(const char *uuid)
{
    Ems_Db_Database *db = NULL;
    Eina_List *l;

    /* db lock */
    eina_lock_take(&_db->mutex);
    /* Search for the right place to add the file*/
    EINA_LIST_FOREACH(_db->databases->list, l, db)
    {
        if (!strcmp(uuid, db->uuid))
        {
            eina_lock_release(&_db->mutex);
            return db;
        }
    }
    eina_lock_release(&_db->mutex);
    return NULL;
}
