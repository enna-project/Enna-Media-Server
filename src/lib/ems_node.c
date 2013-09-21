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
#include "ems_server_eet.h"
#include "ems_server_protocol.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Ems_Node_Cb Ems_Node_Cb;
typedef struct _Ems_Node_Media_Get_Cb Ems_Node_Media_Get_Cb;
typedef struct _Ems_Node_Media_Infos_Get_Cb Ems_Node_Media_Infos_Get_Cb;

struct _Ems_Node_Media_Get_Cb
{
   void (*add_cb)(void *data, Ems_Node *node, Ems_Video *media);
   void *data;
};

struct _Ems_Node_Media_Infos_Get_Cb
{
   void (*add_cb)(void *data, Ems_Node *node, const char *media);
   void *data;
};

struct _Ems_Node_Cb
{
   void (*add_cb)(void *data, Ems_Node *node);
   void (*del_cb)(void *data, Ems_Node *node);
   void (*update_cb)(void *data, Ems_Node *node);
   void (*connected_cb)(void *data, Ems_Node *node);
   void (*disconnected_cb)(void *data, Ems_Node *node);
   void *data;
};

static Eina_List *_nodes = NULL;
static Eina_List *_nodes_cb = NULL;
static Eina_List *_media_get_cb = NULL;
//static Eina_List *_media_infos_get_cb = NULL;

static Eina_Hash *_media_info_hash_cb = NULL;


static void
_database_update_cb(void *data __UNUSED__, Ecore_Con_Reply *reply __UNUSED__, const char *name __UNUSED__, void *value __UNUSED__)
{
   
}

static Eina_Bool
_server_connected_cb(void *data, Ecore_Con_Reply *reply, Ecore_Con_Server *conn __UNUSED__)
{
   Ems_Node *node = data;
   Eina_List *l_cb;
   Ems_Node_Cb *cb;
   Ems_Database_Req *req;

   DBG("Connected to %s:%d (%s)", node->ip, node->port, node->name);

   if (node->is_connected)
     return EINA_TRUE;
   else
     node->is_connected = EINA_TRUE;

   node->reply = reply;
   DBG("Node reply : %p", node->reply);

   EINA_LIST_FOREACH(_nodes_cb, l_cb, cb)
     {
        if (cb->connected_cb)
          cb->connected_cb(cb->data, node);
     }

   req = calloc(1, sizeof(Ems_Database_Req));
   req->uuid = node->uuid;
   ecore_con_eet_send(node->reply, "database_get", req);

   return EINA_TRUE;

}

static Eina_Bool
_server_disconnected_cb(void *data, Ecore_Con_Reply *reply __UNUSED__, Ecore_Con_Server *conn __UNUSED__)
{

   Ems_Node *node = data;
   Eina_List *l_cb;
   Ems_Node_Cb *cb;

   DBG("Disconnected from  %s:%d (%s)", node->ip, node->port, node->name);

   node->reply = NULL;

   node->is_connected = EINA_FALSE;
   EINA_LIST_FOREACH(_nodes_cb, l_cb, cb)
     {
        if (cb->disconnected_cb)
          cb->disconnected_cb(cb->data, node);
     }
   return EINA_TRUE;

}

