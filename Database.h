#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QVariant>
#include <QJsonObject>
#include <QMap>


#include <QSqlError>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QTime>

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = 0);
    ~Database();

    void beginTransaction();
    void endTransaction();

    void fileInsert(QString filename, QByteArray sha1);
    void songInsert(QString song, QJsonObject *metadata);
    void artistInsert(QString artist, QJsonObject *metadata);
    void albumInsert(QString album, QJsonObject *metadata);
    void artistAlbumInsert(QString artist, QString album, QJsonObject *metadata);
    QJsonObject artistsListGet();
    QJsonObject albumsListGet();
    QJsonObject songsListGet();
    QJsonObject artistDataGet(QString artist);
    QJsonObject albumDataGet(QString album);
    QJsonObject songDataGet(QString song);



private:
    int insertFileMagic;
    QSqlError initDb();
    QSqlDatabase db;
    QSqlQuery *insertTrackQuery;
    QSqlQuery *insertFileQuery;
    QVariantList m_filenames;
    QVariantList m_sha1s;
    QVariantList m_magics;

signals:

public slots:
};

#endif // DATABASE_H
