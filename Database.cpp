#include "Database.h"
#include "DefaultSettings.h"
#include <QDebug>
#include <QFile>
#include <QSettings>

Database* Database::_instance = 0;

/*****************************************************************************
 *    FEED WITH NEW DATA (server-side only)
 ****************************************************************************/
/* Add a new track in the database.
 * You must fill all data about this track, including
 * - The album (WITH the ID) => So you need to create the album first,
 *                              Set id=0 if there is no associated album
 * - The artists (without the id)
 * - The genres (without the id)
 * - The filename and other required track data
 * If a genre or an artist does not exist in the database, it will be created.
 * The argument newTrack will be updated with the corresponding database ID (
 * for track, artist and genre).
 * TODO: do not do select queries to retrieve inserted values, it takes time.
 */
bool Database::insertNewTrack(EMSTrack *newTrack)
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

    EMSTrack dbTrack;
    /* Check if the track already exists in the database */
    if (getTrackBySha1(&dbTrack, newTrack->sha1))
    {
        if (newTrack->filename != dbTrack.filename)
        {
            if(!insertNewFilename(newTrack->filename, dbTrack.id))
            {
                qCritical() << "Error while inserting new track : " + q.lastError().text();
                q.exec("ROLLBACK;");
                return false;
            }
        }
        /* Track already exists and the new sha1 has been added */
        *newTrack = dbTrack;
        return true;
    }
    else
    {
        /* Insert the new track */
        q.prepare("INSERT INTO tracks "
                  "  (album_id, position, name, sha1, format, sample_rate, duration, format_parameters) "
                  "VALUES "
                     "(?,?,?,?,?,?,?,?);");
        q.bindValue(0, newTrack->album.id);
        q.bindValue(1, newTrack->position);
        q.bindValue(2, newTrack->name);
        q.bindValue(3, newTrack->sha1);
        q.bindValue(4, newTrack->format);
        q.bindValue(5, newTrack->sample_rate);
        q.bindValue(6, newTrack->duration);
        q.bindValue(7, newTrack->format_parameters);
        if(!q.exec())
        {
            qCritical() << "Error while inserting new track : " + q.lastError().text();
            q.exec("ROLLBACK;");
            return false;
        }

        /* Retrieve the new ID */
        if (!getTrackBySha1(&dbTrack, newTrack->sha1))
        {
            qCritical() << "Cannot retrieve the track ID for SHA1 : " + newTrack->sha1;
            q.exec("ROLLBACK;");
            return false;
        }

        /* Insert new filename */
        if(!insertNewFilename(newTrack->filename, dbTrack.id))
        {
            qCritical() << "Error while inserting new filename for track ID : " + QString("%1").arg(dbTrack.id);
            q.exec("ROLLBACK;");
            return false;
        }

        /* Update data in EMSTrack */
        dbTrack.filename = newTrack->filename;
        *newTrack = dbTrack;
    }

    /* Look for the artists in the db (use the "name" as key) */
    for (int i = 0; i < newTrack->artists.size(); ++i)
    {
        EMSArtist dbArtist;

        /* Look for an existing artist with the same name */
        if (getArtistByName(&dbArtist, newTrack->artists[i].name))
        {
            newTrack->artists[i] = dbArtist;
        }
        else /* Artist not found => create it */
        {
            q.prepare("INSERT INTO artists(name, picture) VALUES (?,?);");
            q.bindValue(0, newTrack->artists[i].name);
            q.bindValue(1, newTrack->artists[i].picture);
            if(!q.exec())
            {
                qCritical() << "Error while inserting new artist : " + q.lastError().text();
                q.exec("ROLLBACK;");
                return false;
            }
            /* Retrieve the new id in the database */
            if (getArtistByName(&dbArtist, newTrack->artists[i].name))
            {
                newTrack->artists[i] = dbArtist;
            }
            else
            {
                qCritical() << "Database error: cannot find artist name '" + newTrack->artists[i].name + "' which has been added.";
                q.exec("ROLLBACK;");
                return false;
            }
        }

        /* Link the artist to the new track */
        q.prepare("INSERT INTO tracks_artists(track_id, artist_id) VALUES (?,?);");
        q.bindValue(0, newTrack->id);
        q.bindValue(1, newTrack->artists[i].id);
        if(!q.exec())
        {
            qCritical() << "Error while inserting the relation track-artist : " + q.lastError().text();
            q.exec("ROLLBACK;");
            return false;
        }
    }

    /* Look for the genres in the db (use the "name" as key) */
    for (int i = 0; i < newTrack->genres.size(); ++i)
    {
        EMSGenre dbGenre;

        /* Look for an existing artist with the same name */
        if (getGenreByName(&dbGenre, newTrack->genres[i].name))
        {
            newTrack->genres[i] = dbGenre;
        }
        else /* Genre not found => create it */
        {
            q.prepare("INSERT INTO genres(name, picture) VALUES (?,?);");
            q.bindValue(0, newTrack->genres[i].name);
            q.bindValue(1, newTrack->genres[i].picture);
            if(!q.exec())
            {
                qCritical() << "Error while inserting new genre : " + q.lastError().text();
                q.exec("ROLLBACK;");
                return false;
            }
            /* Retrieve the new id in the database */
            if (getGenreByName(&dbGenre, newTrack->genres[i].name))
            {
                newTrack->genres[i] = dbGenre;
            }
            else
            {
                qCritical() << "Database error: cannot find genre name '" + newTrack->genres[i].name + "' which has been added.";
                q.exec("ROLLBACK;");
                return false;
            }
        }

        /* Link the genre to the new track */
        q.prepare("INSERT INTO tracks_genres(track_id, genre_id) VALUES (?,?);");
        q.bindValue(0, newTrack->id);
        q.bindValue(1, newTrack->genres[i].id);
        if(!q.exec())
        {
            qCritical() << "Error while inserting the relation track-genre : " + q.lastError().text();
            q.exec("ROLLBACK;");
            return false;
        }
    }

    /* All went well => COMMIT */
    q.exec("COMMIT;");
    return true;
}

