#ifndef DATA_H
#define DATA_H

#include <QVector>
#include <QMap>
#include <cdio/cdio.h>

/* All these classes reprensent the data structures stored
 * in the persistent database (SQlite).
 *
 * /!\ IMPORTANT
 * -------------
 * We choose not to stored the whole database in RAM as Sqlite
 * do it for us. These data classes are just used to gather the
 * data returned by the "Database" module. They are "volatile"
 * and computed for each Websocket request. So do not manipulate
 * huge amount of data using these structures. Instead, use the
 * SQLite engine in the Database module.
 * -------------
 * Here, the central data class is "Track". Each track instance
 * contain all its related data : File, Album, Genre, ...
 * For example, if you get only a class EMSAlbum, it will not be
 * linked to anything.
 */

class EMSArtist
{
public:
    unsigned long long id; /* Unique identifier (primary key in db) */
    QString name; /* Full name or pseudonyme of the artist */
    QString picture;
    EMSArtist()
    {
        id = 0;
    }
};

class EMSAlbum
{
public:
    unsigned long long id; /* Unique identifier (primary key in db) */
    QString name; /* Album title */
    QString cover;
    EMSAlbum()
    {
        id = 0;
    }
};

class EMSGenre
{
public:
    unsigned long long id; /* Unique identifier (primary key in db) */
    QString name; /* Genre label */
    QString picture;

    EMSGenre()
    {
        id = 0;
    }
};

/* Track structure can be used for :
 * TRACK_TYPE_DB : Database entry (all field filled)
 * TRACK_TYPE_EXTERNAL : File located outside the directory managed by EMS (eg. USB stick)
 * TRACK_TYPE_CDROM : CDROM track
 */
enum EMSTrackType { TRACK_TYPE_DB, TRACK_TYPE_EXTERNAL, TRACK_TYPE_CDROM } ;

class EMSTrack
{
public:
    EMSTrackType type;
    unsigned long long id; /* Unique identifier (primary key in db) */
    unsigned int position; /* Position in the album */
    QString name; /* Name of the track, could be empty */
    QString filename; /* Full path of _one_ file of this track. */
    QString sha1; /* Custom calculation of the SHA1 of the file. (Unique for each track) */
    QString format; /* Audio file format */
    unsigned long long sample_rate; /* Sample rate in Hz, could be 0 for some format */
    unsigned int duration; /* Duration of the track in seconds */
    QString format_parameters; /* Format specific data */

    /* Used for removing old files which are not in the disk anymore */
    unsigned long long lastscan;

    EMSAlbum album;

    QVector<EMSArtist> artists;
    QVector<EMSGenre> genres;

    EMSTrack()
    {
        type = TRACK_TYPE_DB;
        id = 0;
        position = 0;
        sample_rate = 0;
        duration = 0;
        lastscan = 0;
    }
};

/* Playlist
 * -------------------
 * A playlist just have a name. The picture of a playlist is
 * computed using its content (cover of the tracks).
 * They can be placed into a sub-directory (= category). If subdir
 * is null, the playlist is not in a subdirectory (= root)
 */
class EMSPlaylist
{
public:
    unsigned long long id; /* Unique identifier (primary key in db) */
    QString name; /* The name "current" is reserved for the Player playlist */
    QString subdir;

    QVector<EMSTrack> tracks;
};

/* Player data
 * -------------------
 */
enum EMSPlayerState { STATUS_UNKNOWN, STATUS_STOP, STATUS_PLAY, STATUS_PAUSE } ;
enum EMSPlayerAction { ACTION_ADD, ACTION_DEL, ACTION_DEL_ALL, ACTION_LIST,
                       ACTION_STOP, ACTION_PLAY, ACTION_PLAY_POS, ACTION_PAUSE, ACTION_TOGGLE,
                       ACTION_NEXT, ACTION_PREV, ACTION_REPEAT, ACTION_RANDOM, ACTION_SEEK,
                       ACTION_ENABLE_OUTPUT, ACTION_DISABLE_OUTPUT } ;

class EMSSndCard
{
public:
    QString name;
    QString type;
    unsigned int id_system;
    unsigned int id_mpd;
    unsigned int priority;
    bool enabled;
    bool present;

    EMSSndCard()
    {
        enabled = false;
        id_mpd = 1;
        id_system = 0;
        priority = 0;
        present = false;
    }
};

class EMSPlayerStatus
{
public:
    EMSPlayerState state;
    int posInPlaylist; /* Position in playlist of the current track */
    unsigned int progress; /* Progression in second */
    bool repeat;
    bool random;

    /* Default value */
    EMSPlayerStatus()
    {
        state = STATUS_UNKNOWN;
        posInPlaylist = -1;
        progress = 0;
        repeat = false;
        random = false;
    }
};

class EMSPlayerCmd
{
public:
    EMSPlayerAction action;

    /* Track data for action related to one track (eg. ACTION_ADD)
     * If not used, let it uninitialized.
     */
    EMSTrack track;

    /* For action related to a boolean, as for changing "repeat" or
     * "random" option, we use the following variable instead.
     */
    bool boolValue;

    /* For activating/disabling a SndCard */
    unsigned int uintValue;
};

/* CdromManager data
 * -------------------
 */
class EMSCdrom
{
public:
    /* Name of the device (eg. /dev/sr0). This data is also inserted in each track */
    QString device;

    /* Disc id */
    QString disc_id;

    /* A CDROM is a list of track. For each track, available data are :
     * - filename = the name of the device (eg. /dev/sr0)
     * - position = the track number starting from 1
     * - name = the name to be displayed (eg. Track 01)
     * - duration = the duration of the track
     * - lsnBegin = the index of the first sector
     * - lsnEnd = the index of the last sector
     * Other data may be missing.
     */
    QVector<EMSTrack> tracks;

    /* Indexes of the first and last sector for each track */
    QMap<unsigned int, QPair<lsn_t, lsn_t> > trackSectors;
};

/* CdromManager data
 * -------------------
 */
class EMSClient
{
public:
    QString uuid;
    QString hostname;
    QString username;
};

class EMSRipProgress
{
public:
    unsigned int overall_progress; /* rip progress for the entire disk (in percent) */
    unsigned int track_in_progress; /* index of the track currently ripped */
    unsigned int track_progress; /* rip progress for the current track (in percent) */
    unsigned int track_total; /* total number of track being ripped */
    EMSRipProgress()
    {
        overall_progress = 0;
        track_in_progress = 0;
        track_progress = 0;
        track_total = 0;
    }
};
#endif // DATA_H
