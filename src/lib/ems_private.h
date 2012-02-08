
#include <Eina.h>

/*
 * variable and macros used for the eina_log module
 */
extern int _ems_log_dom_global;

/*
 * Macros that are used everywhere
 *
 * the first four macros are the general macros for the lib
 */
#ifdef EMS_DEFAULT_LOG_COLOR
# undef EMS_DEFAULT_LOG_COLOR
#endif /* ifdef EMS_DEFAULT_LOG_COLOR */
#define EMS_DEFAULT_LOG_COLOR EINA_COLOR_CYAN
#ifdef ERR
# undef ERR
#endif /* ifdef ERR */
#define ERR(...)  EINA_LOG_DOM_ERR(_ems_log_dom_global, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif /* ifdef DBG */
#define DBG(...)  EINA_LOG_DOM_DBG(_ems_log_dom_global, __VA_ARGS__)
#ifdef INF
# undef INF
#endif /* ifdef INF */
#define INF(...)  EINA_LOG_DOM_INFO(_ems_log_dom_global, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif /* ifdef WRN */
#define WRN(...)  EINA_LOG_DOM_WARN(_ems_log_dom_global, __VA_ARGS__)
#ifdef CRIT
# undef CRIT
#endif /* ifdef CRIT */
#define CRIT(...) EINA_LOG_DOM_CRIT(_ems_log_dom_global, __VA_ARGS__)



#define EMS_CONFIG_FILE "enna-media-server.cfg"
#define EMS_PORT 1337

#define ENNA_CONFIG_DD_NEW(str, typ)                    \
  ems_config_descriptor_new(str, sizeof(typ))
#define ENNA_CONFIG_DD_FREE(eed) if (eed) { eet_data_descriptor_free((eed)); (eed) = NULL; }
#define ENNA_CONFIG_VAL(edd, type, member, dtype) EET_DATA_DESCRIPTOR_ADD_BASIC(edd, type, #member, member, dtype)
#define ENNA_CONFIG_SUB(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_SUB(edd, type, #member, member, eddtype)
#define ENNA_CONFIG_LIST(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_LIST(edd, type, #member, member, eddtype)
#define ENNA_CONFIG_HASH(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_HASH(edd, type, #member, member, eddtype)


typedef struct _Ems_Config Ems_Config;
typedef struct _Ems_Directory Ems_Directory;
typedef struct _Ems_Extension Ems_Extension;

struct _Ems_Directory
{
   const char *path;
   const char *label;
};

struct _Ems_Extension
{
   const char *ext;
};

struct _Ems_Config
{
   Eina_List *video_extensions;
   Eina_List *video_directories;
};

extern Ems_Config *ems_config;
