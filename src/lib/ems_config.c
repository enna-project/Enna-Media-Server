#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <Eet.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include "ems_private.h"
#include "ems_config.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static Eet_Data_Descriptor *conf_edd = NULL;
static Eet_Data_Descriptor *video_directory_edd = NULL;

static const char *
ems_config_filename_get(void)
{
   static const char *filename = NULL;
   char tmp[PATH_MAX];

   if (filename)
     return filename;

   snprintf(tmp, sizeof(tmp), "%s/.config/enna-media-server/%s", getenv("HOME"), EMS_CONFIG_FILE);
   return eina_stringshare_add(tmp);
}

static const char *
ems_config_dirname_get(void)
{
   return ecore_file_dir_get(ems_config_filename_get());
}

static const char *
ems_config_cache_filename_get(void)
{
   static const char *filename = NULL;
   char tmp[PATH_MAX];

   if (filename)
     return filename;

   snprintf(tmp, sizeof(tmp), "%s/.cache/enna-media-server/%s", getenv("HOME"), EMS_CONFIG_FILE);

   return eina_stringshare_add(tmp);
}

static const char *
ems_config_cache_dirname_get(void)
{
   return ecore_file_dir_get(ems_config_cache_filename_get());
}

static const char *
ems_config_default_filename_get(void)
{
   static const char *filename = NULL;
   char tmp[PATH_MAX];

   if (filename)
     return filename;

   snprintf(tmp, sizeof(tmp), "%s/%s", PACKAGE_DATA_DIR, EMS_CONFIG_FILE);

   return eina_stringshare_add(tmp);
}

static Eet_Data_Descriptor *
ems_config_descriptor_new(const char *name, int size)
{
   Eet_Data_Descriptor_Class eddc;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc), name, size))
     return NULL;

   return eet_data_descriptor_stream_new(&eddc);
}


static void
_make_config(void)
{
   Eet_File *ef;
   FILE *f;
   int textlen;
   char *text;

   if (!ecore_file_is_dir(ems_config_dirname_get()))
     ecore_file_mkpath(ems_config_dirname_get());

   if (!ecore_file_is_dir(ems_config_cache_dirname_get()))
     ecore_file_mkpath(ems_config_cache_dirname_get());

   ef = eet_open(ems_config_cache_filename_get(),
                 EET_FILE_MODE_READ_WRITE);
   if (!ef)
     ef = eet_open(ems_config_cache_filename_get(),
                   EET_FILE_MODE_WRITE);
   f = fopen(ems_config_filename_get(), "rb");
   if (!f)
     {
        WRN("Could not open '%s', setup default config.", ems_config_filename_get());
        if (ecore_file_exists(ems_config_default_filename_get()))
          {
             ecore_file_cp(ems_config_default_filename_get(), ems_config_filename_get());
             f = fopen(ems_config_filename_get(), "rb");
             if (!f)
               return;
          }
        else
          return;
     }

   fseek(f, 0, SEEK_END);
   textlen = ftell(f);
   rewind(f);
   text = (char *)malloc(textlen);
   if (!text)
     {
        fclose(f);
        eet_close(ef);
        return;
     }

   if (fread(text, textlen, 1, f) != 1)
     {
        free(text);
        fclose(f);
        eet_close(ef);
        return;
     }

   fclose(f);
   if (eet_data_undump(ef, "config", text, textlen, 1))
     INF("Updating configuration");
   free(text);
   eet_close(ef);
}

static Ems_Config *
_config_get(Eet_Data_Descriptor *edd)
{
   Ems_Config *config = NULL;
   Eet_File *file;

   if (!ecore_file_is_dir(ems_config_cache_dirname_get()))
     ecore_file_mkdir(ems_config_cache_dirname_get());
   file = eet_open(ems_config_cache_filename_get(),
                   EET_FILE_MODE_READ_WRITE);

   config = eet_data_read(file, edd, "config");
   if (!config)
     {
        WRN("Warning no configuration found! This must not happen, we will go back to a void configuration");
        return NULL;
     }

   eet_close(file);

   return config;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
ems_config_init(void)
{
   struct stat cache;
   struct stat conf;

   video_directory_edd = ENNA_CONFIG_DD_NEW("Ems_Directory",
                                            Ems_Directory);
#undef T
#undef D
#define T Ems_Directory
#define D video_directory_edd
   ENNA_CONFIG_VAL(D, T, path, EET_T_STRING);
   ENNA_CONFIG_VAL(D, T, label, EET_T_STRING);


   conf_edd = ENNA_CONFIG_DD_NEW("config", Ems_Config);
#undef T
#undef D
#define T Ems_Config
#define D conf_edd
   ENNA_CONFIG_VAL(D, T, version, EET_T_UINT);
   ENNA_CONFIG_VAL(D, T, port, EET_T_SHORT);
   ENNA_CONFIG_VAL(D, T, name, EET_T_STRING);
   ENNA_CONFIG_LIST(D, T, video_directories, video_directory_edd);
   ENNA_CONFIG_VAL(D, T, video_extensions, EET_T_STRING);
   ENNA_CONFIG_VAL(D, T, scan_period, EET_T_UINT);

   if (stat(ems_config_cache_filename_get(), &cache) == -1)
     {
        _make_config();
     }
   else
     {
        stat(ems_config_filename_get(), &conf);
        if (cache.st_mtime < conf.st_mtime)
          _make_config();
     }

   ems_config = _config_get(conf_edd);

   return EINA_TRUE;
}

void
ems_config_shutdown(void)
{
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
