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
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_Con.h>

#include "Ems.h"
#include "ems_private.h"
#include "ems_server.h"
#include "ems_server_protocol.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static Eina_Bool _serialisation_type_set(const char *type, void *data, Eina_Bool unknow);
static const char *_serialisation_type_get(const void *data, Eina_Bool *unknow);


static Eet_Data_Descriptor *
_get_medias_req_edd()
{
   static Eet_Data_Descriptor *edd = NULL;
   Eet_Data_Descriptor *collection_edd = NULL;
   Eet_Data_Descriptor *collection_filter_edd = NULL;
   Eet_Data_Descriptor_Class eddc;

   if (edd) return edd;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Collection_Filter);
   collection_filter_edd =  eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(collection_filter_edd, Ems_Collection_Filter, "metadata", metadata, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(collection_filter_edd, Ems_Collection_Filter, "value", value, EET_T_STRING);


   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Collection);
   collection_edd =  eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(collection_edd, Ems_Collection, "type", type, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_LIST(collection_edd, Ems_Collection, "filters", filters, collection_filter_edd);


   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Get_Medias_Req);
   edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_SUB(edd, Get_Medias_Req, "collection", collection, collection_edd);

   return edd;
}

typedef struct _Files
{
   const char *file;
}Files;

static Eet_Data_Descriptor *
_get_medias_edd()
{
   static Eet_Data_Descriptor *edd = NULL;
   Eet_Data_Descriptor_Class eddc;

   if (edd) return edd;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Get_Medias);
   edd =  eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd, Get_Medias, "files", files);

   return edd;
}

Match_Type match_type[] = {
  { "get_medias_req", EMS_SERVER_PROTOCOL_TYPE_GET_MEDIAS_REQ, _get_medias_req_edd },
  { "get_medias", EMS_SERVER_PROTOCOL_TYPE_GET_MEDIAS, _get_medias_edd },
  { NULL, 0, NULL }
};


static Eina_Bool _serialisation_type_set(const char *type, void *data, Eina_Bool unknow)
{
   Ems_Server_Protocol_Type *ev = data;
   int i;

   if (unknow) return EINA_FALSE;

   for (i = 0; match_type[i].name != NULL; ++i)
     if (!strcmp(type, match_type[i].name))
       {
          DBG("found '%s'", match_type[i].name);
          *ev = match_type[i].type;
          return EINA_TRUE;
       }

   DBG("not found !");
   *ev = EMS_SERVER_PROTOCOL_TYPE_UNKNOWN;
   return EINA_FALSE;
}

static const char *_serialisation_type_get(const void *data, Eina_Bool *unknow)
{
   const Ems_Server_Protocol_Type *ev = data;
   int i;

   for (i = 0; match_type[i].name != NULL; ++i)
     if (*ev == match_type[i].type)
       {
          DBG("found '%s'", match_type[i].name);
          return match_type[i].name;
       }

   if (unknow)
     *unknow = EINA_TRUE;

   DBG("unknow (%i)[%p] !", *ev, ev);
   return NULL;
}


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eet_Data_Descriptor *
ems_server_protocol_init(void)
{
   Eet_Data_Descriptor_Class eddc;
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor *un;
   int i;

   edd = NULL;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Server_Protocol);
   edd = eet_data_descriptor_stream_new(&eddc);

   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.type_get = _serialisation_type_get;
   eddc.func.type_set = _serialisation_type_set;
   un = eet_data_descriptor_stream_new(&eddc);

   for (i = 0; match_type[i].name != NULL; i++)
     EET_DATA_DESCRIPTOR_ADD_MAPPING(un, match_type[i].name, match_type[i].edd());

   EET_DATA_DESCRIPTOR_ADD_UNION(edd, Ems_Server_Protocol, "Ems_Server_Protocol", data, type, un);

   return edd;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
