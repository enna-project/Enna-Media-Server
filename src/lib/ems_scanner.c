#include <Eio.h>

#include "ems_private.h"

static Eina_Bool
_ems_util_has_suffix(const char *name, const char *extensions)
{
   Eina_Bool ret = EINA_FALSE;
   int i;
   char *ext;
   char **arr = eina_str_split(extensions, ",", 0);

   if (!arr)
     return EINA_FALSE;

   for (i = 0; arr[i]; i++)
     {
        if (eina_str_has_extension(name, arr[i]))
          {
             ret = EINA_TRUE;
             break;
          }
     }
   free(arr[0]);
   free(arr);

   return ret;
}


static Eina_Bool
_file_filter_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)
{

    if (*(info->path + info->name_start) == '.' )
        return EINA_FALSE;

    if ( info->type == EINA_FILE_DIR ||
         _ems_util_has_suffix(info->path + info->name_start, ems_config->video_extensions))
        return EINA_TRUE;
    else
        return EINA_FALSE;
}

static void
_file_main_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)
{
   DBG("Found %s", info->path);
}

static void
_file_done_cb(void *data, Eio_File *handler)
{
   DBG("DONE");
}

static void
_file_error_cb(void *data, Eio_File *handler, int error)
{
   Ems_Directory *dir = data;

   ERR("Unable to parse %s\n", dir->path);
}

int
ems_scanner_init(void)
{

   return 1;
}

void
ems_scanner_shutdown(void)
{

}

void
ems_scanner_start(void)
{
   Eina_List *l;
   Ems_Directory *dir;

   INF("Scanning videos directories :");
   EINA_LIST_FOREACH(ems_config->video_directories, l, dir)
     {
        INF("Scanning %s: %s", dir->label, dir->path);
        eio_file_direct_ls(dir->path,
                           _file_filter_cb,
                           _file_main_cb,
                           _file_done_cb,
                           _file_error_cb,
                           dir);
     }
}
