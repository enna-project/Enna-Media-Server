xdele#include <Eio.h>
#include <Ecore.h>

#include "ems_private.h"
#include "ems_scanner.h"
#include "ems_config.h"

typedef struct _Ems_Scanner Ems_Scanner;

struct _Ems_Scanner
{
   Eina_Bool is_running;
   Ecore_Timer *schedule_timer;
};

static Eina_Bool _file_filter_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info);
static void _file_main_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info);
static void _file_done_cb(void *data, Eio_File *handler);
static void _file_error_cb(void *data, Eio_File *handler, int error);

static Ems_Scanner *_scanner;

static Eina_Bool
_schedule_timer_cb(void *data)
{

   ems_scanner_start();

   _scanner->schedule_timer = NULL;

   return EINA_FALSE;
}

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
   if (info->type == EINA_FILE_DIR)
     {
        DBG("Scanning : %s", info->path);
        _scanner->is_running++;
        eio_file_direct_ls(info->path,
                           _file_filter_cb,
                           _file_main_cb,
                           _file_done_cb,
                           _file_error_cb,
                           eina_stringshare_add(info->path));
     }
   else
     {
        INF("Found %s", info->path);
        /* TODO: add this file in the database */
        /* TODO: Add this file in the scanner list */
     }
}

static void
_file_done_cb(void *data, Eio_File *handler)
{
   const char *path = data;
   eina_stringshare_del(path);
   _scanner->is_running--;

   if (!_scanner->is_running)
     {
        /* Schedule the next scan */
        if (_scanner->schedule_timer)
          ecore_timer_del(_scanner->schedule_timer);

        _scanner->schedule_timer = ecore_timer_add(10.0, _schedule_timer_cb, NULL);
        INF("Scan finished, schedule next scan in %3.3f seconds", 10.0);
     }
}


static void
_file_error_cb(void *data, Eio_File *handler, int error)
{
   const char *path= data;
   _scanner->is_running--;
   ERR("Unable to parse %s", path);
}

int
ems_scanner_init(void)
{
   _scanner = calloc(1, sizeof(Ems_Scanner));
   if (!_scanner)
     return 0;
   return 1;
}

void
ems_scanner_shutdown(void)
{
   if (_scanner)
     {
        if (_scanner->schedule_timer)
          ecore_timer_del(_scanner->schedule_timer);
        free(_scanner);
     }
}

void
ems_scanner_start(void)
{
   Eina_List *l;
   Ems_Directory *dir;

   if (!_scanner)
     {
        ERR("Init scanner first : ems_scanner_init()");
     }

   if (_scanner->is_running)
     {
        WRN("Scanner is already running, did you try to run the scanner twice ?");
        return;
     }

   _scanner->is_running++;

   INF("Scanning videos directories :");
   EINA_LIST_FOREACH(ems_config->video_directories, l, dir)
     {
        INF("Scanning %s: %s", dir->label, dir->path);
        eio_file_direct_ls(dir->path,
                           _file_filter_cb,
                           _file_main_cb,
                           _file_done_cb,
                           _file_error_cb,
                           eina_stringshare_add(dir->path));
     }
}
