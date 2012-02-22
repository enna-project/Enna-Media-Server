#ifndef _EMS_H_
#define _EMS_H_

typedef enum _Ems_Error Ems_Error;

enum _Ems_Error
{
    EMS_OK = 0,
    EMS_GENERIC_ERROR = 1,
};

typedef enum _Ems_Media_Type Ems_Media_Type;
typedef struct _Ems_Server Ems_Server;
typedef struct _Ems_Player Ems_Player;
typedef struct _Ems_Observer Ems_Observer;
typedef struct _Ems_Collection Ems_Collection;
typedef struct _Ems_Media Ems_Media;
typedef struct _Ems_Media_Info Ems_Media_Info;
typedef struct _Ems_Server_Dir Ems_Server_Dir;

typedef void (*Ems_Server_Add_Cb)(void *data, Ems_Server *server);
typedef void (*Ems_Server_Del_Cb)(void *data, Ems_Server *server);
typedef void (*Ems_Server_Update_Cb)(void *data, Ems_Server *server);
typedef void (*Ems_Player_Add_Cb)(void *data, Ems_Player *player);
typedef void (*Ems_Player_Del_Cb)(void *data, Ems_Player *player);
typedef void (*Ems_Player_Update_Cb)(void *data, Ems_Player *player);

typedef void (*Ems_Media_Add_Cb)(void *data, Ems_Server *server,
                                 Eina_Iterator *it);
typedef void (*Ems_Media_Del_Cb)(void *data, Ems_Server *server,
                                 Eina_Iterator *it);
typedef void (*Ems_Media_Done_Cb)(void *data, Ems_Server *server);

typedef void (*Ems_Media_Error_Cb)(void *data, Ems_Server *server,
                                   Ems_Error error);
typedef void (*Ems_Media_Info_Add_Cb)(void *data, Ems_Server *server,
                                      Ems_Media *media);
typedef void (*Ems_Media_Info_Del_Cb)(void *data, Ems_Server *server,
                                      Ems_Media *media);
typedef void (*Ems_Media_Info_Update_Cb)(void *data, Ems_Server *server,
                                         Ems_Media *media);


int ems_init(void);
int ems_shutdown(void);

void ems_run(void);

/*
 * Get the list of servers detected with avahi/bonjour
 * Returns an Eina_List of Ems_Server or NULL if none is detected
 */
Eina_List *ems_server_list_get(void);
/*
 * Get the list of players detected with avahi/bonjour
 * Returns an Eina_List of Ems_Client or NULL if none is detected
 */
Eina_List *ems_player_list_get(void);

/* Set callbacks to be informed when a server is added/deleted/update */
void ems_server_cb_set(Ems_Server_Add_Cb server_add_cb,
                       Ems_Server_Del_Cb server_del_cb,
                       Ems_Server_Update_Cb server_update_cb,
                       void *data);

/* Unset callbacks */
void ems_server_cb_del(Ems_Server_Add_Cb server_add_cb,
                       Ems_Server_Del_Cb server_del_cb,
                       Ems_Server_Update_Cb server_uddate_cb);

/* Set callbacks to be informed when a player is added/deleted/update */
void ems_player_cb_set(Ems_Player_Add_Cb player_add_cb,
                       Ems_Player_Del_Cb player_del_cb,
                       Ems_Player_Update_Cb player_update_cb,
                       void *data);
/* Unset callbacks */
void ems_player_cb_del(Ems_Player_Add_Cb player_add_cb,
                       Ems_Player_Del_Cb player_del_cb,
                       Ems_Player_Update_Cb player_uddate_cb);

/* No need to be asyncrhonous here as name, icons, local and type are already set */

/* Get the name of the specified server, returns a stringshared name */
const char *ems_server_name_get(Ems_Server *server);

/* Get the icon's uri of the specified server, returns a stringshared uri*/
const char *ems_server_icon_get(Ems_Server *server);

/* Get the name of the specified player, returns a stringshared name */
const char *ems_player_name_get(Ems_Player *player);

/* Get the icon's uri of the specified player, returns a stringshared uri*/
const char *ems_player_icon_get(Ems_Player *player);

/* Return EINA_TRUE if the specified server is a local one, EINA_FALSE otherwise */
Eina_Bool ems_server_is_local(Ems_Server *server);

/* Return EINA_TRUE if the specified player is a local one, EINA_FALSE otherwise */
Eina_Bool ems_player_is_local(Ems_Player *player);

/* Return the type of the specified player */
Ems_Media_Type ems_player_type_get(Ems_Player *player);

/*
 * Get the list of files and directories contains in path on the specified server 
 * If path is NULL returns the root directories
 * Filter file by type
 * Return an error if browse is not allowed. 
 */
void ems_media_observer_del(Ems_Observer *obs);

Ems_Observer *ems_server_dir_get(Ems_Server *server,
                                 const char *path,
                                 Ems_Media_Type type,
                                 Ems_Media_Add_Cb media_add,
                                 Ems_Media_Del_Cb media_del,
                                 Ems_Media_Done_Cb media_done,
                                 Ems_Media_Error_Cb media_error,
                                 void *data);

/* Generic Api to create filters and searches :
 * Examples :
 * get all tvshows :
 * collection = ems_collection_new(EMS_MEDIA_TYPE_TVSHOW, NULL);
 * get all seasons of Dexter :
 * collection = ems_collection_new(EMS_MEDIA_TYPE_TVSHOW, "tvshow", "Dexter", NULL);
 */
Ems_Collection *ems_collection_new(Ems_Media_Type type, ...);
void ems_collection_free(Ems_Collection *collection);


/* Get The list of movies on the specified server */
Ems_Observer *ems_server_media_get(Ems_Server *server,
                                   Ems_Media_Type type,
                                   Ems_Collection *collection,
                                   Ems_Media_Add_Cb media_add,
                                   Ems_Media_Del_Cb media_del,
                                   Ems_Media_Done_Cb media_done,
                                   Ems_Media_Error_Cb media_error,
                                   void *data);



/* Get the info propertiy of a media on the specified server */
Ems_Observer *ems_server_media_info_get(Ems_Server *server,
                                        Ems_Media *media,
                                        const char *info,
                                        Ems_Media_Info_Add_Cb info_add,
                                        Ems_Media_Info_Add_Cb info_del,
                                        Ems_Media_Info_Update_Cb info_update,
                                        void *data);
void ems_media_info_observer_del(Ems_Observer *obs);

/* Set the info of a media on the specified server */
void ems_server_media_info_set(Ems_Server *server, Ems_Media *media, Ems_Media_Info *info, const char *value);

void ems_player_play(Ems_Player *player);
void ems_player_pause(Ems_Player *player);
void ems_player_stop(Ems_Player *player);
void ems_player_next(Ems_Player *player);
void ems_player_prev(Ems_Player *player);

void ems_player_playlist_add(Ems_Player *player, Ems_Media *media);
void ems_player_playlist_del(Ems_Player *player, Ems_Media *media);
Eina_List *ems_player_playlist_get(Ems_Player *player);

/* More ideas */
void ems_player_synchronise(Ems_Player *master, Ems_Player *slave);
void ems_player_desyncronise(Ems_Player *slave);

#endif /* _EMS_H_ */
