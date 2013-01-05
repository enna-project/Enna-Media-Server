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
#include "ems_node.h"
#include "ems_server_protocol.h"

Eet_Data_Descriptor *ems_medias_req_edd = NULL;
Eet_Data_Descriptor *ems_medias_add_edd = NULL;
Eet_Data_Descriptor *ems_media_infos_edd = NULL;
Eet_Data_Descriptor *ems_media_infos_req_edd = NULL;

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static Eet_Data_Descriptor *
_medias_req_edd(void)
{
   Eet_Data_Descriptor *edd = NULL;
   Eet_Data_Descriptor *collection_edd = NULL;
   Eet_Data_Descriptor *collection_filter_edd = NULL;
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Collection_Filter);
   collection_filter_edd =  eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(collection_filter_edd, Ems_Collection_Filter, "metadata", metadata, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(collection_filter_edd, Ems_Collection_Filter, "value", value, EET_T_STRING);


   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Ems_Collection);
   collection_edd =  eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(collection_edd, Ems_Collection, "type", type, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_LIST(collection_edd, Ems_Collection, "filters", filters, collection_filter_edd);


   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Medias_Req);
   edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_SUB(edd, Medias_Req, "collection", collection, collection_edd);

   return edd;
}

static Eet_Data_Descriptor *
_medias_edd(void)
{
   Eet_Data_Descriptor *edd = NULL;
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Medias);
   edd =  eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Medias, "files", files, _edd_media);

   return edd;
}

static Eet_Data_Descriptor *
_media_infos_req_edd(void)
{
   Eet_Data_Descriptor *edd = NULL;
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Media_Infos_Req);
   edd =  eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Media_Infos_Req, "sha1", sha1, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Media_Infos_Req, "metadata", metadata, EET_T_STRING);

   return edd;
}

static Eet_Data_Descriptor *
_media_infos_edd(void)
{
   Eet_Data_Descriptor *edd = NULL;
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Media_Infos);
   edd =  eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Media_Infos, "sha1", sha1, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Media_Infos, "value", value, EET_T_STRING);

   return edd;
}

struct _Match_Type {
   const char *name;
   Eet_Data_Descriptor *(*func)(void);
   Eet_Data_Descriptor **edd;
} match_type[] = {
  { "medias_req", _medias_req_edd, &ems_medias_req_edd },
  { "medias", _medias_edd, &ems_medias_add_edd },
  { "media_info", _media_infos_edd, &ems_media_infos_edd },
  { "media_info_req", _media_infos_req_edd, &ems_media_infos_req_edd },
  { NULL, 0, NULL }
};

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

void
ems_server_protocol_init(void)
{
   int i;

   for (i = 0; match_type[i].name != NULL; i++)
     *match_type[i].edd = match_type[i].func();
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
