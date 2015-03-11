#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QVariant>
#include <QJsonObject>
#include <QMap>
#include <QMutex>
#include <QSqlError>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QTime>

#include "Data.h"

class Database : public QObject
{
    Q_OBJECT

public:
    /* Internal interface for managing this object */
    bool open(); /* Open the database */
    void close(); /* Close the database. For performance, let it open. */

    /* Interface for track management (server-side actions) */
    bool insertNewAlbum(EMSAlbum *album);
    bool insertNewTrack(EMSTrack *newTrack);
    bool insertNewFilename(QString filename, unsigned long long trackId);

    /* Interface for browsing */
    void getTracks(QVector<EMSTrack> *tracksList);
    void getTracksByAlbum(QVector<EMSTrack> *tracksList, unsigned long long albumId);
    bool getTrackById(EMSTrack *track, unsigned long long trackId);
    bool getTrackBySha1(EMSTrack *track, QString sha1);
    void getAlbumsList(QVector<EMSAlbum> *albumsList);
    bool getAlbumById(EMSAlbum *album, unsigned long long albumId);
    void getArtistsList(QVector<EMSArtist> *artistsList);
    bool getArtistById(EMSArtist *artist, unsigned long long artistId);
    bool getArtistByName(EMSArtist *artist, QString name);
    void getArtistsByTrackId(QVector<EMSArtist> *artistsList, unsigned long long trackId);
    void getGenresList(QVector<EMSGenre> *genresList);
    bool getGenreById(EMSGenre *genre, unsigned long long genreId);
    bool getGenreByName(EMSGenre *genre, QString name);
    void getGenresByTrackId(QVector<EMSGenre> *genresList, unsigned long long trackId);

    /* Signleton pattern
     * See: http://www.qtcentre.org/wiki/index.php?title=Singleton_pattern
     */
    static Database* instance()
    {
        static QMutex mutex;
        if (!_instance)
        {
            mutex.lock();

            if (!_instance)
                _instance = new Database;

            mutex.unlock();
        }
        return _instance;
    }

    static void drop()
    {
        static QMutex mutex;
        mutex.lock();
        delete _instance;
        _instance = 0;
        mutex.unlock();
    }

private:
    /* Current state of the opened database */
    bool opened;
    unsigned int version;
    QSqlDatabase db;

    /* Data parsed from the QSetting file */
    QString dbSettingPath;
    QString dbSettingCreateScript;
    unsigned int dbSettingVersion;

    /* Internal method */
    bool createSchema(QString filePath);
    bool storeTrack(QSqlQuery *q, EMSTrack *track);
    void storeTrackList(QSqlQuery *q, QVector<EMSTrack> *tracksList);
    void storeArtistsInTrackList(QSqlQuery *q, QVector<EMSTrack> *tracksList);
    void storeGenresInTrackList(QSqlQuery *q, QVector<EMSTrack> *tracksList);

    /* Hide all other access to this class */
    static Database* _instance;
    Database(QObject *parent = 0);
    Database(const Database &);
    Database& operator=(const Database &);
    ~Database();

signals:

public slots:
};

#endif // DATABASE_H
