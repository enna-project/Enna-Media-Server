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

#include <Eina.h>

#include "Ems.h"
#include "ems_private.h"
#include "ems_collection.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static Eina_List *_collections = NULL;

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

Ems_Collection *
ems_collection_new(Ems_Media_Type type, const char *arg, ...)
{
    va_list ap;
    char *metadata;
    char *value;
    Eina_Bool end = EINA_FALSE;
    Ems_Collection *collection;
    Ems_Collection_Filter *filter;

    if (!arg)
        return NULL;

    collection = calloc(1, sizeof(Ems_Collection));

    va_start(ap, arg);
    filter = calloc(1, sizeof(Ems_Collection_Filter));
    DBG("Create new collection");
    filter->metadata = arg;
    filter->value = va_arg(ap, char *);
    DBG("Create new filter %s %s", filter->metadata, filter->value);
    collection->filters = eina_list_append(collection->filters, filter);
    while(!end)
        {
            metadata = va_arg(ap, char*);
            if (!metadata)
                break;
            value = va_arg(ap, char *);
            if (!value)
                break;
            filter = calloc(1, sizeof(Ems_Collection_Filter));
            filter->metadata = eina_stringshare_add(metadata);
            filter->value = eina_stringshare_add(value);
            collection->filters = eina_list_append(collection->filters, filter);
            DBG("Create new filter %s %s", metadata, value);
        }
    va_end(ap);

    _collections = eina_list_append(_collections, collection);
    return collection;

}

void
ems_collection_free(Ems_Collection *collection)
{
   Ems_Collection_Filter *filter;

    if (!collection)
        return;

    EINA_LIST_FREE(collection->filters, filter)
      {
         eina_stringshare_del(filter->metadata);
         eina_stringshare_del(filter->value);
         free(filter);
      }
    _collections = eina_list_remove(_collections, collection);
    free(collection);
}


