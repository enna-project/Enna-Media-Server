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
#include <Ecore.h>
#include <Ecore_Con.h>

#include "Ems.h"
#include "ems_private.h"

#include "http_parser.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static const char *error400 = "HTTP/1.1 400 Bad Request\r\n"
                              "Connection: close\r\n"
                              "Content-Type: text/html\r\n\r\n"
                              "<html>"
                              "<head>"
                              "<title>400 Bad Request</title>"
                              "</head>"
                              "<body>"
                              "<h1>Enna Media Server - Bad Request</h1>"
                              "<p>The server received a request it could not understand.</p>"
                              "</body>"
                              "</html>";

static Ecore_Con_Server *_server = NULL;

static Ecore_Event_Handler *_handler_add = NULL;
static Ecore_Event_Handler *_handler_del= NULL;
static Ecore_Event_Handler *_handler_data = NULL;

typedef struct _Ems_Stream_Client Ems_Stream_Client;

struct _Ems_Stream_Client
{
   Ecore_Con_Client *client;

   Eina_Hash *request_headers;

   http_parser *parser;
   Eina_Bool headers_done;
   Eina_Bool parse_done;
   enum { NONE = 0, FIELD, VALUE } last_header_element;

   Eina_Strbuf *header_field;
   Eina_Strbuf *header_value;
   Eina_List *request_path;
   unsigned char request_method;
};

Eina_Hash *_stream_clients = NULL;

static http_parser_settings _parser_settings;

static int _parser_header_field(http_parser *parser, const char *at, size_t length);
static int _parser_header_value(http_parser *parser, const char *at, size_t length);
static int _parser_headers_complete(http_parser *parser);
static int _parser_message_complete(http_parser *parser);

static void _stream_client_free(Ems_Stream_Client *client);
static void _stream_server_request_process(Ems_Stream_Client *client);

static void
_header_free(void *data)
{
   eina_stringshare_del(data);
}

