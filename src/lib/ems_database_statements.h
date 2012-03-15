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

#ifndef _EMS_DATABASE_STATEMENTS_H_
#define _EMS_DATABASE_STATEMENTS_H_

/*
 * PRIi64 is replaced by I64i with inttypes.h provided by MinGW. These SQL
 * statements are built with sqlite3_snprintf() [1]. This one does not use
 * the functions from MSVC, then I64i is not known by SQLite.
 *
 * Note that %lli is supported only by sqlite3_snprintf(). With the *printf
 * functions of MSVC, %ll is considered like %l. It is for this reason that
 * MinGW uses %I64i instead of %lli in inttypes.h.
 *
 * [1]: see SQL_CONCAT() in database.c
 */
#ifdef _WIN32
#define EMS_I64 "lli"
#else /* _WIN32 */
#define EMS_I64 PRIi64
#endif /* !_WIN32 */

/******************************************************************************/
/*                                                                            */
/*                                 Controls                                   */
/*                                                                            */
/******************************************************************************/

#define BEGIN_TRANSACTION                       \
  "BEGIN;"

#define END_TRANSACTION                         \
  "COMMIT;"

/******************************************************************************/
/*                                                                            */
/*                              Create tables                                 */
/*                                                                            */
/******************************************************************************/


#define CREATE_TABLE_FILE                                       \
  "CREATE TABLE IF NOT EXISTS file ( "                          \
  "file_id          INTEGER PRIMARY KEY AUTOINCREMENT, "        \
  "file_path        TEXT    NOT NULL UNIQUE, "                  \
  "file_mtime       INTEGER NOT NULL,"                          \
  "scan_magic       INTEGER NOT NULL"                           \
  ");"

#define CREATE_TABLE_META                                       \
  "CREATE TABLE IF NOT EXISTS meta ( "                          \
  "meta_id          INTEGER PRIMARY KEY AUTOINCREMENT, "        \
  "meta_name        TEXT    NOT NULL UNIQUE "                   \
  ");"

#define CREATE_TABLE_DATA                                       \
  "CREATE TABLE IF NOT EXISTS data ( "                          \
  "data_id          INTEGER PRIMARY KEY AUTOINCREMENT, "        \
  "data_value       TEXT    NOT NULL UNIQUE, "                  \
  "_lang_id         INTEGER NULL "                              \
  ");"

#define CREATE_TABLE_ASSOC_FILE_METADATA                \
  "CREATE TABLE IF NOT EXISTS assoc_file_metadata ( "   \
  "file_id          INTEGER NOT NULL, "                 \
  "meta_id          INTEGER NOT NULL, "                 \
  "data_id          INTEGER NOT NULL, "                 \
  "PRIMARY KEY (file_id, meta_id, data_id) "            \
  ");"


/******************************************************************************/
/*                                                                            */
/*                              Create indexes                                */
/*                                                                            */
/******************************************************************************/

#define CREATE_INDEX_ASSOC                                      \
  "CREATE INDEX IF NOT EXISTS "                                 \
  "assoc_idx ON assoc_file_metadata (meta_id, data_id);"


/******************************************************************************/
/*                                                                            */
/*                                  Select                                    */
/*                                                                            */
/******************************************************************************/

#define SELECT_FILE_ID                          \
  "SELECT file_id "                             \
  "FROM file "                                  \
  "WHERE file_path = ?;"

#define SELECT_FILE_FROM_ID                     \
  "SELECT file_path "                           \
  "FROM file "                                  \
  "WHERE file_id = ?;"

#define SELECT_FILE_MTIME                       \
  "SELECT file_mtime "                          \
  "FROM file "                                  \
  "WHERE file_path = ?;"

#define SELECT_FILE_SCAN_MAGIC                  \
  "SELECT file_path,file_id "                   \
  "FROM file "                                  \
  "WHERE scan_magic <> ?;"

/******************************************************************************/
/*                                                                            */
/*                                  Insert                                    */
/*                                                                            */
/******************************************************************************/

#define INSERT_INFO                             \
  "INSERT OR REPLACE "                          \
  "INTO info (info_name, info_value) "          \
  "VALUES (?, ?);"

#define INSERT_FILE                             \
  "INSERT "                                     \
  "INTO file (file_path, "                      \
  "           file_mtime,"                      \
  "           scan_magic) "                     \
  "VALUES (?, ?, ?);"

#define INSERT_META                             \
  "INSERT "                                     \
  "INTO meta (meta_name) "                      \
  "VALUES (?);"

#define INSERT_DATA                             \
  "INSERT "                                     \
  "INTO data (data_value, "                     \
  "           _lang_id) "                       \
  "VALUES (?, ?);"

#define INSERT_ASSOC_FILE_METADATA                              \
  "INSERT "                                                     \
  "INTO assoc_file_metadata (file_id, meta_id, data_id) "       \
  "VALUES (?, ?, ?);"

/******************************************************************************/
/*                                                                            */
/*                                  Update                                    */
/*                                                                            */
/******************************************************************************/

#define UPDATE_FILE                             \
  "UPDATE file "                                \
  "SET file_mtime      = ?, "                   \
  "    scan_magic      = ? "                    \
  "WHERE file_path = ?;"

/******************************************************************************/
/*                                                                            */
/*                                  Delete                                    */
/*                                                                            */
/******************************************************************************/

#define DELETE_FILE  \
 "DELETE FROM file " \
 "WHERE file_path = ?;"



#endif /* _EMS_DATABASE_STATEMENTS_H_ */
