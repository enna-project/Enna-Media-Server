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
#include "ems_database_statements.h"
#include "ems_database.h"
#include "ems_config.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

struct _Ems_Database
{
   const char *filename;
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
   
   return NULL;
}

void
ems_database_free(Ems_Database *db)
{
  
}

void
ems_database_table_create(Ems_Database *db)
{

}

void
ems_database_prepare(Ems_Database *db)
{

}

void
ems_database_release(Ems_Database *db)
{

}

void
ems_database_file_insert(Ems_Database *db, const char *filename,
                         const char *uuid, int64_t mtime,
                         Ems_Media_Type type __UNUSED__, int64_t magic)
{

}

void
ems_database_file_update(Ems_Database *db, const char *filename, int64_t mtime, Ems_Media_Type type __UNUSED__, int64_t magic)
{

}

void
ems_database_meta_insert(Ems_Database *db, const char *filename, const char *meta, const char *value)
{

}

void
ems_database_transaction_begin(Ems_Database *db)
{

}

void
ems_database_transaction_end(Ems_Database *db)
{

}

Eina_List *
ems_database_files_get(Ems_Database *db)
{
   return NULL;
}

const char *
ems_database_file_get(Ems_Database *db, int item_id)
{
   return file;
}

const char *
ems_database_file_uuid_get(Ems_Database *db, char *media_uuid)
{
   return NULL;
}

int64_t
ems_database_file_mtime_get(Ems_Database *db, const char *filename)
{
  return -1;
}

const char *
ems_database_file_hash_get(Ems_Database *db, const char *filename)
{
   return NULL;
}

int64_t
ems_database_file_delete(Ems_Database *db, const char *filename)
{
   return -1;
}

void
ems_database_deleted_files_remove(Ems_Database *db, int64_t magic)
{
   return;
}

const char *
ems_database_uuid_get(Ems_Database *db)
{
   return NULL;
}

Eina_List *
ems_database_collection_get(Ems_Database *db, Ems_Collection *collection)
{
   return NULL;
}

const char *
ems_database_info_get(Ems_Database *db, const char *uuid, const char *metadata)
{
   return val;
}
