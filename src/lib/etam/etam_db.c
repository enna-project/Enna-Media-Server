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


#include "Etam.h"

#include "etam_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ETAM_URI_PREFIX "etam://"

struct _Etam_Db
{
   const char *file;
   Eina_List *callbacks;
   Eina_Bool is_local;
   Eet_File *ef;
};

struct _Etam_Db_Callback
{
   func;
   void *data;
   Etam_DB_Op type;
};

void _callback_call(Etam_Db *db, Etam_DB_Op type)
{
   Etam_Db_Callback *cb;
   Eina_List *l;

   if (!db)
     return;

   EINA_LIST_FOREACH(l, db->callbacks, cb)
     {
        if ((type == cb->type) && cb->func)
          {
             cb->func(cb->data);
          }
     }
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

Etam_DB *etam_db_new(const char *uri)
{
   Etam_DB *db;

   db = calloc(1, sizeof(Etam_Db));
   if (!db)
     return NULL;

   db->uri = eina_stringshare_add(uri);

   is_local = !strstr(uri, ETAM_URI_PREFIX);

   return db;
}

void etam_db_del(Etam_Db *db)
{
   if (!db)
     return;

   if (db->is_local)
     {
        eet_close(db->ef);
     }
   else
     {
        ERR("TODO: implement eet_connexion");
     }

   EINA_LIST_FOREACH(l, db->callbacks, cb)
     {
        _callback_call(db, ETAM_DB_CLOSE);
        db->callbacks = eina_list_remove(db->callbacks, cb);
        free(cb);
     }

   eina_stringshare_del(db->uri);
   free(db);
}

void etam_db_open(Etam_Db *db)
{
   if (!db)
     return;

   if (db->is_local)
     {
        db->ef = eet_open(db->uri, EET_FILE_MODE_WRITE);
        if (!ef)
          {
             _callback_call(db, ETAM_DB_ERROR);
             return;
          }
        else
          {
             _callback_call(db, ETAM_OPEN);
          }
     }
   else
     {
        ERR("TODO: implement eet_connexion");
     }
}

void etam_db_signal_callback_add(Etam_DB *db, Etam_DB_Op type, func, data)
{
   Etam_Db_Callback *cb;

   if (!func || !db)
     return;

   cb->func = func;
   cb->type = type;
   cb->data = data;
   db->callbacks = eina_list_append(db->callbacks, cb);
}

void etam_db_signal_callback_del(Etam_DB *db, func)
{
   Etam_Db_Callback *cb;
   Eina_List *l;

   if (!func || !db)
     return;

   EINA_LIST_FOREACH(l, db->callbacks, cb)
     {
        if (func == cb->func)
          {
             db->callbacks = eina_list_remove(db->callbacks, cb);
             free(cb);
          }
     }
}
