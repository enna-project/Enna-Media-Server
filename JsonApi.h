#ifndef JSONAPI_H
#define JSONAPI_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QString>

#include "Database.h"
#include "Data.h"

class JsonApi : public QObject
{
    Q_OBJECT
public:
    explicit JsonApi(QWebSocket *websScket);
    ~JsonApi();

    enum MessageType {EMS_BROWSE, EMS_PLAYER, EMS_DISK, EMS_UNKNOWN};
    enum UrlSchemeType {SCHEME_MENU, SCHEME_LIBRARY, SCHEME_CDDA,
                        SCHEME_PLAYLIST, SCHEME_SETTINGS, SCHEME_UNKNOWN};

signals:

private:
    QWebSocket *m_webSocket;

    JsonApi::MessageType toMessageType(const QString &type) const;

    bool processMessagePlayer(const QJsonObject &message);
    bool processMessageDisk(const QJsonObject &type);
    QJsonObject processMessageBrowse(const QJsonObject &type, bool &ok);
    QJsonObject processMessageBrowseLibrary(const QJsonObject &message, bool &ok);
    QJsonObject processMessageBrowseLibraryArtists(QStringList &list, bool &ok);
    QJsonObject processMessageBrowseLibraryAlbums(QStringList &list, bool &ok);
    QJsonObject processMessageBrowseLibraryTracks(QStringList &list, bool &ok);
    QJsonObject processMessageBrowseLibraryGenre(QStringList &list, bool &ok);
    JsonApi::UrlSchemeType urlSchemeGet(const QString &url) const;

    QJsonObject EMSArtistToJson(const EMSArtist &artist) const;
    QJsonObject EMSTrackToJson(const EMSTrack &track) const;
    QJsonObject EMSGenreToJson(const EMSGenre &genre) const;
    QJsonObject EMSAlbumToJson(const EMSAlbum &album) const;
    void getTracksFromFilename(QVector<EMSTrack> *trackList, QString filename);
    // To be implemented when if menu become dynamic
    //QJsonObject buildJsonMenu();

public slots:
    bool processMessage(const QString &message);
};

#endif // JSONAPI_H
