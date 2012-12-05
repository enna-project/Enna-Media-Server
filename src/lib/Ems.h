#ifndef _EMS_H_
#define _EMS_H_

#ifdef __cplusplus
extern "C" {
#endif /* ifdef __cplusplus */

typedef enum _Ems_Error
{
   EMS_OK = 0,
   EMS_GENERIC_ERROR = 1,
} Ems_Error;

typedef enum _Ems_Media_Type
{
  EMS_MEDIA_TYPE_VIDEO  = 1 << 0,
  EMS_MEDIA_TYPE_MUSIC  = 1 << 1,
  EMS_MEDIA_TYPE_PHOTO  = 1 << 2,
  EMS_MEDIA_TYPE_TVSHOW = 1 << 3,
} Ems_Media_Type;

enum _Ems_Scanner_State
{
    EMS_SCANNER_STATE_IDLE,
    EMS_SCANNER_STATE_RUNNING,
};

typedef struct _Ems_Node Ems_Node;
typedef struct _Ems_Player Ems_Player;
typedef struct _Ems_Observer Ems_Observer;
typedef struct _Ems_Collection Ems_Collection;
typedef struct _Ems_Media Ems_Media;
typedef struct _Ems_Media_Info Ems_Media_Info;
typedef struct _Ems_Node_Dir Ems_Node_Dir;

typedef void (*Ems_Node_Add_Cb)(void *data, Ems_Node *node);
typedef void (*Ems_Node_Del_Cb)(void *data, Ems_Node *node);
typedef void (*Ems_Node_Update_Cb)(void *data, Ems_Node *node);
typedef void (*Ems_Node_Connected_Cb)(void *data, Ems_Node *node);
typedef void (*Ems_Node_Disconnected_Cb)(void *data, Ems_Node *node);

typedef void (*Ems_Player_Add_Cb)(void *data, Ems_Player *player);
typedef void (*Ems_Player_Del_Cb)(void *data, Ems_Player *player);
typedef void (*Ems_Player_Update_Cb)(void *data, Ems_Player *player);

typedef void (*Ems_Media_Add_Cb)(void *data, Ems_Node *node,
                                 const char *media);
typedef void (*Ems_Media_Del_Cb)(void *data, Ems_Node *node,
                                 Eina_Iterator *it);
typedef void (*Ems_Media_Done_Cb)(void *data, Ems_Node *node);

typedef void (*Ems_Media_Error_Cb)(void *data, Ems_Node *node,
                                   Ems_Error error);
typedef void (*Ems_Media_Info_Add_Cb)(void *data, Ems_Node *node,
                                      const char *value);
typedef void (*Ems_Media_Info_Del_Cb)(void *data, Ems_Node *node,
                                      Ems_Media *media);
typedef void (*Ems_Media_Info_Update_Cb)(void *data, Ems_Node *node,
                                         Ems_Media *media);

typedef enum _Ems_Scanner_State Ems_Scanner_State;

int ems_init(const char *config_file);
int ems_shutdown(void);

void ems_run(void);

/*
 * Get the list of nodes detected with avahi/bonjour
 * Returns an Eina_List of Ems_Node or NULL if none is detected
 */
Eina_List *ems_node_list_get(void);
/*
 * Get the list of players detected with avahi/bonjour
 * Returns an Eina_List of Ems_Client or NULL if none is detected
 */
Eina_List *ems_player_list_get(void);

/* Set callbacks to be informed when a node is added/deleted/update */
void ems_node_cb_set(Ems_Node_Add_Cb node_add_cb,
                       Ems_Node_Del_Cb node_del_cb,
                       Ems_Node_Update_Cb node_update_cb,
                       Ems_Node_Connected_Cb node_connected_cb,
                       Ems_Node_Disconnected_Cb node_disconnected_cb,
                       void *data);

/* Unset callbacks */
void ems_node_cb_del(Ems_Node_Add_Cb node_add_cb,
                       Ems_Node_Del_Cb node_del_cb,
                       Ems_Node_Update_Cb node_uddate_cb);

/* Set callbacks to be informed when a player is added/deleted/update */
/* void ems_player_cb_set(Ems_Player_Add_Cb player_add_cb, */
/*                        Ems_Player_Del_Cb player_del_cb, */
/*                        Ems_Player_Update_Cb player_update_cb, */
/*                        void *data); */
/* /\* Unset callbacks *\/ */
/* void ems_player_cb_del(Ems_Player_Add_Cb player_add_cb, */
/*                        Ems_Player_Del_Cb player_del_cb, */
/*                        Ems_Player_Update_Cb player_update_cb); */

/* No need to be asyncrhonous here as name, icons, local and type are already set */

/* Get the name of the specified node, returns a stringshared name */
const char *ems_node_name_get(Ems_Node *node);

const char *ems_node_ip_get(Ems_Node *node);
int ems_node_port_get(Ems_Node *srever);

/* Get the icon's uri of the specified node, returns a stringshared uri*/
const char *ems_node_icon_get(Ems_Node *node);

/* Get the name of the specified player, returns a stringshared name */
const char *ems_player_name_get(Ems_Player *player);

/* Get the icon's uri of the specified player, returns a stringshared uri*/
const char *ems_player_icon_get(Ems_Player *player);

/* Return EINA_TRUE if the specified node is a local one, EINA_FALSE otherwise */
Eina_Bool ems_node_is_local(Ems_Node *node);

/* Return EINA_TRUE if the specified player is a local one, EINA_FALSE otherwise */
Eina_Bool ems_player_is_local(Ems_Player *player);

/* Return EINA_TRUE if we are connected to the specified player, EINA_FALSE otherwise */
Eina_Bool ems_node_is_connected(Ems_Node *node);

/* Try to connect to node, return EINA_TRUE if connection is in progress, EINA_FALSE if an error occured */
Eina_Bool ems_node_connect(Ems_Node *node);

/* Return the type of the specified player */
Ems_Media_Type ems_player_type_get(Ems_Player *player);

/*
 * Get the list of files and directories contains in path on the specified node
 * If path is NULL returns the root directories
 * Filter file by type
 * Return an error if browse is not allowed.
 */
void ems_media_obnode_del(Ems_Observer *obs);

Ems_Observer *ems_node_dir_get(Ems_Node *node,
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
Ems_Collection *ems_collection_new(Ems_Media_Type type, const char *arg, ...);
void ems_collection_free(Ems_Collection *collection);


/* Get The list of movies on the specified node */
Ems_Observer *ems_node_media_get(Ems_Node *node,
                                   Ems_Collection *collection,
                                   Ems_Media_Add_Cb media_add,
                                   void *data);



/* Get the info propertiy of a media on the specified node */
Ems_Observer *ems_node_media_info_get(Ems_Node *node,
                                        const char *uuid,
                                        const char *info,
                                        Ems_Media_Info_Add_Cb info_add,
                                        Ems_Media_Info_Add_Cb info_del,
                                        Ems_Media_Info_Update_Cb info_update,
                                        void *data);

void ems_media_info_observer_del(Ems_Observer *obs);

//Helper function to get a valid url from an Ems_Node and a media UUID
char *ems_node_media_stream_url_get(Ems_Node *node, const char *media_uuid);

/* Set the info of a media on the specified node */
void ems_node_media_info_set(Ems_Node *node, Ems_Media *media, Ems_Media_Info *info, const char *value);

void ems_player_play(Ems_Player *player);
void ems_player_pause(Ems_Player *player);
void ems_player_stop(Ems_Player *player);
void ems_player_next(Ems_Player *player);
void ems_player_prev(Ems_Player *player);

void ems_player_playlist_add(Ems_Player *player, Ems_Media *media);
void ems_player_playlist_del(Ems_Player *player, Ems_Media *media);
Eina_List *ems_player_playlist_get(Ems_Player *player);

void ems_scanner_start(void);
void ems_scanner_state_get(Ems_Scanner_State *state, double *percent);


Eina_Bool ems_avahi_start(void);


/* More ideas */
void ems_player_synchronise(Ems_Player *master, Ems_Player *slave);
void ems_player_desyncronise(Ems_Player *slave);

#ifdef __cplusplus
}
#endif /* ifdef __cplusplus */

#endif /* _EMS_H_ */
