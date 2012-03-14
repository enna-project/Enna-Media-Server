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

#include <sqlite3.h>
#include <Eina.h>
#include <Ecore_File.h>

#include "ems_private.h"
#include "ems_database_statements.h"
#include "ems_database.h"
#include "ems_config.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/


#define EMS_DB_BIND_TEXT_OR_GOTO(stmt, col, value, label)               \
  do                                                                    \
    {                                                                   \
       res = sqlite3_bind_text(stmt, col, value, -1, SQLITE_STATIC);    \
       if (res != SQLITE_OK)                                            \
         goto label;                                                    \
    }                                                                   \
  while (0)
#define EMS_DB_BIND_INT_OR_GOTO(stmt, col, value, label)        \
  do                                                            \
    {                                                           \
       res = sqlite3_bind_int(stmt, col, value);                \
       if (res != SQLITE_OK)                                    \
         goto label;                                            \
    }                                                           \
  while (0)
#define EMS_DB_BIND_INT64_OR_GOTO(stmt, col, value, label)      \
  do                                                            \
    {                                                           \
       res = sqlite3_bind_int64(stmt, col, value);              \
       if (res != SQLITE_OK)                                    \
         goto label;                                            \
    }                                                           \
  while (0)

struct _Ems_Database
{
   sqlite3 *db;
   const char *filename;
   sqlite3_stmt *file_stmt;
   sqlite3_stmt *file_update_stmt;
   sqlite3_stmt *file_get_stmt;
   sqlite3_stmt *fileid_get_stmt;
   sqlite3_stmt *filemtime_stmt;
   sqlite3_stmt *data_stmt;
   sqlite3_stmt *meta_stmt;
   sqlite3_stmt *assoc_file_meta_stmt;
   sqlite3_stmt *begin_stmt;
   sqlite3_stmt *end_stmt;
};

static int64_t
_step_rowid (Ems_Database *db,
             sqlite3_stmt *stmt, int *res)
{
   int64_t val = 0;

   val = sqlite3_last_insert_rowid(db->db);
   *res = sqlite3_step (stmt);
   return val;
}

static int64_t
_data_insert(Ems_Database *db, Eina_Value *value, int64_t lang_id)
{
   int res;
   int64_t val = 0;
   char *str;

   str = eina_value_to_string(value);
   if (!str)
     return 0;

   EMS_DB_BIND_TEXT_OR_GOTO(db->data_stmt, 1, str, out);
   EMS_DB_BIND_INT64_OR_GOTO(db->data_stmt, 2, lang_id, out_clear);

   val = _step_rowid(db, db->data_stmt, &res);
   sqlite3_reset(db->data_stmt);
 out_clear:
   sqlite3_clear_bindings(db->data_stmt);
 out:
   if (res != SQLITE_CONSTRAINT && res != SQLITE_DONE) /* ignore constraint violation */
     ERR("%s", sqlite3_errmsg(db->db));
   free(str);
   return val;
}

static int64_t
_meta_insert(Ems_Database *db, const char *meta)
{
   int res;
   int64_t val = 0;

   if (!db || !db->db || !meta)
     return 0;

   EMS_DB_BIND_TEXT_OR_GOTO(db->meta_stmt, 1, meta, out);

   val = _step_rowid(db, db->meta_stmt, &res);
   sqlite3_reset(db->meta_stmt);
   sqlite3_clear_bindings(db->meta_stmt);

 out:
   if (res != SQLITE_CONSTRAINT && res != SQLITE_DONE) /* ignore constraint violation */
     ERR("%s", sqlite3_errmsg (db->db));

   return val;
}

