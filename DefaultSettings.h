#ifndef DEFAULTSETTINGS_H
#define DEFAULTSETTINGS_H

/* You can redefine all these value in your user setting file
 * in ~/.config/Enna/EnnaMediaServer.conf
 */

#define EMS_WEBSOCKET_PORT 7337

/* DATABASE
 * --------- */
// database/path
#define EMS_DATABASE_PATH "database.sqlite"
// database/create_script
#define EMS_DATABASE_CREATE_SCRIPT "database.sql"
// database/version
#define EMS_DATABASE_VERSION 1

/* PLAYER
 * --------- */
#define EMS_MPD_IP "127.0.0.1"
#define EMS_MPD_PORT 6600
#define EMS_MPD_TIMEOUT 5000

#endif // DEFAULTSETTINGS_H