/* Insert a new album.
 * You have to call to this function before inserting new track of this album.
 * The new album ID is written in album
 */
bool Database::insertNewAlbum(EMSAlbum *album)
{
    if (!opened)
    {
        return false;
    }

    /* Get all possible data in one row */
    QSqlQuery q(db);
    q.prepare("INSERT INTO album(name, cover) VALUES (?,?);");
    q.bindValue(0, album->name);
    q.bindValue(1, album->cover);
    if(!q.exec())
    {
        qCritical() << "Querying track data failed : " + q.lastError().text();
        return false;
    }

    /* Return the new album ID */
    q.prepare("SELECT last_insert_rowid();");
    if(!q.exec() || !q.next())
    {
        qCritical() << "Querying track data failed : " + q.lastError().text();
        return false;
    }
    album->id = q.value(0).toULongLong();
    return true;
}

/* Insert a filename (path) for a given already created track
 * The track ID must match an existing row in table tracks
 * If the filename already exist, the track_id is replaced.
 */
bool Database::insertNewFilename(QString filename, unsigned long long trackId)
{
    if (!opened)
    {
        return false;
    }

    /* Get all possible data in one row */
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO files(filename, track_id) VALUES (?,?);");
    q.bindValue(0, filename);
    q.bindValue(1, trackId);
    if(!q.exec())
    {
        qCritical() << "Querying track data failed : " + q.lastError().text();
        return false;
    }
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

const QString select_track_from_artist_data1 = \
"SELECT tracks.id, tracks.position, tracks.name, tracks.sha1, tracks.format, "
"       tracks.sample_rate, tracks.duration, tracks.format_parameters, "
"       albums.id, albums.name, albums.cover, "
"       files.filename "
"FROM   tracks, albums, artists, tracks_artists "
"WHERE  tracks.album_id = albums.id AND "
"       tracks.id = files.track_id AND "
"       tracks.id = tracks_artists.track_id AND "
"       artists.id = tracks_artists.artist_id ";

const QString select_track_from_genre_data1 = \
"SELECT tracks.id, tracks.position, tracks.name, tracks.sha1, tracks.format, "
"       tracks.sample_rate, tracks.duration, tracks.format_parameters, "
"       albums.id, albums.name, albums.cover, "
"       files.filename "
"FROM   tracks, albums, genres, tracks_genres "
"WHERE  tracks.album_id = albums.id AND "
"       tracks.id = files.track_id AND "
"       tracks.id = tracks_genres.track_id AND "
"       genres.id = tracks_genres.genre_id ";

const QString select_artist_from_track_data1 = \
"SELECT tracks.id, artists.id, artists.name, artists.picture "
"FROM   tracks, artists, tracks_artists "
"WHERE  tracks.id = tracks_artists.track_id AND "
"       artists.id = tracks_artists.artist_id ";

const QString select_artist_from_genre_data1 = \
"SELECT tracks.id, artists.id, artists.name, artists.picture "
"FROM   tracks, genres, tracks_genres, artists, tracks_artists "
"WHERE  tracks.id = tracks_genres.track_id AND "
"       genre.id = tracks_genres.genre_id AND "
"       tracks.id = tracks_artists.track_id AND "
"       artists.id = tracks_artists.artist_id ";

const QString select_genre_from_track_data1 = \
"SELECT tracks.id, genre.id, genre.name, genre.picture "
"FROM   tracks, genres, tracks_genres "
"WHERE  tracks.id = tracks_genres.track_id AND "
"       genre.id = tracks_genres.genre_id ";

const QString select_genre_from_artist_data1 = \
"SELECT tracks.id, genre.id, genre.name, genre.picture "
"FROM   tracks, genres, tracks_genres, artists, tracks_artists "
"WHERE  tracks.id = tracks_genres.track_id AND "
"       genre.id = tracks_genres.genre_id AND "
"       tracks.id = tracks_artists.track_id AND "
"       artists.id = tracks_artists.artist_id ";

const QString select_artist_data1 = \
"SELECT id, name, picture "
"FROM artists ";

const QString select_genre_data1 = \
"SELECT id, name, picture "
"FROM genres ";

const QString select_album_data1 = \
"SELECT id, name, cover "
"FROM albums ";


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
    if (!storeTrack(&q, track))
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

/* Like getTrackById but use SHA1 as identifier
 * Prefer to use getTrackById if possible for performance
 */
bool Database::getTrackBySha1(EMSTrack *track, QString sha1)
{
    if (!opened)
    {
        return false;
    }

    /* Get all possible data in one row */
    QSqlQuery q(db);
    q.prepare(select_track_data1 + " AND tracks.sha1 = '?' LIMIT 1;");
    q.bindValue(0, sha1);
    if (!storeTrack(&q, track))
    {
        qDebug() << "The track SHA1 " + sha1 + " does not exist in database";
        return false;
    }

    /* Get the artists list */
    getArtistsByTrackId(&(track->artists), track->id);

    /* Get the genres list */
    getGenresByTrackId(&(track->genres), track->id);

    return true;
}

/* Execute the query q which return ONE row
 * Store the result in the track structure
 * Warning: the query MUST match the order of field assignment in this function
 */
bool Database::storeTrack(QSqlQuery *q, EMSTrack *track)
{
    if(!q->exec())
    {
        qCritical() << "Querying track data failed : " + q->lastError().text();
        return false;
    }
    unsigned int colId = 1;
    if (q->size() >= 1 && !q->next())
    {
        /* Follow the same order as in the SQL query */
        track->id = q->value(colId++).toULongLong();
        track->position = q->value(colId++).toUInt();
        track->name = q->value(colId++).toUInt();
        track->sha1 = QByteArray().fromHex(q->value(colId++).toByteArray());
        track->format = q->value(colId++).toString();
        track->sample_rate = q->value(colId++).toULongLong();
        track->duration = q->value(colId++).toUInt();
        track->format_parameters = q->value(colId++).toString();;
        track->album.id = q->value(colId++).toULongLong();
        track->album.name = q->value(colId++).toString();
        track->album.cover = q->value(colId++).toString();
        track->filename = q->value(colId++).toString(); /* Discard the other row with the LIMIT clause */
    }
    else
    {
        return false;
    }
    return true;
}

/* Execute the query q
 * Store each result in the tracks list
 * Warning: the query MUST match the order of field assignment in this function
 * Warning: the rows must be ordered by the track ID
 * Warning: only one row per track should be returned by the query
 */
void Database::storeTrackList(QSqlQuery *q, QVector<EMSTrack> *tracksList)
{
    if(!q->exec())
    {
        qCritical() << "Querying tracks list failed : " + q->lastError().text();
        return;
    }
    tracksList->clear();
    while (q->next())
    {
        /* Follow the same order as in the SQL query */
        EMSTrack track;
        unsigned int colId = 0;
        track.id = q->value(colId++).toULongLong();
        track.position = q->value(colId++).toUInt();
        track.name = q->value(colId++).toUInt();
        track.sha1 = QByteArray().fromHex(q->value(colId++).toByteArray());
        track.format = q->value(colId++).toString();
        track.sample_rate = q->value(colId++).toULongLong();
        track.duration = q->value(colId++).toUInt();
        track.format_parameters = q->value(colId++).toString();;
        track.album.id = q->value(colId++).toULongLong();
        track.album.name = q->value(colId++).toString();
        track.album.cover = q->value(colId++).toString();
        track.filename = q->value(colId++).toString();
        tracksList->append(track);
    }
}

/* Execute the query q to insert artists list in the corresponding track structure.
 * Warning: the rows must be ordered by the track ID
 */
void Database::storeArtistsInTrackList(QSqlQuery *q, QVector<EMSTrack> *tracksList)
{
    if(!q->exec())
    {
        qCritical() << "Querying tracks list (artists) failed : " + q->lastError().text();
        return;
    }
    int currentListId = 0;
    unsigned long long currentTrackId;
    while (q->next())
    {
        // tracks.id, artists.id, artists.name, artists.picture
        currentTrackId = q->value(0).toULongLong();
        while((currentTrackId != tracksList->at(currentListId).id) && currentListId < (tracksList->size()-1))
        {
            currentListId++;
        }

        EMSArtist artist;
        artist.id = q->value(1).toULongLong();
        artist.name = q->value(2).toString();
        artist.picture = q->value(3).toString();
        (*tracksList)[currentListId].artists.append(artist);
    }
}

/* Execute the query q to insert genres list in the corresponding track structure.
 * Warning: the rows must be ordered by the track ID
 */
void Database::storeGenresInTrackList(QSqlQuery *q, QVector<EMSTrack> *tracksList)
{
    if(!q->exec())
    {
        qCritical() << "Querying tracks list (genres) failed : " + q->lastError().text();
        return;
    }
    int currentListId = 0;
    unsigned long long currentTrackId;
    while (q->next())
    {
        // tracks.id, genres.id, genres.name, genres.picture
        currentTrackId = q->value(0).toULongLong();
        while((currentTrackId != tracksList->at(currentListId).id) && currentListId < (tracksList->size()-1))
        {
            currentListId++;
        }

        EMSGenre genre;
        genre.id = q->value(1).toULongLong();
        genre.name = q->value(2).toString();
        genre.picture = q->value(3).toString();
        (*tracksList)[currentListId].genres.append(genre);
    }
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
     *                     two filenames for one tracks.id ? => TOCHECK
     */
    QSqlQuery q(db);
    q.prepare(select_track_data1 + " ORDER BY tracks.id GROUP BY tracks.id;");
    if(!q.exec())
    {
        qCritical() << "Querying tracks list failed : " + q.lastError().text();
        return;
    }
    storeTrackList(&q, tracksList);

    /* If no track, return now. */
    if (tracksList->size() <= 0)
    {
        return;
    }

    /* STEP 2 :
     * Get all artists data (ordered by track id)
     * Note that the list of track is ORDERED by tracks id.
     */
    q.prepare(select_artist_from_track_data1 + " ORDER BY tracks.id;");
    storeArtistsInTrackList(&q, tracksList);

    /* STEP 3 :
     * Get all genres data (ordered by track id)
     * Note that the list of track is ORDERED by tracks id.
     */
    q.prepare(select_genre_from_track_data1 + " ORDER BY tracks.id;");
    storeGenresInTrackList(&q, tracksList);

}

void Database::getTracksByAlbum(QVector<EMSTrack> *tracksList, unsigned long long albumId)
{
    if (!opened)
    {
        return;
    }

    /* Get all the tracks data */
    QSqlQuery q(db);
    q.prepare(select_track_data1 + " WHERE tracks.album_id = '?' ORDER BY tracks.id GROUP BY tracks.id;");
    q.bindValue(0, albumId);
    if(!q.exec())
    {
        qCritical() << "Querying tracks list failed : " + q.lastError().text();
        return;
    }
    storeTrackList(&q, tracksList);

    /* If no track, return now. */
    if (tracksList->size() <= 0)
    {
        return;
    }

    /* Get all artists data */
    q.prepare(select_artist_from_track_data1 + " WHERE tracks.album_id = '?' ORDER BY tracks.id;");
    q.bindValue(0, albumId);
    storeArtistsInTrackList(&q, tracksList);

    /* Get all genres data */
    q.prepare(select_genre_from_track_data1 + " WHERE tracks.album_id = '?' ORDER BY tracks.id;");
    q.bindValue(0, albumId);
    storeGenresInTrackList(&q, tracksList);
}

/*****************************************************************************
 *    BROWSING ARTISTS
 ****************************************************************************/
void Database::getArtistsList(QVector<EMSArtist> *artistsList)
{
    if (!opened)
    {
        return;
    }
    QSqlQuery q(db);
    q.prepare(select_artist_data1 + ";");
    if(!q.exec())
    {
        qCritical() << "Querying artists list failed : " + q.lastError().text();
        return;
    }
    artistsList->clear();
    while (q.next())
    {
        // artists.id, artists.name, artists.picture
        EMSArtist artist;
        artist.id = q.value(0).toULongLong();
        artist.name = q.value(1).toString();
        artist.picture = q.value(2).toString();
        artistsList->append(artist);
    }
}

bool Database::getArtistById(EMSArtist *artist, unsigned long long artistId)
{
    if (!opened)
    {
        return false;
    }
    QSqlQuery q(db);
    q.prepare(select_artist_data1 + " WHERE id='?';");
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
    q.prepare(select_artist_data1 + " WHERE name='?';");
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
void Database::getAlbumsList(QVector<EMSAlbum> *albumsList)
{
    if (!opened)
    {
        return;
    }
    QSqlQuery q(db);
    q.prepare(select_album_data1 + ";");
    if(!q.exec())
    {
        qCritical() << "Querying album data failed : " + q.lastError().text();
        return;
    }
    albumsList->clear();
    while (q.next())
    {
        // albums.id, albums.name, albums.cover
        EMSAlbum album;
        album.id = q.value(0).toULongLong();
        album.name = q.value(1).toString();
        album.cover = q.value(2).toString();
        albumsList->append(album);
    }
}

bool Database::getAlbumById(EMSAlbum *album, unsigned long long albumId)
{
    if (!opened)
    {
        return false;
    }
    QSqlQuery q(db);
    q.prepare(select_album_data1 + " AND albums.id = '?';");
    q.bindValue(0, albumId);
    if(!q.exec())
    {
        qCritical() << "Querying album data failed : " + q.lastError().text();
        return false;
    }
    if (q.size() >= 1 && !q.next())
    {
        if (q.size() > 1)
        {
            qCritical() << "Database consistency error: several albums with ID " + QString("%1").arg(albumId);
        }
        album->id = q.value(0).toULongLong();
        album->name = q.value(1).toString();
        album->cover = q.value(2).toString();
        return true;
    }
    return false;
}

/*****************************************************************************
 *    BROWSING GENRES
 ****************************************************************************/
void Database::getGenresList(QVector<EMSGenre> *genresList)
{
    if (!opened)
    {
        return;
    }
    QSqlQuery q(db);
    q.prepare(select_genre_data1 + ";");
    if(!q.exec())
    {
        qCritical() << "Querying genre data failed : " + q.lastError().text();
        return;
    }
    genresList->clear();
    while (q.next())
    {
        // genre.id, genre.name, genre.cover
        EMSGenre genre;
        genre.id = q.value(0).toULongLong();
        genre.name = q.value(1).toString();
        genre.picture = q.value(2).toString();
        genresList->append(genre);
    }
}

bool Database::getGenreById(EMSGenre *genre, unsigned long long genreId)
{
    if (!opened)
    {
        return false;
    }
    QSqlQuery q(db);
    q.prepare(select_genre_data1 + " WHERE id='?';");
    q.bindValue(0, genreId);
    q.exec();
    if (q.size() >= 1 && !q.next())
    {
        if (q.size() > 1)
        {
            qCritical() << "Database consistency error: several genre with ID " + QString("%1").arg(genreId);
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
    q.prepare(select_genre_data1 + " WHERE name='?';");
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
        genresList->append(genre);
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
        //TODO_weak : for the future: execute a upgrade script
        //            otherwise, delete the whole database and re-create it
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


