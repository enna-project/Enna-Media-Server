#include "ems_config.h"

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
