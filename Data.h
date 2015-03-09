#ifndef DATA_H
#define DATA_H

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

/* Supported format */
enum EMSTrackFormat
{
    UNKNOWN,
    WAV,
    FLAC,
    RAW,
    AIFF,
    MP3
};

class EMSTrack
{
public:
    unsigned long id; /* Unique identifier (primary key in db) */
    unsigned int position; /* Position in the album */
    QString name; /* Name of the track, could be empty */
    QString filename; /* Full path of _one_ file of this track. */
    QByteArray sha1; /* Custom calculation of the SHA1 of the file. (Unique for each track) */
    EMSTrackFormat format; /* Audio file format */
    unsigned long sample_rate; /* Sample rate in Hz, could be 0 for some format */
    unsigned int duration; /* Duration of the track in seconds */
    QString format_parameters; /* Format specific data */

    EMSArtist artist;
    EMSAlbum album;
    EMSGenre genre;
};

class EMSArtist
{
public:
    unsigned long id; /* Unique identifier (primary key in db) */
    QString name; /* Full name or pseudonyme of the artist */
};

class EMSAlbum
{
public:
    unsigned long id; /* Unique identifier (primary key in db) */
    QString name; /* Album title */
};

class EMSGenre
{
public:
    unsigned long id; /* Unique identifier (primary key in db) */
    QString name; /* Genre label */
};

/* Playlist management
 * -------------------
 * A playlist just have a name. The picture of a playlist is
 * computed using its content (cover of the tracks).
 * They can be placed into a sub-directory (= category). If subdir
 * is null, the playlist is not in a subdirectory (= root)
 */
class EMSPlaylist
{
    unsigned long id; /* Unique identifier (primary key in db) */
    QString name;
    QString subdir;
};

#endif // DATA_H
