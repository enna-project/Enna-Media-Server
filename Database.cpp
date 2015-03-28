#include "Database.h"
#include "DefaultSettings.h"
#include <QDebug>
#include <QFile>
#include <QSettings>

Database* Database::_instance = 0;

/*****************************************************************************
 *    FEED WITH NEW DATA (server-side only)
 ****************************************************************************/

bool Database::insertNewTrack(EMSTrack newTrack)
{
    if (!opened)
    {
        return false;
    }
    /* Use transaction to get "atomic" behavior in case of failure */
    QSqlQuery q(db);
    if (!q.exec("BEGIN;"))
    {
        return false;
    }

    /* Look for the artist in the db (use the "name" as key) */
    EMSArtist dbArtist;
    //    if (getArtistByName(&dbArtist, newTrack.artist.name))
    //    {
    //        newTrack.artist = dbArtist;
    //    }
    //    else /* Artist not found => create it */
    //    {
    //        q.prepare("INSERT INTO artists(name, cover) VALUES (?,?)");
    //        q.bindValue(0, newTrack.artist.name);
    //        q.bindValue(1, newTrack.artist.picture);
    //        if(!q.exec())
    //        {
    //            qCritical() << "Error while inserting new artist : " + q.lastError().text();
    //            q.exec("ROLLBACK");
    //            return false;
    //        }
    //        /* Retrieve the new id in the database */
    //        if (getArtistByName(&dbArtist, newTrack.artist.name))
    //        {
    //            newTrack.artist = dbArtist;
    //        }
    //        else
    //        {
    //            qCritical() << "Database error: cannot find artist name '" + name + "' which has been added.";
    //            q.exec("ROLLBACK");
    //            return false;
    //        }
    //    }

    /* Look for the album in the database */
    /* TODO: voir avec Nicolas comment on "identifie" un album si on considère le nom
     * comme non unique.
     */

    return true;
}

bool Database::searchExistingTrack(QString sha1, EMSTrack *existingTrack)
{
    return true;
}

/*****************************************************************************
 *    BROWSE DATABASE
 ****************************************************************************/
/* Templates of SQL queries which are used in the browsing functions.
 * Additional clauses are appended to them, this is why these strings
 * are not terminated with ';'.
 */
const QString select_track_data1 = \
"SELECT tracks.id, tracks.position, tracks.name, tracks.sha1, tracks.format, "
"       tracks.sample_rate, tracks.duration, tracks.format_parameters, "
"       albums.id, albums.name, albums.cover, "
"       files.filename "
"FROM   tracks, albums "
"WHERE  tracks.album_id = albums.id AND "
"       tracks.id = files.track_id ";

const QString select_artist_from_track_data1 = \
"SELECT tracks.id, artists.id, artists.name, artists.picture"
"FROM   tracks, artists, tracks_artists "
"WHERE  tracks.id = tracks_artists.track_id AND "
"       albums.id = tracks_artists.album_id ";

const QString select_genre_from_track_data1 = \
"SELECT tracks.id, genre.id, artists.name, artists.picture "
"FROM   tracks, artists, tracks_artists "
"WHERE  tracks.id = tracks_artists.track_id AND "
"       albums.id = tracks_artists.album_id ";

const QString select_artist_data1 = \
"SELECT id, name, picture "
"FROM artists ";

const QString select_genre_data1 = \
"SELECT id, name, picture "
"FROM genres ";


void Database::getAlbumList(QVector<EMSAlbum> *albumList) {}
void Database::getGenreList(QVector<EMSGenre> *genreList) {}


/*****************************************************************************
 *    BROWSING TRACKS
 ****************************************************************************/
/* Note: As precised in Data.h, the getTrack() function not only get the data
 * of a track but also the data of all related tables : album, genres, etc.
 */
