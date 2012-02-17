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

#include <Ecore.h>
#include <Azy.h>
#include <Esskyuehl.h>

#include "Demo_Message.azy_server.h"


static Eina_Bool
error_(void *data, int type, Esql_Res *res)
{
   fprintf(stderr, "Error : %s\n", esql_res_error_get(res)); /**< print error condition */
   ecore_main_loop_quit();
   return ECORE_CALLBACK_RENEW;
}

int main(int argc, char *argv[])
{
	azy_init();

	//Init Esskyuehl.h
	esql_init();
	ecore_event_handler_add(ESQL_EVENT_ERROR, (Ecore_Event_Handler_Cb)error_, NULL);
	//


        //Define the list of module used by the server.
	Azy_Server_Module_Def *modules[] = {
			Demo_Message_module_def(),
			NULL
	};

	//launch the server
	azy_server_basic_run(3412, AZY_SERVER_LOCAL, NULL, modules);

	esql_shutdown();
	azy_shutdown();
	ecore_shutdown();
	eina_shutdown();

	return 0;
}
