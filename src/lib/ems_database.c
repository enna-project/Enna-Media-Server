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

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

struct _Ems_Database
{
   sqlite3 *db;
   const char *filename;
   sqlite3_stmt *file_stmt;
   sqlite3_stmt *begin_stmt;
   sqlite3_stmt *end_stmt;
};

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

   if (!filename)
     return NULL;

   db = calloc(1, sizeof(Ems_Database));
   if (!db)
     return NULL;

   res = sqlite3_initialize();
   if (res != SQLITE_OK)
     return NULL;

   exists = ecore_file_exists(filename);

   res = sqlite3_open(filename, &db->db);
   if (res)
     {
        ERR("Unable to open database: %s", sqlite3_errmsg(db->db));
        goto err;
     }

   db->filename = eina_stringshare_add(filename);

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
ems_database_table_create(Ems_Database *db)
{
   char *m;

   if (!db)
     return;

   sqlite3_exec(db->db, BEGIN_TRANSACTION, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_INFO, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_FILE, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_TYPE, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_META, NULL, NULL, &m);
   if (m)
       goto err;
   sqlite3_exec(db->db, CREATE_TABLE_DATA, NULL, NULL, &m);
   if (m)
       goto err;
   sqlite3_exec(db->db, CREATE_TABLE_LANG, NULL, NULL, &m);
   if (m)
       goto err;
   sqlite3_exec(db->db, CREATE_TABLE_GRABBER, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_DLCONTEXT, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_ASSOC_FILE_METADATA, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_ASSOC_FILE_GRABBER, NULL, NULL, &m);
   if (m)
       goto err;
   sqlite3_exec(db->db, CREATE_INDEX_CHECKED, NULL, NULL, &m);
   if (m)
       goto err;
   sqlite3_exec(db->db, CREATE_INDEX_INTERRUPTED, NULL, NULL, &m);
   if (m)
       goto err;
   sqlite3_exec(db->db, CREATE_INDEX_OUTOFPATH, NULL, NULL, &m);
   if (m)
       goto err;
   sqlite3_exec(db->db, CREATE_INDEX_ASSOC, NULL, NULL, &m);
   if (m)
       goto err;
   sqlite3_exec(db->db, CREATE_INDEX_FK_FILE, NULL, NULL, &m);
   if (m)
       goto err;
   sqlite3_exec(db->db, CREATE_INDEX_FK_ASSOC, NULL, NULL, &m);
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
   if (!db || !db->db)
     return;

   sqlite3_prepare_v2(db->db, INSERT_FILE, -1, &db->file_stmt, NULL);
   sqlite3_prepare_v2(db->db, BEGIN_TRANSACTION, -1, &db->begin_stmt, NULL);
   sqlite3_prepare_v2(db->db, END_TRANSACTION, -1, &db->end_stmt, NULL);
}

void
ems_database_release(Ems_Database *db)
{
   if (!db || !db->db)
     return;

   sqlite3_finalize(db->file_stmt);
   sqlite3_finalize(db->begin_stmt);
   sqlite3_finalize(db->end_stmt);
}

void
ems_database_file_insert(Ems_Database *db, const char *filename, int64_t mtime, Ems_Media_Type type __UNUSED__)
{
   if (!db || !db->db || !filename)
     return;

   sqlite3_bind_text(db->file_stmt, 1, filename, -1, SQLITE_STATIC);
   sqlite3_bind_int(db->file_stmt, 2, mtime);
   if (sqlite3_step(db->file_stmt) != SQLITE_DONE)
     ERR("SQLite error: %s", sqlite3_errmsg(db->db));
   sqlite3_reset(db->file_stmt);
   sqlite3_clear_bindings(db->file_stmt);
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