bool Database::getTrackById(EMSTrack *track, unsigned long long trackId)
{
    if (!opened)
    {
        return false;
    }

    /* Get all possible data in one row */
    QSqlQuery q(db);
    q.prepare(select_track_data1 + " AND tracks.id = '?' LIMIT 1;");
    q.bindValue(0, trackId);
    if(!q.exec())
    {
        qCritical() << "Querying track data failed : " + q.lastError().text();
        return false;
    }
    unsigned int colId = 1;
    if (q.size() >= 1 && !q.next())
    {
        /* Follow the same order as in the SQL query */
        track->id = trackId;
        track->position = q.value(colId++).toUInt();
        track->name = q.value(colId++).toUInt();
        track->sha1 = QByteArray().fromHex(q.value(colId++).toByteArray());
        track->format = q.value(colId++).toString();
        track->sample_rate = q.value(colId++).toULongLong();
        track->duration = q.value(colId++).toUInt();
        track->format_parameters = q.value(colId++).toString();;
        track->album.id = q.value(colId++).toULongLong();
        track->album.name = q.value(colId++).toString();
        track->album.cover = q.value(colId++).toString();
        track->filename = q.value(colId++).toString(); /* Discard the other row with the LIMIT clause */
    }
    else
    {
        qDebug() << "The track ID " + QString("%1").arg(trackId) + " does not exist in database";
        return false;
    }

    /* Get the artists list */
    getArtistsByTrackId(&(track->artists), trackId);

    /* Get the genres list */
    getGenresByTrackId(&(track->genres), trackId);

    return true;
}

/* Get all the tracks in the database. This function will return almost all the database !
 * For performance purpose, this function execute three SQL queries :
 * 1- Get all the tracks data (one row per track) (ordered by track id)
 * 2- Get all artists data (ordered by track id)
 * 3- Get all genres data (ordered by track id)
 * For each iteration of 2- and 3-, a O(1) operation is made for matching the associated
 * track structure.
 */
void Database::getTracks(QVector<EMSTrack> *tracksList)
{
    if (!opened)
    {
        return;
    }

    /* STEP 1 :
     * Get all the tracks data (one row per track) (ordered by track id)
     * Reminder (vdehors): what happend in the column filename if there is
     *                     two filenames for one track.id ? => TOCHECK
     */
    QSqlQuery q(db);
    q.prepare(select_track_data1 + " ORDER BY track.id GROUP BY track.id;");
    if(!q.exec())
    {
        qCritical() << "Querying tracks list failed : " + q.lastError().text();
        return;
    }
    tracksList->clear();
    while (q.next())
    {
        /* Follow the same order as in the SQL query */
        EMSTrack track;
        unsigned int colId = 0;
        track.id = q.value(colId++).toULongLong();
        track.position = q.value(colId++).toUInt();
        track.name = q.value(colId++).toUInt();
        track.sha1 = QByteArray().fromHex(q.value(colId++).toByteArray());
        track.format = q.value(colId++).toString();
        track.sample_rate = q.value(colId++).toULongLong();
        track.duration = q.value(colId++).toUInt();
        track.format_parameters = q.value(colId++).toString();;
        track.album.id = q.value(colId++).toULongLong();
        track.album.name = q.value(colId++).toString();
        track.album.cover = q.value(colId++).toString();
        track.filename = q.value(colId++).toString();
        tracksList->append(track);
    }

    /* If no track, return now. */
    if (tracksList->size() == 0)
    {
        return;
    }

    /* STEP 2 :
     * Get all artists data (ordered by track id)
     * Note that the list of track is ORDERED by track id.
     */
    q.prepare(select_artist_from_track_data1 + " ORDER BY track.id;");
    if(!q.exec())
    {
        qCritical() << "Querying tracks list (artists) failed : " + q.lastError().text();
        return;
    }
    int currentListId = 0;
    unsigned long long currentTrackId;
    while (q.next())
    {
        // tracks.id, artists.id, artists.name, artists.picture
        currentTrackId = q.value(0).toULongLong();
        if(currentTrackId != tracksList->at(currentListId).id)
        {
            currentTrackId++;
        }
        if(currentListId >= tracksList->size())
        {
            qCritical() << "Unable to match tracks.id with the last query.";
            break;
        }

        EMSArtist artist;
        artist.id = q.value(1).toULongLong();
        artist.name = q.value(2).toString();
        artist.picture = q.value(3).toString();
        tracksList->at(currentListId).artists.append(artist);
    }

    /* STEP 3 :
     * Get all genres data (ordered by track id)
     * Note that the list of track is ORDERED by track id.
     */
    q.prepare(select_genre_from_track_data1 + " ORDER BY track.id;");
    if(!q.exec())
    {
        qCritical() << "Querying tracks list (genres) failed : " + q.lastError().text();
        return;
    }
    currentListId = 0;
    while (q.next())
    {
        // tracks.id, genres.id, genres.name, genres.picture
        currentTrackId = q.value(0).toULongLong();
        if(currentTrackId != tracksList->at(currentListId).id)
        {
            currentTrackId++;
        }
        if(currentListId < tracksList->size())
        {
            qCritical() << "Unable to match tracks.id with the last query.";
            break;
        }

        EMSGenre genre;
        genre.id = q.value(1).toULongLong();
        genre.name = q.value(2).toString();
        genre.picture = q.value(3).toString();
        tracksList->at(currentListId).genres.append(genre);
    }
}