static Eina_Bool
_client_add(void *data, int type, Ecore_Con_Event_Client_Add *ev)
{
   Ems_Stream_Client *client;

   if (ev && (_server != ecore_con_client_server_get(ev->client)))
     return ECORE_CALLBACK_PASS_ON;

   client = calloc(1, sizeof(Ems_Stream_Client));
   client->client = ev->client;
   client->parser = calloc(1, sizeof(http_parser));
   http_parser_init(client->parser, HTTP_REQUEST);
   client->parser->data = client;
   client->request_headers = eina_hash_string_superfast_new((Eina_Free_Cb)_header_free);

   eina_hash_add(_stream_clients, ev->client, client);

   INF("New client from : %s", ecore_con_client_ip_get(ev->client));

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_client_del(void *data, int type, Ecore_Con_Event_Client_Del *ev)
{
   Ems_Stream_Client *client;

   if (ev && (_server != ecore_con_client_server_get(ev->client)))
     return ECORE_CALLBACK_PASS_ON;

   client = eina_hash_find(_stream_clients, ev->client);

   if (!client)
     {
        ERR("Can't find Ems_Stream_Client !");
     }
   else
     {
        eina_hash_del(_stream_clients, ev->client, NULL);
     }

   INF("Connection from %s closed by remote host", ecore_con_client_ip_get(ev->client));

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_client_data(void *data, int type, Ecore_Con_Event_Client_Data *ev)
{
   Ems_Stream_Client *client;
   int nparsed;

   if (ev && (_server != ecore_con_client_server_get(ev->client)))
     return ECORE_CALLBACK_PASS_ON;

   DBG("Got data from client : %s\n%s\n", ecore_con_client_ip_get(ev->client), ev->data);

   client = eina_hash_find(_stream_clients, ev->client);

   if (!client)
     {
        ERR("Can't find Ems_Stream_Client !");
     }
   else
     {
        nparsed = http_parser_execute(client->parser, &_parser_settings, ev->data, ev->size);

        if (client->parser->upgrade)
          {
             /* handle new protocol */
             INF("Protocol Upgrade not supported");
          } 
        else if (nparsed != ev->size)
          {
             /* Handle error. Usually just close the connection. */
             ecore_con_client_del(ev->client);
          }

        if (client->parse_done)
          {
             //Finally parsing of request is done, we can search for
             //a response for the requested path
             _stream_server_request_process(client);
          }
     }

   return ECORE_CALLBACK_RENEW;
}

void
_stream_server_request_process(Ems_Stream_Client *client)
{
   if (!client)
     return;

   

   ecore_con_client_send(client->client, error400, strlen(error400));

   //Use a timer to close the connection after data has been sent?
   ecore_timer_add(0.00001, (Ecore_Task_Cb)ecore_con_client_del, client->client);
}

void
_stream_client_free(Ems_Stream_Client *client)
{
   const char *data;

   if (!client) 
     return;

   free(client->parser);
   eina_hash_free(client->request_headers);
   EINA_LIST_FREE(client->request_path, data)
      eina_stringshare_del(data);
   free(client);
}

int
_parser_header_field(http_parser *parser, const char *at, size_t length)
{
   Ems_Stream_Client *client = parser->data;

   if (client->header_value && client->header_field)
     {
        eina_hash_add(client->request_headers, 
                      eina_strbuf_string_get(client->header_field),
                      eina_stringshare_add(eina_strbuf_string_get(client->header_value)));

        eina_strbuf_free(client->header_field);
        eina_strbuf_free(client->header_value);
        client->header_field = NULL;
        client->header_value = NULL;
     }

   if (!client->header_field)
     client->header_field = eina_strbuf_new();

   eina_strbuf_append_length(client->header_field, at, length);

   return 0;
}

int
_parser_header_value(http_parser *parser, const char *at, size_t length)
{
   Ems_Stream_Client *client = parser->data;

   if (!client->header_value)
     client->header_value = eina_strbuf_new();

   eina_strbuf_append_length(client->header_value, at, length);

   return 0;
}

int
_parser_headers_complete(http_parser *parser)
{
   Ems_Stream_Client *client = parser->data;
   client->headers_done = EINA_TRUE;

   if (client->header_value && client->header_field)
     {
        eina_hash_add(client->request_headers, 
                      eina_strbuf_string_get(client->header_field),
                      eina_stringshare_add(eina_strbuf_string_get(client->header_value)));

        eina_strbuf_free(client->header_field);
        eina_strbuf_free(client->header_value);
        client->header_field = NULL;
        client->header_value = NULL;
     }

   DBG("parse headers done:");
   Eina_Iterator *it = eina_hash_iterator_tuple_new(client->request_headers);
   void *data;
   while (eina_iterator_next(it, &data))
     {
        Eina_Hash_Tuple *t = data;
        DBG("HEADER: %s : %s", (const char *)t->key, (const char *)t->data);
     }
   eina_iterator_free(it);

   return 0;
}

int
_parser_url(http_parser *parser, const char *at, size_t length)
{
   Ems_Stream_Client *client = parser->data;
   char **arr;
   int i;
   Eina_Strbuf *path = eina_strbuf_new();

   eina_strbuf_append_length(path, at, length);
   arr = eina_str_split(eina_strbuf_string_get(path), "/", 0);

   for (i = 0; arr[i]; i++)
     {
        if (strlen(arr[i]) > 0)
          client->request_path = eina_list_append(client->request_path,
                                                  eina_stringshare_add(arr[i]));
     }

   free(arr[0]);
   free(arr);

   //TODO: parse query parameters like:
   // [/path/requested][?query]&[key=value]
   // This needs to be parsed from GET or POST requests

   DBG("URL Path requested: %s", eina_strbuf_string_get(path));
   eina_strbuf_free(path);

   Eina_List *l;
   const char *data;
   EINA_LIST_FOREACH(client->request_path, l, data)
      DBG("PATH: %s", data);

   return 0;
}

int
_parser_message_complete(http_parser *parser)
{
   Ems_Stream_Client *client = parser->data;
   client->parse_done = EINA_TRUE;
   client->request_method = client->parser->method;

   DBG("parse message done");

   return 0;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
ems_stream_server_init(void)
{
   _server = ecore_con_server_add(ECORE_CON_REMOTE_TCP, "0.0.0.0", ems_config->port_stream, NULL);

   _handler_add = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_ADD, 
                           (Ecore_Event_Handler_Cb)_client_add, NULL);
   _handler_del = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DEL,
                           (Ecore_Event_Handler_Cb)_client_del, NULL);
   _handler_data = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA,
                           (Ecore_Event_Handler_Cb)_client_data, NULL);

   _stream_clients = eina_hash_pointer_new((Eina_Free_Cb)_stream_client_free);

   //set up callbacks for the parser
   _parser_settings.on_message_begin = NULL;
   _parser_settings.on_url = _parser_url;
   _parser_settings.on_header_field = _parser_header_field;
   _parser_settings.on_header_value = _parser_header_value;
   _parser_settings.on_headers_complete = _parser_headers_complete;
   _parser_settings.on_body = NULL;
   _parser_settings.on_message_complete = _parser_message_complete;

   INF("Listening to port %d", ems_config->port_stream);

   return EINA_TRUE;
}

void
ems_stream_server_shutdown(void)
{
   ecore_con_server_del(_server);

   ecore_event_handler_del(_handler_add);
   ecore_event_handler_del(_handler_del);
   ecore_event_handler_del(_handler_data);
}


