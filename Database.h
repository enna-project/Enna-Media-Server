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
    bool insertNewTrack(EMSTrack newTrack);
    bool searchExistingTrack(QString sha1, EMSTrack *existingTrack);

    /* Interface for browsing */
    void getTracks(QVector<EMSTrack> *tracksList);
    void getTracksByArtist(QVector<EMSTrack> *tracksList, unsigned long artistId);
    void getTracksByAlbum(QVector<EMSTrack> *tracksList, unsigned long albumId);
    void getTracksByGenre(QVector<EMSTrack> *tracksList, unsigned long genreId);
    void getAlbumList(QVector<EMSAlbum> *albumList);
    void getGenreList(QVector<EMSGenre> *genreList);

    /* Data accessor (search) */
    bool getTrackById(EMSTrack *track, unsigned long trackId);
    void getTracksByName(QVector<EMSTrack> *tracksList, QString name);
    bool getArtistById(EMSArtist *artist, unsigned long artistId);
    bool getArtistByName(EMSArtist *artist, QString name);
    bool getAlbumById(EMSAlbum *album, unsigned long albumId);
    void getAlbumsByName(QVector<EMSAlbum> *albumsList, QString name);
    bool getGenreById(EMSGenre *genre, unsigned long genreId);
    bool getGenreByName(EMSGenre *genre, QString name);

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