void Database::getTracksByArtist(QVector<EMSTrack> *tracksList, unsigned long long artistId) {}
void Database::getTracksByAlbum(QVector<EMSTrack> *tracksList, unsigned long long albumId) {}
void Database::getTracksByGenre(QVector<EMSTrack> *tracksList, unsigned long long genreId) {}

/*****************************************************************************
 *    BROWSING ARTISTS
 ****************************************************************************/
bool Database::getArtistById(EMSArtist *artist, unsigned long long artistId)
{
    if (!opened)
    {
        return false;
    }
    QSqlQuery q(db);
    q.prepare(select_artist_data1 + " WHERE id='?'");
    q.bindValue(0, artistId);
    q.exec();
    if (q.size() >= 1 && !q.next())
    {
        if (q.size() > 1)
        {
            qCritical() << "Database consistency error: several artist with ID " + QString().arg(artistId);
        }
        artist->id = q.value(0).toULongLong();
        artist->name = q.value(1).toString();
        artist->picture = q.value(2).toString();
        return true;
    }
    return false;
}

bool Database::getArtistByName(EMSArtist *artist, QString name)
{
    if (!opened)
    {
        return false;
    }
    QSqlQuery q(db);
    q.prepare(select_artist_data1 + " WHERE name='?'");
    q.bindValue(0, name);
    q.exec();
    if (q.size() >= 1 && !q.next())
    {
        if (q.size() > 1)
        {
            qCritical() << "Database consistency error: several artist with name " + name;
        }
        artist->id = q.value(0).toULongLong();
        artist->name = q.value(1).toString();
        artist->picture = q.value(2).toString();
        return true;
    }
    return false;
}

void Database::getArtistsByTrackId(QVector<EMSArtist> *artistsList, unsigned long long trackId)
{
    if (!opened)
    {
        return;
    }
    QSqlQuery q(db);
    q.prepare(select_artist_from_track_data1 + " AND tracks.id = '?';");
    q.bindValue(0, trackId);
    if(!q.exec())
    {
        qCritical() << "Querying track data failed : " + q.lastError().text();
        return;
    }
    artistsList->clear();
    while (q.next())
    {
        // tracks.id, artists.id, artists.name, artists.picture
        EMSArtist artist;
        artist.id = q.value(1).toULongLong();
        artist.name = q.value(2).toString();
        artist.picture = q.value(3).toString();
        artistsList->append(artist);
    }
}

/*****************************************************************************
 *    BROWSING ALBUMS
 ****************************************************************************/
bool Database::getAlbumById(EMSAlbum *album, unsigned long long albumId) {}

/*****************************************************************************
 *    BROWSING GENRES
 ****************************************************************************/
bool Database::getGenreById(EMSGenre *genre, unsigned long long genreId)
{
    if (!opened)
    {
        return false;
    }
    QSqlQuery q(db);
    q.prepare(select_genre_data1 + " WHERE id='?'");
    q.bindValue(0, genreId);
    q.exec();
    if (q.size() >= 1 && !q.next())
    {
        if (q.size() > 1)
        {
            qCritical() << "Database consistency error: several genre with ID " + QString("%1").arg(name);
        }
        genre->id = q.value(0).toULongLong();
        genre->name = q.value(1).toString();
        genre->picture = q.value(2).toString();
        return true;
    }
    return false;
}

