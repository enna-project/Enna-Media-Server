#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include <Azy.h>

#include "ems_rpc_Config.azy_client.h"
#include "ems_rpc_Browser.azy_client.h"

#define CALL_CHECK(X)                                                   \
  do                                                                    \
    {                                                                   \
       if (!azy_client_call_checker(cli, err, ret, X, __PRETTY_FUNCTION__)) \
         {                                                              \
            printf("%s\n", azy_content_error_message_get(err));         \
            exit(1);                                                    \
         }                                                              \
    } while (0)

/**
 * Here we receive the response and print it
 */
static Eina_Error
_ems_rpc_Config_Get_Ret(Azy_Client *client, Azy_Content *content, void *_response)
{
   ems_rpc_Config *response = _response;

   if (azy_content_error_is_set(content))
     {
        printf("Error encountered: %s\n", azy_content_error_message_get(content));
        azy_client_close(client);
        ecore_main_loop_quit();
        return azy_content_error_code_get(content);
     }

   //printf("%s: Success? %s!\n", __PRETTY_FUNCTION__, ret ? "YES" : "NO");
   printf("Version : %s\n", response->version);
   printf("Name : %s\n", response->name);
   printf("Port : %d\n", response->port);
   printf("Video Extensions : %s\n", response->video_extensions);



   //ecore_main_loop_quit();

   //response is automaticaly free
   return AZY_ERROR_NONE;
}


static Eina_Error
_ems_rpc_Browser_GetDirectory_Get_Ret(Azy_Client *client, Azy_Content *content, void *_response)
{
   Eina_List *files = _response;
   Eina_List *l;
   ems_rpc_File *f;



   if (azy_content_error_is_set(content))
     {
        printf("Error encountered: %s\n", azy_content_error_message_get(content));
        azy_client_close(client);
        ecore_main_loop_quit();
        return azy_content_error_code_get(content);
     }

   EINA_LIST_FOREACH(files, l, f)
     {
        printf("[%s]%s - %s\n", f->type ? "FILE": "DIR", f->path, f->friendly_name);
     }

   return AZY_ERROR_NONE;
}


/**
 * Here we receive the response and print it
 */
static Eina_Error
_ems_rpc_Browser_GetSources_Get_Ret(Azy_Client *cli, Azy_Content *content, void *_response)
{
   Eina_List *files = _response;
   Eina_List *l;
   ems_rpc_File *f;

   unsigned int ret;
   Azy_Net *net;
   Azy_Content *err;



   if (azy_content_error_is_set(content))
     {
        printf("Error encountered: %s\n", azy_content_error_message_get(content));
        azy_client_close(cli);
        ecore_main_loop_quit();
        return azy_content_error_code_get(content);
     }


   net = azy_client_net_get(cli);
   content = azy_content_new(NULL);
   err = azy_content_new(NULL);
   azy_net_transport_set(net, AZY_NET_TRANSPORT_JSON);

   printf("Sources : \n");
   EINA_LIST_FOREACH(files, l, f)
     {
        printf("%s\n", f->path);
        ret = ems_rpc_Browser_GetDirectory(cli, f->path, err, NULL);
        CALL_CHECK(_ems_rpc_Browser_GetDirectory_Get_Ret);
     }



   azy_content_free(content);

   

   return AZY_ERROR_NONE;
}


/**
 * Bad we have been disconnected
 */
static Eina_Bool _disconnected(void *data, int type, void *data2)
{
   ecore_main_loop_quit();
   return ECORE_CALLBACK_RENEW;
}

/**
 * Yes we are connected ! Now we can send our question
 */
static Eina_Bool
connected(void *data, int type, Azy_Client *cli)
{
   unsigned int ret;
   Azy_Content *content;
   Azy_Net *net;
   Azy_Content *err;

   net = azy_client_net_get(cli);
   content = azy_content_new(NULL);
   err = azy_content_new(NULL);
   azy_net_transport_set(net, AZY_NET_TRANSPORT_JSON);
   /* ret = ems_rpc_Config_GetAll(cli, err, NULL); */
   /* CALL_CHECK(_ems_rpc_Config_Get_Ret); */

   //azy_content_free(content);

   ret = ems_rpc_Browser_GetSources(cli, err, NULL);
   CALL_CHECK(_ems_rpc_Browser_GetSources_Get_Ret);

   return ECORE_CALLBACK_RENEW;
}

int main(int argc, char *argv[])
{
   Ecore_Event_Handler *handler;
   Azy_Client *cli;

   azy_init();

   /* create object for performing client connections */
   cli = azy_client_new();

   if (!azy_client_host_set(cli, "127.0.0.1", 2000))
     return 1;

   handler = ecore_event_handler_add(AZY_CLIENT_CONNECTED, (Ecore_Event_Handler_Cb)connected, cli);
   handler = ecore_event_handler_add(AZY_CLIENT_DISCONNECTED, (Ecore_Event_Handler_Cb)_disconnected, cli);

   /* connect to the servlet on the server specified by uri */
   if (!azy_client_connect(cli, EINA_FALSE))
     return 1;

   ecore_main_loop_begin();

   azy_client_free(cli);
   azy_shutdown();
   ecore_shutdown();
   eina_shutdown();

   return 0;
}
