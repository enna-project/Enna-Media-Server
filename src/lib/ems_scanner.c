#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include <Eio.h>

#include "ems_private.h"
#include "ems_scanner.h"
#include "ems_config.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

typedef struct _Ems_Scanner Ems_Scanner;

struct _Ems_Scanner
{
   int is_running;
   Ecore_Timer *schedule_timer;
   Eina_List *scan_files;
   int progress;
};

static Eina_Bool _file_filter_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info);
static void _file_main_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info);
static void _file_done_cb(void *data, Eio_File *handler);
static void _file_error_cb(void *data, Eio_File *handler, int error);

static Ems_Scanner *_scanner = NULL;

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
        DBG("[DIR] %s", info->path);
     }
   else
     {
        INF("[FILE] %s", info->path);
        /* TODO : add this file only if it doesn't exists in the db */
        _scanner->scan_files = eina_list_append(_scanner->scan_files,
                                                eina_stringshare_add(info->path));
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
        const char *f;

	/* Schedule the next scan */
	if (_scanner->schedule_timer)
	  ecore_timer_del(_scanner->schedule_timer);
	if (ems_config->scan_period)
	  {
	     _scanner->schedule_timer = ecore_timer_add(ems_config->scan_period, _schedule_timer_cb, NULL);
	     /* TODO: covert time into a human redeable value */
	     INF("Scan finished, schedule next scan in %d seconds", ems_config->scan_period);
	  }
	else
	  {
	     INF("Scan finished, scan schedule disabled according to the configuration.");
	  }

        /* Free the scan list */
        EINA_LIST_FREE(_scanner->scan_files, f)
          eina_stringshare_del(f);
        _scanner->scan_files = NULL;
        _scanner->progress = 0;
     }
}

static void
_file_error_cb(void *data, Eio_File *handler, int error)
{
   const char *path= data;
   /* _scanner->is_running--; */
   ERR("Unable to parse %s", path);
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
ems_scanner_init(void)
{
   _scanner = calloc(1, sizeof(Ems_Scanner));
   if (!_scanner)
     return EINA_FALSE;

   return EINA_TRUE;
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

   /* TODO : get all files in the db and see if they exist on the disk */

   /* Scann all files on the disk */
   INF("Scanning videos directories :");
   EINA_LIST_FOREACH(ems_config->video_directories, l, dir)
     {
        INF("Scanning %s: %s", dir->label, dir->path);
        eio_dir_stat_ls(dir->path,
                        _file_filter_cb,
                        _file_main_cb,
                        _file_done_cb,
                        _file_error_cb,
                        eina_stringshare_add(dir->path));
     }
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

void
ems_scanner_state_get(Ems_Scanner_State *state, double *percent)
{

   if (!_scanner)
     return;

   if (_scanner->is_running && state)
     *state = EMS_SCANNER_STATE_RUNNING;
   else if (state)
     *state = EMS_SCANNER_STATE_IDLE;

   if (eina_list_count(_scanner->scan_files) && percent)
     *percent = (double) _scanner->progress / eina_list_count(_scanner->scan_files);
   else if (percent)
     *percent = 0.0;

   INF("Scanner is %s (%3.3f%%)", _scanner->is_running ? "running" : "in idle", (double) _scanner->progress * 100.0 / eina_list_count(_scanner->scan_files));

}

