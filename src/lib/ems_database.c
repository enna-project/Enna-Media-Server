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

#include <sqlite3.h>
#include <Eina.h>
#include <Ecore_File.h>

#include "ems_private.h"
#include "ems_database.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

#define STMT_INSERT_FILE                        \
  "INSERT "                                     \
  "INTO file (file_path, "                      \
  "           file_mtime)"                      \
  "VALUES (?, ?);"

#define CREATE_TABLE_FILE                                       \
    "CREATE TABLE IF NOT EXISTS file ( "                        \
    "file_id          INTEGER PRIMARY KEY AUTOINCREMENT, "      \
    "file_path        TEXT    NOT NULL UNIQUE, "                \
    "file_mtime       INTEGER NOT NULL "                        \
    ");"

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

   sqlite3_exec(db->db, "BEGIN;", NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, CREATE_TABLE_FILE, NULL, NULL, &m);
   if (m)
     goto err;
   sqlite3_exec(db->db, "COMMIT;", NULL, NULL, &m);
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

   sqlite3_prepare_v2(db->db, STMT_INSERT_FILE, -1, &db->file_stmt, NULL);
   sqlite3_prepare_v2(db->db, "BEGIN;", -1, &db->begin_stmt, NULL);
   sqlite3_prepare_v2(db->db, "END;", -1, &db->end_stmt, NULL);
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
ems_database_file_insert(Ems_Database *db, const char *filename, int64_t mtime)
{
   if (!db || !db->db || !filename)
     return;

   sqlite3_bind_text(db->file_stmt, 1, filename, -1, SQLITE_STATIC);
   sqlite3_bind_int(db->file_stmt, 2, mtime);
   sqlite3_step(db->file_stmt);
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

