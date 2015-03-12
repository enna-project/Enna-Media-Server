#include "JsonApi.h"

const QString JSON_OBJECT_MENU = "{\"menus\":[{\"name\":\"Library\",\"url_scheme\":\"library://\",\"icon\":\"http://ip/imgs/library.png\",\"enabled\":true},{\"name\":\"Audio CD\",\"url_scheme\":\"cdda://\",\"icon\":\"http://ip/imgs/cdda.png\",\"enabled\":false},{\"name\":\"Playlists\",\"url_scheme\":\"playlist://\",\"icon\":\"http://ip/imgs/playlists.png\",\"enabled\":true},{\"name\":\"Settings\",\"url_scheme\":\"settings://\",\"icon\":\"http://ip/imgs/settings.png\",\"enabled\":true}]}";

JsonApi::JsonApi(QWebSocket *webSocket) :
    m_webSocket(webSocket)
{

}

JsonApi::~JsonApi()
{

}

bool JsonApi::processMessage(const QString &message)
{
    bool ret = false;

    QJsonParseError e;
    QJsonDocument j = QJsonDocument::fromJson(message.toUtf8(), &e);

    if (e.error !=  QJsonParseError::NoError )
    {
        qDebug() << "Error parsing Json message : " << e.errorString();
        ret = false;
    }
    else
    {
        qDebug() << "msg : " << j.object()["msg"].toString();
        qDebug() << "msg_id : " << j.object()["msg_id"].toString();
        qDebug() << "uuid  : " << j.object()["uuid"].toString();

        QJsonObject answerData;


        switch (toMessageType(j.object()["msg"].toString()))
        {
        case EMS_BROWSE:
            answerData = processMessageBrowse(j.object(), ret);
            break;
        case EMS_DISK:
            ret = processMessageDisk(j.object());
            break;
        case EMS_PLAYER:
            ret = processMessagePlayer(j.object());
            break;
        default:
            ret = false;
            break;
        }

        if (!ret)
            return false;

        QJsonObject answer;
        answer["msg"] = j.object()["msg"];
        answer["msg_id"] = j.object()["msg_id"];
        answer["uuid"] = j.object()["uuid"];
        answer["data"] = answerData;

        QJsonDocument doc(answer);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }

    return ret;
}

JsonApi::MessageType JsonApi::toMessageType(const QString &type) const
{
    if (type == "EMS_BROWSE")
        return EMS_BROWSE;
    else if (type == "EMS_DISK")
        return EMS_DISK;
    else if (type == "EMS_PLAYER")
        return EMS_PLAYER;
    else
        return EMS_UNKNOWN;
}

QJsonObject JsonApi::processMessageBrowse(const QJsonObject &message, bool &ok)
{
    QString url = message["url"].toString();
    QJsonObject obj;
    ok = false;
    if (url.isEmpty())
        return obj;

    switch(urlSchemeGet(url))
    {
    case SCHEME_MENU:
        url.remove("menu://");
        if (!url.isEmpty())
            return obj;
        obj = QJsonDocument::fromJson(JSON_OBJECT_MENU.toUtf8()).object();
        break;
    case SCHEME_LIBRARY:
        url.remove("library://");
        obj = processMessageBrowseLibrary(message, ok);
        break;
    default:
        return obj;
    }
    ok = true;
    return obj;
}

QJsonObject JsonApi::processMessageBrowseLibrary(const QJsonObject &message, bool &ok)
{
    QJsonObject obj;
    if (message["url"].toString().isEmpty())
    {
        ok = false;
        return obj;
    }

    if (message["url"].toString() == "music")
    {
        //return artist/albums/tracks ...
    }

    QStringList list = message["url"].toString().split("/");

    if (list[0] == "artists")
    {
        processMessageBrowseLibraryArtists(list, ok);
    }
    else if (list[0] == "albums")
    {
        processMessageBrowseLibraryAlbums(list, ok);
    }
    else if (list[0] == "tracks")
    {
        processMessageBrowseLibraryTracks(list, ok);
    }

    else if (list[0] == "genre")
    {
        processMessageBrowseLibraryGenre(list, ok);
    }

    return obj;
}

QJsonObject JsonApi::processMessageBrowseLibraryArtists(QStringList &list, bool &ok)
{
    QJsonObject obj;
    int artistId;
    int albumId;
    switch (list.size())
    {
    case 0:
        //get list of all artists
        break;
    case 1:
        // get list of albums of artistId
         artistId = list[0].toInt();
        break;
    case 2:
        // get list of tracks of albumId of ArtistId
        artistId = list[0].toInt();
        albumId = list[1].toInt();
        break;
    default:
        ok = false;
        break;
    }
    return obj;
}

QJsonObject JsonApi::processMessageBrowseLibraryAlbums(QStringList &list, bool &ok)
{

}

QJsonObject JsonApi::processMessageBrowseLibraryTracks(QStringList &list, bool &ok)
{

}

QJsonObject JsonApi::processMessageBrowseLibraryGenre(QStringList &list, bool &ok)
{

}

bool JsonApi::processMessageDisk(const QJsonObject &message)
{

}

bool JsonApi::processMessagePlayer(const QJsonObject &message)
{

}

JsonApi::UrlSchemeType JsonApi::urlSchemeGet(const QString &url) const
{
    QString scheme = url.split("://").at(0);

    if (scheme.isEmpty())
        return SCHEME_UNKNOWN;

    if (scheme == "menu")
        return SCHEME_MENU;
    else if (scheme == "library")
        return SCHEME_LIBRARY;
    else if (scheme == "cdda")
        return SCHEME_CDDA;
    else if (scheme == "playlist")
        return SCHEME_PLAYLIST;
    else if (scheme == "settings")
        return SCHEME_SETTINGS;
    else
        return SCHEME_UNKNOWN;
}
