#include <Eet.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ems_private.h"
#include "ems_config.h"

static Eet_Data_Descriptor *conf_edd = NULL;
static Eet_Data_Descriptor *video_directory_edd = NULL;
static Eet_Data_Descriptor *video_extension_edd = NULL;

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

   if (!ecore_file_is_dir("/home/nico/.config/enna-media-server"))
     ecore_file_mkdir("/home/nico/.config/enna-media-server");

   if (!ecore_file_is_dir("/home/nico/.cache/enna-media-server"))
     ecore_file_mkdir("/home/nico/.cache/enna-media-server");
   ef = eet_open("/home/nico/.cache/enna-media-server/"EMS_CONFIG_FILE,
                 EET_FILE_MODE_READ_WRITE);
   if (!ef)
     ef = eet_open("/home/nico/.cache/enna-media-server/"EMS_CONFIG_FILE,
                   EET_FILE_MODE_WRITE);
   f = fopen("/home/nico/.config/enna-media-server/enna-media-server.conf", "rb");
   if (!f)
     {
        ERR("Could not open /home/nico/.config/enna-media-server/enna-media-server.conf");
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
   if (eet_data_undump(ef, "Ems_Config", text, textlen, 1))
     INF("Updating configuration");
   free(text);
   eet_close(ef);
}

static Ems_Config *
_config_get(Eet_Data_Descriptor *edd)
{
   Ems_Config *config = NULL;
   Eet_File *file;

   if (!ecore_file_is_dir("/home/nico/.cache/enna-media-server"))
     ecore_file_mkdir("/home/nico/.cache/enna-media-server");
   file = eet_open("/home/nico/.cache/enna-media-server/"EMS_CONFIG_FILE,
                   EET_FILE_MODE_READ_WRITE);

   config = eet_data_read(file, edd, "Ems_Config");
   if (!config)
     {
        Ems_Directory *dir;
        Ems_Extension *ext;
        WRN("Warning no configuration found! This must not append, we will go back to a void configuration");
        config = calloc(1, sizeof(Ems_Config));
        /* dir = calloc(1, sizeof(Ems_Directory)); */
        /* dir->path = eina_stringshare_add("/home/nico/videos"); */
        /* dir->label = eina_stringshare_add("Videos Locales"); */
        /* config->video_directories = eina_list_append(config->video_directories, dir); */
        /* dir = calloc(1, sizeof(Ems_Directory)); */
        /* dir->path = eina_stringshare_add("/home/partage/videos"); */
        /* dir->label = eina_stringshare_add("Videos Serveur"); */
        /* config->video_directories = eina_list_append(config->video_directories, dir); */
        /* ext = calloc(1, sizeof(Ems_Extension)); */
        /* ext->ext = eina_stringshare_add(".avi"); */
        /* config->video_extensions = eina_list_append(config->video_extensions, ext); */
        /* ext = calloc(1, sizeof(Ems_Extension)); */
        /* ext->ext = eina_stringshare_add(".mov"); */
        /* config->video_extensions = eina_list_append(config->video_extensions, ext); */
        /* printf("Write to file\n"); */
        /* eet_data_write(file, edd, "Ems_Config", config, 1); */
        //eet_data_text_dump(data, len, output, NULL);
     }

   eet_close(file);

   return config;
}

void
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


   video_extension_edd = ENNA_CONFIG_DD_NEW("Ems_Extension",
                                            Ems_Extension);
#undef T
#undef D
#define T Ems_Extension
#define D video_extension_edd
   ENNA_CONFIG_VAL(D, T, ext, EET_T_STRING);

   conf_edd = ENNA_CONFIG_DD_NEW("Ems_Config", Ems_Config);
#undef T
#undef D
#define T Ems_Config
#define D conf_edd
   ENNA_CONFIG_LIST(D, T, video_directories, video_directory_edd);
   ENNA_CONFIG_LIST(D, T, video_extensions, video_extension_edd);

   if (stat("/home/nico/.cache/enna-media-server/"EMS_CONFIG_FILE, &cache) == -1)
     {
        _make_config();
     }
   else
     {
        stat("/home/nico/.config/enna-media-server/enna-server.conf", &conf);
        if (cache.st_mtime < conf.st_mtime)
          _make_config();
     }

   ems_config = _config_get(conf_edd);



}

void
ems_config_shutdown(void)
{
}
