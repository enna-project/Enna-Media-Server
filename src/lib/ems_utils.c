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

#include <ctype.h>

#include <Eina.h>

#include "ems_utils.h"
#include "ems_private.h"
#include "sha1.h"


#define PATTERN_NUMBER "NUM"
#define PATTERN_SEASON  "SE"
#define PATTERN_EPISODE "EP"

#define EMS_ISALNUM(c) isalnum ((int) (unsigned char) (c))
#define EMS_ISGRAPH(c) isgraph ((int) (unsigned char) (c))
#define EMS_ISSPACE(c) isspace ((int) (unsigned char) (c))
#define IS_TO_DECRAPIFY(c)                \
 ((unsigned) (c) <= 0x7F                  \
  && (c) != '\''                          \
  && !EMS_ISSPACE (c)                      \
  && !EMS_ISALNUM (c))

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void
_decrapify_pattern (char *str, const char *bl,
                       unsigned int *se, unsigned int *ep)
{
   char pattern[64];
   int res, len_s = 0, len_e = 0;
   char *it, *it1, *it2;

   /* prepare pattern */
   snprintf (pattern, sizeof (pattern), "%%n%s%%n", bl);
   while ((it = strstr (pattern, PATTERN_NUMBER)))
     memcpy (it, "%*u", 3);

   it1 = strstr (pattern, PATTERN_SEASON);
   it2 = strstr (pattern, PATTERN_EPISODE);
   if (it1)
     memcpy (it1, "%u", 2);
   if (it2)
     memcpy (it2, "%u", 2);

   /* search pattern in the string */
   it = str;
   do
     {
        len_s = 0;
        if (!it1 && it2) /* only EP */
          res = sscanf (it, pattern, &len_s, ep, &len_e);
        else if (!it2 && it1) /* only SE */
          res = sscanf (it, pattern, &len_s, se, &len_e);
        else if (it1 && it2) /* SE & EP */
          {
             if (it1 < it2)
               res = sscanf (it, pattern, &len_s, se, ep, &len_e);
             else
               res = sscanf (it, pattern, &len_s, ep, se, &len_e);
             /* Both must return something with sscanf. */
             if (!*ep)
               *se = 0;
             if (!*se)
               *ep = 0;
          }
        else /* NUM */
          res = sscanf (it, pattern, &len_s, &len_e);
        it += len_s + 1;
     }
   while (!len_e && res != EOF);

   if (len_e <= len_s)
     return;

   it--;

   /*
    * Check if the pattern found is not into a string. Space, start or end
    * of string must be found before and after the part corresponding to
    * the pattern.
    */
   if ((it == str || *it == ' ' || *(it - 1) == ' ') /* nothing or ' ' before */
       && (   *(it + len_e - len_s) == ' '           /* ' ' or nothing after  */
              || *(it + len_e - len_s) == '\0'))
     /* cleanup */
     memset (it, ' ', len_e - len_s);
   else
     {
        *se = 0;
        *ep = 0;
     }
}

static void
_decrapify_blacklist (char **list, char *str, unsigned int *se, unsigned int *ep)
{
   char **l;

   for (l = list; l && *l; l++)
     {
        size_t size;
        char *p;

        if (   strstr (*l, PATTERN_NUMBER)
               || strstr (*l, PATTERN_SEASON)
               || strstr (*l, PATTERN_EPISODE))
          {
             unsigned int i, _se = 0, _ep = 0;

             _decrapify_pattern (str, *l, &_se, &_ep);
             for (i = 0; i < 2; i++)
               {
                  unsigned int val = i ? _ep : _se;

                  if (!val)
                    continue;

                  if (i)
                    {
                       if (ep)
                         *ep = _ep;
                       DBG("Found episode %d", _ep);
                    }
                  else
                    {
                       if (se)
                         *se = _se;
                       DBG("Found season %d", _se);
                    }
               }
             continue;
          }

        p = strcasestr (str, *l);
        if (!p)
          continue;

        size = strlen (*l);
        if (!EMS_ISGRAPH (*(p + size)) && (p == str || !EMS_ISGRAPH (*(p - 1))))
          {
             *p = '\0';
          }
        //memset (p, ' ', size);
     }
   DBG("str : \"%s\"", str);
}

