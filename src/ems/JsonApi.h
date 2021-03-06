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
#include "Networkctl.h"



class JsonApi : public QObject
{
    Q_OBJECT
public:
    explicit JsonApi(QWebSocket *websScket, bool isLocal);
    ~JsonApi();

    enum MessageType {EMS_BROWSE, EMS_PLAYER, EMS_PLAYLIST, EMS_DISK,
                      EMS_AUTH, EMS_CD_RIP, EMS_NETWORK, EMS_UNKNOWN};
    enum UrlSchemeType {SCHEME_MENU, SCHEME_LIBRARY, SCHEME_CDDA,
                        SCHEME_PLAYLIST, SCHEME_SETTINGS, SCHEME_FILE, SCHEME_UNKNOWN};

    /* Asynchronous messages */
    void sendStatus(EMSPlayerStatus status);
    void sendPlaylist(EMSPlaylist newPlaylist);
    void sendAuthRequest(EMSClient client);
    void sendRipProgress(EMSRipProgress ripProgress);
    void sendMenu();
    void sendWifiConnected();
    void sendEthConnected();
    void sendWifiList();

signals:
    void startCdromRip();

private:
    QWebSocket *m_webSocket;
    QString httpPort;
    QString httpSrvUrl;
    QString cacheDirectory;
    QStringList m_supportedFormat;
    QString m_directoriesBasePath;
    bool m_isLocal;

    /* Queries handlers */
    JsonApi::MessageType toMessageType(const QString &type) const;

    bool processMessagePlayer(const QJsonObject &message);
    bool processMessagePlaylist(const QJsonObject &message);
    bool processMessageDisk(const QJsonObject &type);
    bool processMessageAuthentication(const QJsonObject &message);
    bool processMessageCDRip(const QJsonObject &message);
    bool processMessageNetwork(const QJsonObject &message);
    QJsonObject processMessageBrowse(const QJsonObject &type, bool &ok);
    QJsonObject processMessageBrowseMenu(const QJsonObject &message, bool &ok);
    QJsonObject processMessageBrowseLibrary(const QJsonObject &message, bool &ok);
    QJsonObject processMessageBrowseLibraryArtists(QStringList &list, bool &ok);
    QJsonObject processMessageBrowseLibraryAlbums(QStringList &list, bool &ok);
    QJsonObject processMessageBrowseLibraryTracks(QStringList &list, bool &ok);
    QJsonObject processMessageBrowseLibraryGenre(QStringList &list, bool &ok);
    QJsonObject processMessageBrowsePlaylist(const QJsonObject &message, bool &ok);
    QJsonObject processMessageBrowseCdrom(const QJsonObject &message, bool &ok);
    QJsonObject processMessageBrowseDirectory(const QJsonObject &message, bool &ok);
    JsonApi::UrlSchemeType urlSchemeGet(const QString &url) const;

    void updateNewUrl(void);

    QJsonObject EMSPlaylistToJson(EMSPlaylist playlist);
    QJsonObject EMSArtistToJson(const EMSArtist &artist);
    QJsonObject EMSTrackToJson(const EMSTrack &track);
    QJsonObject EMSGenreToJson(const EMSGenre &genre);
    QJsonObject EMSAlbumToJson(const EMSAlbum &album);
    QJsonObject EMSAlbumToJsonWithArtists(const EMSAlbum &album);
    QJsonObject EMSPlaylistToJsonWithoutTrack(EMSPlaylist playlist);
    QJsonObject EMSPlaylistsListToJson(EMSPlaylistsList playlistsList);
    QString EMSTrackTypeToString(EMSTrackType type) const;
    QJsonObject EMSSsidToJson(const EMSSsid &ssid) const;
    QJsonObject EMSEthernetToJson(const EMSEthernet &ethDat) const;
    void getTracksFromFilename(QVector<EMSTrack> *trackList, QString filename);
    QString convertImageUrl(QString url);
    // To be implemented when if menu become dynamic
    //QJsonObject buildJsonMenu();

public slots:
    bool processMessage(const QString &message);
    void ipChanged(QString newIp);
};

#endif // JSONAPI_H
