#include <Ecore.h>
#include <Azy.h>
#include "EMS_Config.azy_client.h"

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
_EMS_Config_Get_Ret(Azy_Client *client, Azy_Content *content, void *_response)
{
   EMS_Config *response = _response;

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
   

   EMS_Config *cfg;
   Azy_Net *net;   
   Azy_Content *err;
   cfg  = EMS_Config_new();

   cfg->name = eina_stringshare_add("Test new Name #1");
   cfg->port = 1337;

   net = azy_client_net_get(client);
   content = azy_content_new(NULL);
   err = azy_content_new(NULL);
   azy_net_transport_set(net, AZY_NET_TRANSPORT_JSON);
   EMS_Config_SetAll(client, cfg, err, NULL);

   azy_content_free(content);



   //ecore_main_loop_quit();

   //response is automaticaly free
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
   ret = EMS_Config_GetAll(cli, err, NULL);
   CALL_CHECK(_EMS_Config_Get_Ret);

   azy_content_free(content);

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