static void
_decrapify_cleanup (char *str)
{
   char *clean, *clean_it;
   char *it;

   clean = malloc (strlen (str) + 1);
   if (!clean)
     return;
   clean_it = clean;

   for (it = str; *it; it++)
     {
        if (!EMS_ISSPACE (*it))
          {
             *clean_it = *it;
             clean_it++;
          }
        else if (!EMS_ISSPACE (*(it + 1)) && *(it + 1) != '\0')
          {
             *clean_it = ' ';
             clean_it++;
          }
     }

   /* remove spaces after */
   while (clean_it > str && EMS_ISSPACE (*(clean_it - 1)))
     clean_it--;
   *clean_it = '\0';

   /* remove spaces before */
   clean_it = clean;
   while (EMS_ISSPACE (*clean_it))
     clean_it++;

   strcpy (str, clean_it);
   free (clean);
}

static Eina_Bool
_isunreserved(unsigned char in)
{
  switch (in) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '-': case '.': case '_': case '~':
      return EINA_TRUE;
    default:
      break;
  }
  return EINA_FALSE;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/* stolen from libvalhalla */
char *
ems_utils_decrapify(const char *file, unsigned int *season, unsigned int *episode)
{
   char *it, *filename, *res;
   char *file_tmp = strdup (file);
   char **items;

   if (!file_tmp)
     return NULL;

   it = strrchr (file_tmp, '.');
   if (it) /* trim suffix */
     *it = '\0';
   it = strrchr (file_tmp, '/');
   if (it) /* trim path */
     it++;
   else
     {
        it = file_tmp;
        DBG("decrapifier: not a path");
     }

   filename = it;

   /* decrapify */
   for (; *it; it++)
     if (IS_TO_DECRAPIFY (*it))
       *it = ' ';

   items = eina_str_split(ems_config->blacklist, ",", 0);
   _decrapify_blacklist(items, filename, season, episode);
   free(*items);
   free(items);
   _decrapify_cleanup (filename);

   res = strdup (filename);
   free (file_tmp);

   DBG("decrapifier: \"%s\"", res);

   return res;
}


/* stolen from libcurl */
char *
ems_utils_escape_string(const char *string)
{
   size_t alloc = strlen(string)+1;
   char *ns;
   char *testing_ptr = NULL;
   unsigned char in; /* we need to treat the characters unsigned */
   size_t newlen = alloc;
   size_t strindex=0;
   size_t length;

   ns = malloc(alloc);
   if(!ns)
     return NULL;

   length = alloc-1;
   while(length--)
     {
        in = *string;

        if(_isunreserved(in))
          /* just copy this */
          ns[strindex++]=in;
        else
          {
             /* encode it */
             newlen += 2; /* the size grows with two, since this'll become a %XX */
             if(newlen > alloc)
               {
                  alloc *= 2;
                  testing_ptr = realloc(ns, alloc);
                  if(!testing_ptr)
                    {
                       free( ns );
                       return NULL;
                    }
                  else
                    {
                       ns = testing_ptr;
                    }
               }

             snprintf(&ns[strindex], 4, "%%%02X", in);

             strindex+=3;
          }
        string++;
     }
   ns[strindex]=0; /* terminate it */
   return ns;
}

Eina_Bool
ems_utils_sha1_compute(const char *filename, unsigned char *sha1)
{
    FILE *fd;
    int i;
    uint64_t fsize;
    unsigned int rsize = 0;
    uint8_t *tmp;
    SHA1 ctx;

    fd = fopen(filename, "rb");
    if (!fd)
      {
         ERR("Error opening %s", filename);
         return EINA_FALSE;
      }

    if (!sha1)
      return EINA_FALSE;

    sha1_init(&ctx);

    fseek(fd, 0, SEEK_END);
    fsize = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    rsize = MIN(65536, fsize);
    tmp = calloc(rsize, sizeof(uint8_t));
    fread(tmp, 1, rsize, fd);
    sha1_update(&ctx, tmp, rsize);
    fseek(fd, (long)(fsize - 65536), SEEK_SET);
    fread(tmp, 1, rsize, fd);
    sha1_update(&ctx, tmp, rsize);
    sha1_final(&ctx, sha1);

    free(tmp);
    fclose(fd);

    return EINA_TRUE;

}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
