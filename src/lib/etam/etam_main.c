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

static int _etam_init_count = 0;

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

int etam_init(void)
{
   if (++_etam_init_count != 1)
     return _etam_init_count;

   if (!eina_init())
     return --_etam_init_count;

   _etam_log_dom_global = eina_log_domain_register("etam", ETAM_DEFAULT_LOG_COLOR);
   if (_etam_log_dom_global < 0)
     {
        EINA_LOG_ERR("Etam Can not create a general log domain.");
        goto shutdown_eina;
     }
   return _etam_init_count;

 shutdown_eina:
   eina_shutdown();

   return --_etam_init_count;
}

int etam_shutdown(void)
{
   if (--_etam_init_count != 0)
     return _etam_init_count;

   eina_shutdown();

   return _etam_init_count;
}

Etam_DB *etam_db_open(const char *uri,
                      Etam_DB_Validate_Cb cb,
                      const void *data)
{
   DBG("");
}