static int64_t
_file_get(Ems_Database *db, const char *filename)
{
   int res, err = -1;
   int64_t val = 0;

   if (!db || !db->db || !filename)
     return 0;

   EMS_DB_BIND_TEXT_OR_GOTO(db->file_get_stmt, 1, filename, out);
   res = sqlite3_step(db->file_get_stmt);
   if (res == SQLITE_ROW)
     val = sqlite3_column_int64(db->file_get_stmt, 0);
   else
     goto out;
   sqlite3_reset(db->file_get_stmt);
   sqlite3_clear_bindings(db->file_get_stmt);
   err = 0;
 out:
   if (err < 0)
     ERR("%s", sqlite3_errmsg(db->db));
   return val;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

Ems_Database *
ems_database_new(const char *filename)
{
   Ems_Database *db;
   int res;
   Eina_Bool exists;

   db = calloc(1, sizeof(Ems_Database));
   if (!db)
     return NULL;

   res = sqlite3_initialize();
   if (res != SQLITE_OK)
     return NULL;

   if (!filename)
     {
        char tmp[PATH_MAX];
        snprintf(tmp, sizeof(tmp), "%s/%s", ems_config_cache_dirname_get(), EMS_DATABASE_FILE);
        db->filename = eina_stringshare_add(tmp);
     }
   else
     {
        db->filename = eina_stringshare_add(filename);
     }


   exists = ecore_file_exists(db->filename);

   INF("Database file : %s", db->filename);

   res = sqlite3_open(db->filename, &db->db);
   if (res)
     {
        ERR("Unable to open database: %s", sqlite3_errmsg(db->db));
        goto err;
     }

   /* Create table in the new database*/
   if (!exists)
     {
        INF("Creating Database table");
        ems_database_table_create(db);
     }

   return db;

 err:
   if (db)
     {
        if (db->filename) eina_stringshare_del(db->filename);
        if (db->db) sqlite3_close(db->db);
        free(db);
     }
   return NULL;
}

void
ems_database_free(Ems_Database *db)
{
    eina_stringshare_del(db->filename);
    sqlite3_close(db->db);
    free(db);
    sqlite3_shutdown();
}

void
ems_database_table_create(Ems_Database *db)
{
   char *m;

   if (!db)
     return;

   sqlite3_exec(db->db, BEGIN_TRANSACTION, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_FILE, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_META, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_DATA, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_ASSOC_FILE_METADATA, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_INDEX_ASSOC, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, END_TRANSACTION, NULL, NULL, &m);
   if (m)
     goto err;

   return;

 err:
   ERR("%s", m);

}

void
ems_database_prepare(Ems_Database *db)
{
   int res;

   if (!db || !db->db)
     return;

   res = sqlite3_prepare_v2(db->db, INSERT_FILE, -1, &db->file_stmt, NULL);
   if (res != SQLITE_OK)
     {
        ERR("%s", sqlite3_errmsg (db->db));
        return;
     }
   res = sqlite3_prepare_v2(db->db, UPDATE_FILE, -1, &db->file_update_stmt, NULL);
   if (res != SQLITE_OK)
     {
        ERR("%s", sqlite3_errmsg (db->db));
        return;
     }
   res = sqlite3_prepare_v2(db->db, SELECT_FILE_ID, -1, &db->file_get_stmt, NULL);
   if (res != SQLITE_OK)
     {
        ERR("%s", sqlite3_errmsg (db->db));
        return;
     }
   res = sqlite3_prepare_v2(db->db, SELECT_FILE_FROM_ID, -1, &db->fileid_get_stmt, NULL);
   if (res != SQLITE_OK)
     {
        ERR("%s", sqlite3_errmsg (db->db));
        return;
     }
   res = sqlite3_prepare_v2(db->db, SELECT_FILE_MTIME, -1, &db->filemtime_stmt, NULL);
   if (res != SQLITE_OK)
     {
        ERR("%s", sqlite3_errmsg (db->db));
        return;
     }
   res = sqlite3_prepare_v2(db->db, INSERT_META, -1, &db->meta_stmt, NULL);
   if (res != SQLITE_OK)
     {
        ERR("%s", sqlite3_errmsg (db->db));
        return;
     }
   res = sqlite3_prepare_v2(db->db, INSERT_DATA, -1, &db->data_stmt, NULL);
   if (res != SQLITE_OK)
     {
        ERR("%s", sqlite3_errmsg (db->db));
        return;
     }
   res = sqlite3_prepare_v2(db->db, INSERT_ASSOC_FILE_METADATA, -1, &db->assoc_file_meta_stmt, NULL);
   if (res != SQLITE_OK)
     {
        ERR("%s", sqlite3_errmsg (db->db));
        return;
     }
   res = sqlite3_prepare_v2(db->db, BEGIN_TRANSACTION, -1, &db->begin_stmt, NULL);
   if (res != SQLITE_OK)
     {
        ERR("%s", sqlite3_errmsg (db->db));
        return;
     }
   res = sqlite3_prepare_v2(db->db, END_TRANSACTION, -1, &db->end_stmt, NULL);
   if (res != SQLITE_OK)
     {
        ERR("%s", sqlite3_errmsg (db->db));
        return;
     }
}

void
ems_database_release(Ems_Database *db)
{
   if (!db || !db->db)
     return;

   sqlite3_finalize(db->file_stmt);
   sqlite3_finalize(db->file_update_stmt);
   sqlite3_finalize(db->file_get_stmt);
   sqlite3_finalize(db->data_stmt);
   sqlite3_finalize(db->meta_stmt);
   sqlite3_finalize(db->assoc_file_meta_stmt);
   sqlite3_finalize(db->filemtime_stmt);
   sqlite3_finalize(db->begin_stmt);
   sqlite3_finalize(db->end_stmt);
}

void
ems_database_file_insert(Ems_Database *db, const char *filename, int64_t mtime, Ems_Media_Type type __UNUSED__, int64_t magic)
{
   int res, err = -1;
   sqlite3_stmt *stmt = db->file_stmt;

   if (!db || !db->db || !filename)
     return;

   EMS_DB_BIND_TEXT_OR_GOTO(stmt, 1, filename,  out);
   EMS_DB_BIND_INT64_OR_GOTO(stmt, 2, mtime, out_clear);
   EMS_DB_BIND_INT64_OR_GOTO(stmt, 3, magic, out_clear);

   res = sqlite3_step (stmt);
   if (res == SQLITE_DONE)
     err = 0;

   sqlite3_reset (stmt);
 out_clear:
   sqlite3_clear_bindings (stmt);
 out:
   if (err < 0)
     ERR("%s", sqlite3_errmsg(db->db));
}

void
ems_database_file_update(Ems_Database *db, const char *filename, int64_t mtime, Ems_Media_Type type __UNUSED__, int64_t magic)
{
  int res, err = -1;

  EMS_DB_BIND_INT64_OR_GOTO(db->file_update_stmt, 1, mtime, out);
  EMS_DB_BIND_INT_OR_GOTO(db->file_update_stmt, 2, magic,  out_clear);
  EMS_DB_BIND_TEXT_OR_GOTO(db->file_update_stmt, 3, filename, out_clear);

  res = sqlite3_step(db->file_update_stmt);
  if (res == SQLITE_DONE)
    err = 0;

  sqlite3_reset(db->file_update_stmt);
 out_clear:
  sqlite3_clear_bindings(db->file_update_stmt);
 out:
  if (err < 0)
    ERR("%s", sqlite3_errmsg (db->db));
}

void
ems_database_meta_insert(Ems_Database *db, const char *filename, const char *meta, Eina_Value *value)
{
   int res = -1, err = -1;
   int64_t meta_id = 0;
   int64_t data_id = 0;
   int64_t file_id = 0;

   if (!db || !db->db || !filename || !meta || !value)
     return;


   file_id = _file_get(db, filename);
   meta_id = _meta_insert(db, meta);
   data_id = _data_insert(db, value, 0); /* TODO : handle lang correctly */



   EMS_DB_BIND_INT64_OR_GOTO(db->assoc_file_meta_stmt, 1, file_id, out);
   EMS_DB_BIND_INT64_OR_GOTO(db->assoc_file_meta_stmt, 2, meta_id, out_clear);
   EMS_DB_BIND_INT64_OR_GOTO(db->assoc_file_meta_stmt, 3, data_id, out_clear);

   res = sqlite3_step(db->assoc_file_meta_stmt);
   if (res == SQLITE_DONE)
     err = 0;
   sqlite3_reset(db->assoc_file_meta_stmt);
 out_clear:
   sqlite3_clear_bindings (db->assoc_file_meta_stmt);
 out:
   if (err < 0 && res != SQLITE_CONSTRAINT) /* ignore constraint violation */
     ERR("%s", sqlite3_errmsg(db->db));

}

void
ems_database_transaction_begin(Ems_Database *db)
{
   if (!db || !db->db)
     return;

   sqlite3_step(db->begin_stmt);
   sqlite3_reset(db->begin_stmt);
}

void
ems_database_transaction_end(Ems_Database *db)
{
   if (!db || !db->db)
     return;

   sqlite3_step(db->end_stmt);
   sqlite3_reset(db->end_stmt);
}

Eina_List *
ems_database_files_get(Ems_Database *db)
{
   Eina_List *files = NULL;
   sqlite3_stmt *stmt = NULL;
   int res;

   if (!db || !db->db)
     return NULL;

   res = sqlite3_prepare_v2 (db->db, "SELECT file_path,file_mtime FROM file;", -1, &stmt, NULL);
   if (res != SQLITE_OK)
     goto out;

   while (1)
     {

        res = sqlite3_step (stmt);
        if (res == SQLITE_ROW)
          {
             const char * text;

             text  = (const char*)sqlite3_column_text (stmt, 1);
             files = eina_list_append(files, eina_stringshare_add(text));
          }
        else if (res == SQLITE_DONE)
          {
             DBG("Request End");
             break;
          }
        else
          {
             ERR("Request failed");
          }
     }

   return files;

 out:
   return NULL;
}

const char *
ems_database_file_get(Ems_Database *db, int item_id)
{
   const char *file = NULL;
   sqlite3_stmt *stmt = NULL;
   int res = -1, err = -1;

   if (!db || !db->db)
     return NULL;

   stmt = db->fileid_get_stmt;

   EMS_DB_BIND_INT64_OR_GOTO(stmt, 1, item_id, out);

   while (1)
     {

        res = sqlite3_step (stmt);
        if (res == SQLITE_ROW)
          {
             file = eina_stringshare_add((const char *)sqlite3_column_text(stmt, 0));
          }
        else if (res == SQLITE_DONE)
          {
             err = 0;
             break;
          }
        else
          {
             err = res;
          }
     }

   sqlite3_reset (stmt);
   sqlite3_clear_bindings (stmt);
 out:
   if (err < 0)
     ERR("%s", sqlite3_errmsg(db->db));

   return file;
}

int64_t
ems_database_file_mtime_get(Ems_Database *db, const char *filename)
{
  int res, err = -1;
  int64_t val = -1;

  if (!filename || !db || !db->db)
    return -1;

  EMS_DB_BIND_TEXT_OR_GOTO(db->filemtime_stmt, 1, filename, out);

  res = sqlite3_step(db->filemtime_stmt);
  if (res == SQLITE_ROW)
    val = sqlite3_column_int64(db->filemtime_stmt, 0);

  sqlite3_reset(db->filemtime_stmt);
  sqlite3_clear_bindings(db->filemtime_stmt);
  err = 0;
 out:
  if (err < 0)
    ERR("%s", sqlite3_errmsg(db->db));
  return val;
}
