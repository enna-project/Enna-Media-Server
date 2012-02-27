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

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static Eina_Array *_modules = NULL;

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
ems_parser_init(void)
{
   Eina_Array_Iterator iterator;
   Eina_Module *m;
   unsigned int i;
   const char *s;

   _modules = eina_module_arch_list_get(NULL,
                                        PACKAGE_LIB_DIR "/ems/grabbers",
                                        MODULE_ARCH);



   eina_module_list_load(_modules);

   EINA_ARRAY_ITER_NEXT(_modules, i, m, iterator)
     INF("loading module : %s", eina_module_file_get(m));



   return EINA_TRUE;
}

void
ems_parser_shutdown(void)
{
   eina_module_list_free(_modules);
   if (_modules)
      eina_array_free(_modules);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