static Eina_Bool
_ems_node_connect(Ems_Node *node)
{
   Ecore_Con_Server *conn;

   if (!node)
     return EINA_FALSE;

   INF("try to connect to [%s] %s:%d", node->name, node->ip, node->port);

   if (node->is_connected)
     return EINA_TRUE;

   conn = ecore_con_server_connect(ECORE_CON_REMOTE_TCP, node->ip, node->port, node);
   node->ece = ecore_con_eet_client_new(conn);
   ecore_con_eet_data_set(node->ece, conn);
   ecore_con_eet_server_connect_callback_add(node->ece, _server_connected_cb, node);
   ecore_con_eet_server_disconnect_callback_add(node->ece, _server_disconnected_cb, node);

   ecore_con_eet_register(node->ece, "database_get", ems_database_req_edd);
   ecore_con_eet_data_callback_add(node->ece, "database_update", _database_update_cb, node);

   return EINA_TRUE;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
ems_node_init(void)
{
   return ems_server_eet_init();
}

Eina_Bool
ems_node_start(void)
{
  return ems_server_eet_start();
}

void ems_node_shutdown(void)
{
   ems_server_eet_shutdown();
}

void ems_node_add(Ems_Node *node)
{
   Eina_List *l_cb;
   Ems_Node_Cb *cb;


   if (!node || !node->name)
     return;

   INF("Adding %s to the list of detected node", node->name);

   EINA_LIST_FOREACH(_nodes_cb, l_cb, cb)
       {
	  if (cb->add_cb)
	    {
	       cb->add_cb(cb->data, node);
	    }
       }
   _nodes = eina_list_append(_nodes, node);


}

void ems_node_del(const char *name)
{
   Eina_List *l;
   Ems_Node *node;

   if (!_nodes || !name)
     return;

   EINA_LIST_FOREACH(_nodes, l, node)
     {
        if (!node) continue;
        if (!strcmp(name, node->name))
          {
             Eina_List *l_cb;
             Ems_Node_Cb *cb;

             INF("Remove %s from the list of detected nodes", node->name);

             EINA_LIST_FOREACH(_nodes_cb, l_cb, cb)
               {
                  if (cb->del_cb)
                    cb->del_cb(cb->data, node);
               }

             _nodes = eina_list_remove(_nodes, node);
             eina_stringshare_del(node->name);
             if (node->ip) eina_stringshare_del(node->ip);
             free(node);
          }
     }
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
Eina_List *
ems_node_list_get(void)
{
   return _nodes;
}

void
ems_node_cb_set(Ems_Node_Add_Cb node_add_cb,
                  Ems_Node_Del_Cb node_del_cb,
                  Ems_Node_Update_Cb node_update_cb,
                  Ems_Node_Connected_Cb node_connected_cb,
                  Ems_Node_Disconnected_Cb node_disconnected_cb,
                  void *data)
{
   Ems_Node_Cb *cb;

   cb = calloc(1, sizeof(Ems_Node_Cb));
   if (!cb)
     return;

   cb->add_cb = node_add_cb;
   cb->del_cb = node_del_cb;
   cb->update_cb = node_update_cb;
   cb->connected_cb = node_connected_cb;
   cb->disconnected_cb = node_disconnected_cb;
   cb->data = data;

   _nodes_cb = eina_list_append(_nodes_cb, cb);
}

void
ems_node_cb_del(Ems_Node_Add_Cb node_add_cb,
                  Ems_Node_Del_Cb node_del_cb,
                  Ems_Node_Update_Cb node_update_cb)
{
   Eina_List *l;
   Ems_Node_Cb *cb;

   EINA_LIST_FOREACH(_nodes_cb, l, cb)
     {
        int cnt = 0;
        if (node_add_cb == cb->add_cb)
          {
             cb->add_cb = NULL;
             cnt++;
          }
        if (node_del_cb == cb->del_cb)
          {
             cb->del_cb = NULL;
             cnt++;
          }
        if (node_update_cb == cb->update_cb)
          {
             cb->add_cb = NULL;
             cnt++;
          }
        if (cnt == 3)
          {
             _nodes_cb = eina_list_remove(_nodes_cb, cb);
             free(cb);
          }
     }
}

const char *
ems_node_name_get(Ems_Node *node)
{
   if (!node)
     return NULL;

   return node->name;
}

const char *
ems_node_ip_get(Ems_Node *node)
{
   if (!node)
     return NULL;

   return node->ip;
}

int
ems_node_port_get(Ems_Node *node)
{
   if (!node)
     return -1;

   return node->port;
}

Eina_Bool
ems_node_is_local(Ems_Node *node)
{
   if (node)
     return node->is_local;
   else
     return EINA_TRUE;
}

Eina_Bool
ems_node_is_connected(Ems_Node *node)
{
   if (node)
     return node->is_connected;
   else
     return EINA_TRUE;
}

Eina_Bool
ems_node_connect(Ems_Node *node)
{
   if (!node)
     return EINA_FALSE;

   if (node->is_connected)
     return EINA_TRUE;

    return _ems_node_connect(node);
}

char *
ems_node_media_stream_url_get(Ems_Node *node, Ems_Video *media)
{
   Eina_Strbuf *uri;
   char *str;

   if (!node || !media)
     return NULL;

   if (node->is_local)
   {
       uri = eina_strbuf_new();
       eina_strbuf_append_printf(uri, "file://%s", media->title);
       DBG("url get: %s", eina_strbuf_string_get(uri));
       str = strdup(eina_strbuf_string_get(uri));
       eina_strbuf_free(uri);
   }
   else
   {
       uri = eina_strbuf_new();
       eina_strbuf_append_printf(uri, "http://%s:%d/item/%s", node->ip, ems_config->port_stream, media->hash_key);
       DBG("url get: %s", eina_strbuf_string_get(uri));
       str = strdup(eina_strbuf_string_get(uri));
       eina_strbuf_free(uri);
   }

   return str;
}

Ems_Db_Database *
ems_node_database_get(Ems_Node *node)
{
    if (!node)
        return;

    DBG("Get the database for node [%s] %s", node->name, node->uuid);
    if (node->is_local)
    {
        DBG("Db is local nothing to do\n");
        return ems_database_get(node->uuid);
    }
    else
    {
        ERR("Not implemented yet! Database has to be retrieved !");
        return NULL;
    }

}

Ems_Observer *
ems_node_media_get(Ems_Node *node,
                   Ems_Collection *collection,
                   Ems_Media_Add_Cb media_add,
                   void *data)
{

   Ems_Db_Database *db;
   Eina_List *l, *files;
   Ems_Video *v;
   db = ems_node_database_get(node);

   files = ems_database_files_get(db);
   EINA_LIST_FOREACH(files, l, v)
   {
       if(media_add)
           media_add(data, node, v);
   }

   return NULL;
}

Ems_Observer *
ems_node_media_info_get(Ems_Node *node,
                          const char *sha1,
                          const char *metadata,
                          Ems_Media_Info_Add_Cb info_add,
                          Ems_Media_Info_Add_Cb info_del __UNUSED__,
                          Ems_Media_Info_Update_Cb info_update __UNUSED__,
                          void *data)
{
    if (info_add)
        info_add(data, node, ems_database_info_get(sha1, metadata));
}
