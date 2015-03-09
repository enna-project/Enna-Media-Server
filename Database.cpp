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
    if (getArtistByName(&dbArtist, newTrack.artist.name))
    {
        newTrack.artist = dbArtist;
    }
    else /* Artist not found => create it */
    {
        q.prepare("INSERT INTO artists(name, cover) VALUES (?,?)");
        q.bindValue(0, newTrack.artist.name);
        q.bindValue(1, newTrack.artist.picture);
        if(!q.exec())
        {
            qCritical() << "Error while inserting new artist : " + q.lastError().text();
            q.exec("ROLLBACK");
            return false;
        }
        /* Retrieve the new id in the database */
        if (getArtistByName(&dbArtist, newTrack.artist.name))
        {
            newTrack.artist = dbArtist;
        }
        else
        {
            qCritical() << "Database error: cannot find artist name '" + name + "' which has been added.";
            q.exec("ROLLBACK");
            return false;
        }
    }

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

void Database::getTracks(QVector<EMSTrack> *tracksList) {}
void Database::getTracksByArtist(QVector<EMSTrack> *tracksList, unsigned long artistId) {}
void Database::getTracksByAlbum(QVector<EMSTrack> *tracksList, unsigned long albumId) {}
void Database::getTracksByGenre(QVector<EMSTrack> *tracksList, unsigned long genreId) {}
void Database::getAlbumList(QVector<EMSAlbum> *albumList) {}
void Database::getGenreList(QVector<EMSGenre> *genreList) {}

/* Data accessor */
bool Database::getTrackById(EMSTrack *track, unsigned long trackId)
{
    if (!opened)
    {
        return false;
    }
    QSqlQuery q(db);
    q.prepare("SELECT tracks.id, tracks.position, tracks.name, tracks.sha1, tracks.format, "
              "       tracks.sample_rate, tracks.duration, tracks.format_parameters, "
              "       artists.id, artists.name, artists.picture, "
              "       albums.id, albums.name, albums.cover, "
              "       genres.id, genres.name, genres.picture, "
              "FROM tracks, artists, albums, genres "
              "WHERE tracks.id='?'"); //TODO
    q.bindValue(0, trackId);
    q.exec();
    if (q.size() >= 1 && !q.next())
    {
        //TODO
        return true;
    }
    return false;
}

void Database::getTracksByName(QVector<EMSTrack> *tracksList, QString name) {}

bool Database::getArtistById(EMSArtist *artist, unsigned long artistId)
{
    if (!opened)
    {
        return false;
    }
    QSqlQuery q(db);
    q.prepare("SELECT id, name, picture FROM artists WHERE id='?'");
    q.bindValue(0, artistId);
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

bool Database::getArtistByName(EMSArtist *artist, QString name)
{
    if (!opened)
    {
        return false;
    }
    QSqlQuery q(db);
    q.prepare("SELECT id, name, picture FROM artists WHERE name='?'");
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
bool Database::getAlbumById(EMSAlbum *album, unsigned long albumId) {}
void Database::getAlbumsByName(QVector<EMSAlbum> *albumsList, QString name) {}
bool Database::getGenreById(EMSGenre *genre, unsigned long genreId) {}
bool Database::getGenreByName(EMSGenre *genre, QString name) {}

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


