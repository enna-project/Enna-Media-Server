#ifndef DEFAULTSETTINGS_H
#define DEFAULTSETTINGS_H

/* You can redefine all these value in your user setting file
 * in ~/.config/Enna/EnnaMediaServer.conf
 */

#define EMS_WEBSOCKET_PORT 7337
#define EMS_HTTP_PORT 7336

/* DATABASE
 * --------- */
// database/path
#define EMS_DATABASE_PATH "database.sqlite"
// database/create_script
#define EMS_DATABASE_CREATE_SCRIPT EMS_INSTALL_PREFIX "/share/ems/database.sql"
// database/version
#define EMS_DATABASE_VERSION 1

/* PLAYER
 * --------- */
// player/host
// Here you can set an IP address or a UNIX socket
// IMPORTANT: you must use either a socket or a patched MPD to use file:// protocol
#define EMS_MPD_IP "/run/mpd/socket"
// player/port
#define EMS_MPD_PORT 6600
// player/timeout
#define EMS_MPD_TIMEOUT 5000
// player/status_period
#define EMS_MPD_STATUS_PERIOD 1000
// player/retry_period
#define EMS_MPD_CONNECTION_RETRY_PERIOD 10000
// player/password (optional - if no password, let it empty)
#define EMS_MPD_PASSWORD ""

/* Scanner */
//File extensions used by the scanner to detect new files
#define EMS_MUSIC_EXTENSIONS "*.flac, *.wav, *.dsf, *.dff, *.mp3, *.ogg"

#ifdef Q_OS_MAC
#define EMS_DIRECTORIES_BASE_PATH "/Volumes"
#else
#define EMS_DIRECTORIES_BASE_PATH "/media"
#endif

#define EMS_LOAD_SETTINGS(var, confValue, defaultValue, type) \
do {\
if (settings.contains(confValue))\
{\
    var = settings.value(confValue).to##type();\
}\
else\
{\
    settings.setValue(confValue, defaultValue);\
    var = defaultValue;\
}\
}while (0);


#endif // DEFAULTSETTINGS_H