bool Database::getGenreByName(EMSGenre *genre, QString name)
{
    if (!opened)
    {
        return false;
    }
    QSqlQuery q(db);
    q.prepare(select_genre_data1 + " WHERE name='?'");
    q.bindValue(0, name);
    q.exec();
    if (q.size() >= 1 && !q.next())
    {
        if (q.size() > 1)
        {
            qCritical() << "Database consistency error: several genre with name " + name;
        }
        genre->id = q.value(0).toULongLong();
        genre->name = q.value(1).toString();
        genre->picture = q.value(2).toString();
        return true;
    }
    return false;
}

void Database::getGenresByTrackId(QVector<EMSGenre> *genresList, unsigned long long trackId)
{
    if (!opened)
    {
        return;
    }
    QSqlQuery q(db);
    q.prepare(select_genre_from_track_data1 + " AND tracks.id = '?';");
    q.bindValue(0, trackId);
    if(!q.exec())
    {
        qCritical() << "Querying track data failed : " + q.lastError().text();
        return;
    }
    genresList->clear();
    while (q.next())
    {
        // tracks.id, artists.id, artists.name, artists.picture
        EMSGenre genre;
        genre.id = q.value(1).toULongLong();
        genre.name = q.value(2).toString();
        genre.picture = q.value(3).toString();
        genresList->append(artist);
    }
}

/*****************************************************************************
 *    OPEN/CLOSE - CONSTRUCTOR/DESCTRUCTOR
 *    DATABASE MANAGEMENT
 ****************************************************************************/
bool Database::open()
{
    QSettings settings;

    /* If database is already opened, do nothing... */
    if (opened)
    {
        qDebug() << "Asked to open the database but it is already opened.";
        return true;
    }

    /* Database init with SQLite driver */
    if (!QSqlDatabase::drivers().contains("QSQLITE"))
    {
        qCritical() << "Unable to load database, SQlite driver is needed";
        return false;
    }

    /* Look for the path of the database */
    dbSettingPath = settings.value("database/path", EMS_DATABASE_PATH).toString();
    dbSettingCreateScript = settings.value("database/create_script", EMS_DATABASE_CREATE_SCRIPT).toString();
    dbSettingVersion = settings.value("database/version", EMS_DATABASE_VERSION).toUInt();

    /* Try to open the database
     * If the file does not exist, an empty database is created
     */
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbSettingPath);
    if (!db.open())
    {
        qCritical() << "Unable to open database " + dbSettingPath;
        return false;
    }

    /* Check version of current db */
    version = 0;
    QStringList tables = db.tables();
    if (tables.contains("configuration", Qt::CaseSensitive))
    {
        QSqlQuery query(db);
        query.exec("SELECT config_value FROM configuration WHERE config_name='version'");
        if (query.size() == 1 && !query.next())
        {
            version = query.value(0).toUInt();
        }
        //For the future: execute a upgrade script
        //if (version < dbVersion)
    }
    else
    {
        if(!createSchema(dbSettingCreateScript))
        {
            qCritical() << "Database creation has failed using the script " + dbSettingCreateScript;
            return false;
        }
        version = dbSettingVersion;
    }
    opened = true;
    return true;
}

void Database::close()
{
    db.close();
    opened = false;
}

/* Execute a .SQL file
 * This function is used for schema creation.
 * Beware with this function. We parse ';' to split the queries
 * So the script must not contain ';' character in it.
 * Do NOT use this script if you don't know the content of "filePath"
 */
bool Database::createSchema(QString filePath)
{
    QFile schemaFile(filePath);
    if(!schemaFile.open(QFile::ReadOnly))
    {
        return false;
    }

    QStringList schemaTableList = QString(schemaFile.readAll()).split(";");
    foreach(const QString schemaTable, schemaTableList)
    {
        if(!schemaTable.trimmed().isEmpty())
        {
            QSqlQuery q(db);
            if(!q.exec(schemaTable))
            {
                qCritical() << "Error while executing query : " + schemaTable;
                qCritical() << "Error : " + q.lastError().text();
                return false;
            }
        }
    }

    schemaFile.close();
    return true;
}

Database::Database(QObject *parent) : QObject(parent)
{
    opened = false;
}

Database::~Database()
{
    if (opened)
    {
        close();
    }
}


