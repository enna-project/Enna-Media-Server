#include "Database.h"

#include <QDebug>

Database::Database(QObject *parent) : QObject(parent)
{
    // Database init with SQLite driver
    if (!QSqlDatabase::drivers().contains("QSQLITE"))
        qCritical() << "Unable to load database, SQlite driver is needed";

    // initialize the database
    QSqlError err = initDb();
    if (err.type() != QSqlError::NoError) {
         qCritical() << "Unable to initialize database : " << err;
        return;
    }

}

QSqlError Database::initDb()
{
    /* Crate db */
    db = QSqlDatabase::addDatabase("QSQLITE");

    /* Database should come from config file or from arguments ?*/
    db.setDatabaseName("database.sqlite");
    if (!db.open())
        return db.lastError();


    QStringList tables = db.tables();
    /*if (tables.contains("files", Qt::CaseInsensitive)
            && tables.contains("artists", Qt::CaseInsensitive))
        return QSqlError();*/

    QSqlQuery q(db);

    if (!q.exec(QLatin1String("CREATE TABLE IF NOT EXISTS "
                              "files(id integer primary key, filename varchar, sha1 varchar, magic integer)")))
        return q.lastError();

    if (!q.exec(QLatin1String( "CREATE INDEX IF NOT EXISTS "
                               "files_idx ON files (id)")))
        return q.lastError();

    if (!q.exec(QLatin1String( "CREATE TABLE IF NOT EXISTS tracks ("
                               "id INTEGER PRIMARY KEY, "
                               "title TEXT, "
                               "album_id INTEGER, "
                               "artist_id INTEGER, "
                               "genre_id INTEGER, "
                               "track_no INTEGER, "
                               "rating INTEGER, "
                               "play_count INTEGER "
                               ")")))
        return q.lastError();


    if (!q.exec(QLatin1String( "CREATE TABLE IF NOT EXISTS artists ("
                               "id INTEGER PRIMARY KEY, "
                               "name TEXT UNIQUE"
                               ")")))
        return q.lastError();

    if (!q.exec(QLatin1String( "CREATE INDEX IF NOT EXISTS "
                               "artists_name_idx ON artists (name)")))
        return q.lastError();

    if (!q.exec(QLatin1String( "CREATE TABLE IF NOT EXISTS albums ("
                               "id INTEGER PRIMARY KEY, "
                               "artist_id INTEGER, "
                               "name TEXT"
                               ")")))
        return q.lastError();

    if (!q.exec(QLatin1String( "CREATE INDEX IF NOT EXISTS "
                               "albums_artist_idx ON albums (artist_id)")))
        return q.lastError();

    if (!q.exec(QLatin1String( "CREATE INDEX IF NOT EXISTS "
                               "albums_name_idx ON albums (name)")))
        return q.lastError();
#if 0
    if (!q.exec(QLatin1String( "audios_title_idx",
                               "CREATE INDEX IF NOT EXISTS "
                               "audios_title_idx ON audios (title)")))
        return q.lastError();

    if (!q.exec(QLatin1String( "audios_album_idx",
                               "CREATE INDEX IF NOT EXISTS "
                               "audios_album_idx ON audios (album_id)")))
        return q.lastError();

    if (!q.exec(QLatin1String( "audios_artist_idx",
                               "CREATE INDEX IF NOT EXISTS "
                               "audios_artist_idx ON audios (artist_id)")))
        return q.lastError();

    if (!q.exec(QLatin1String( "audios_genre_idx",
                               "CREATE INDEX IF NOT EXISTS "
                               "audios_genre_idx ON audios (genre_id)")))
        return q.lastError();

    if (!q.exec(QLatin1String( "audios_trackno_idx",
                               "CREATE INDEX IF NOT EXISTS "
                               "audios_trackno_idx ON audios (trackno)")))
        return q.lastError();

    if (!q.exec(QLatin1String( "audios_playcnt_idx",
                               "CREATE INDEX IF NOT EXISTS "
                               "audios_playcnt_idx ON audios (playcnt)")))
        return q.lastError();

    if (!q.exec(QLatin1String( "audios_playcnt_idx",
                               "CREATE INDEX IF NOT EXISTS "
                               "audios_playcnt_idx ON audios (playcnt)")))
        return q.lastError();

    if (!q.exec(QLatin1String("create table albums(id integer primary key, albumName varchar)")))
        return q.lastError();

#endif
    insertTrackQuery = new QSqlQuery(db);
    insertTrackQuery->prepare("INSERT OR REPLACE INTO tracks "
                             "(title, album_id, artist_id, genre_id, "
                             "track_no, rating, play_count) "
                             "VALUES (?, ?, ?, ?, ?, ?, ?)");

    insertFileQuery = new QSqlQuery(db);
    insertFileQuery->prepare("INSERT OR REPLACE INTO files "
                             "(filename, sha1, magic) "
                             "VALUES (?, ?, ?)");

    return QSqlError();
}

void Database::beginTransaction()
{
    qsrand(QTime::currentTime().msec());
    insertFileMagic = qrand();
}

void Database::fileInsert(QString filename, QByteArray sha1)
{
    qDebug() << "Add file [" << sha1.toHex() << "] " << filename << " to DB";
    m_filenames << filename;
    m_sha1s << sha1.toHex();
    m_magics << insertFileMagic;
}

void Database::endTransaction()
{
    insertFileQuery->addBindValue(m_filenames);
    insertFileQuery->addBindValue(m_sha1s);
    insertFileQuery->addBindValue(m_magics);
    qDebug() << "Begin Transaction";
    db.exec("BEGIN TRANSACTION;");
    if (!insertFileQuery->execBatch())
        qDebug() << insertFileQuery->lastError();
    db.exec("END TRANSACTION;");
    qDebug() << "End Transaction";
    m_filenames.clear();
    m_sha1s.clear();
    m_magics.clear();


}

Database::~Database()
{
    db.close();
}


