#include "JsonApi.h"

const QString JSON_OBJECT_MENU = "{\"menus\":[{\"name\":\"Library\",\"url_scheme\":\"library://\",\"icon\":\"http://ip/imgs/library.png\",\"enabled\":true},{\"name\":\"Audio CD\",\"url_scheme\":\"cdda://\",\"icon\":\"http://ip/imgs/cdda.png\",\"enabled\":false},{\"name\":\"Playlists\",\"url_scheme\":\"playlist://\",\"icon\":\"http://ip/imgs/playlists.png\",\"enabled\":true},{\"name\":\"Settings\",\"url_scheme\":\"settings://\",\"icon\":\"http://ip/imgs/settings.png\",\"enabled\":true}]}";
const QString JSON_OBJECT_LIBRARY_MUSIC = "{\"msg\": \"EMS_BROWSE\",\"msg_id\": \"id\",\"uuid\": \"110e8400-e29b-11d4-a716-446655440000\",\"data\" : {\"menus\": [{\"name\": \"Artists\",\"url\": \"library://music/artists\"},{\"name\": \"Albums\",\"url\": \"library://music/albums\"},{\"name\": \"Tracks\",\"url\": \"library://music/tracks\"},{\"name\": \"Genre\",\"url\": \"library://music/genres\"},{\"name\": \"Compositors\",\"url\": \"library://music/compositor\"}]}}";
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
        qDebug() << "REQ: " << j.object()["msg"].toString() << ":" << j.object()["url"].toString();

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

    QString url = message["url"].toString();

    if (url.isEmpty())
    {
        ok = false;
        return obj;
    }
    url.remove("library://");
    if (url == "music")
    {
        ok = true;
        obj = QJsonDocument::fromJson(JSON_OBJECT_LIBRARY_MUSIC.toUtf8()).object();
        return obj;
    }

    url.remove("music/");
    QStringList list =url.split("/");

    if (list[0] == "artists")
    {
        obj = processMessageBrowseLibraryArtists(list, ok);
    }
    else if (list[0] == "albums")
    {
        obj = processMessageBrowseLibraryAlbums(list, ok);
    }
    else if (list[0] == "tracks")
    {
        obj = processMessageBrowseLibraryTracks(list, ok);
    }

    else if (list[0] == "genres")
    {
        obj = processMessageBrowseLibraryGenre(list, ok);
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
    case 1:
    {
        //get list of all artists
        QVector<EMSArtist> artistsList;
        Database::instance()->getArtistsList(&artistsList);
        const int listSize = artistsList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
            jsonArray << EMSArtistToJson(artistsList[i]);
        obj["artists"] = jsonArray;
        ok = true;
        break;
    }
    case 2:
    {
        // get list of albums of artistId
         artistId = list[1].toInt();
         QVector<EMSAlbum> albumsList;
         Database::instance()->getAlbumsByArtistId(&albumsList, artistId);
         const int listSize = albumsList.size();
         QJsonArray jsonArray;
         for (int i = 0; i < listSize; ++i)
             jsonArray << EMSAlbumToJson(albumsList[i]);
         obj["albums"] = jsonArray;
         break;
    }
    case 3:
    {
        // get list of tracks of albumId of ArtistId
        albumId = list[2].toInt();
        QVector<EMSTrack> tracksList;
        Database::instance()->getTracksByAlbum(&tracksList, albumId);
        const int listSize = tracksList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
            jsonArray << EMSTrackToJson(tracksList[i]);
        obj["tracks"] = jsonArray;
        break;
    }
    default:
        ok = false;
        break;
    }
    return obj;
}

QJsonObject JsonApi::processMessageBrowseLibraryAlbums(QStringList &list, bool &ok)
{
    QJsonObject obj;
    int albumId;
    switch (list.size())
    {
    case 1:
    {
        //get list of all albums
        QVector<EMSAlbum> albumsList;
        Database::instance()->getAlbumsList(&albumsList);
        const int listSize = albumsList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
            jsonArray << EMSAlbumToJson(albumsList[i]);
        obj["albums"] = jsonArray;
        ok = true;
        break;
    }
    case 2:
    {
        // get list of tracks of albumId
        albumId = list[1].toInt();
        QVector<EMSTrack> tracksList;
        Database::instance()->getTracksByAlbum(&tracksList, albumId);
        const int listSize = tracksList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
            jsonArray << EMSTrackToJson(tracksList[i]);
        obj["tracks"] = jsonArray;
        break;
    }
    default:
        ok = false;
        break;
    }
    return obj;
}

QJsonObject JsonApi::processMessageBrowseLibraryTracks(QStringList &list, bool &ok)
{
    QJsonObject obj;
    switch (list.size())
    {
    case 1:
    {
        QVector<EMSTrack> tracksList;
        Database::instance()->getTracks(&tracksList);
        const int listSize = tracksList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
            jsonArray << EMSTrackToJson(tracksList[i]);
        obj["tracks"] = jsonArray;
        break;
    }

    default:
        ok = false;
        break;
    }
    return obj;
}

QJsonObject JsonApi::processMessageBrowseLibraryGenre(QStringList &list, bool &ok)
{
    QJsonObject obj;
    int genreId, albumId;
    switch (list.size())
    {
    case 1:
    {
        QVector<EMSGenre> genresList;
        Database::instance()->getGenresList(&genresList);
        const int listSize = genresList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
            jsonArray << EMSGenreToJson(genresList[i]);
        obj["tracks"] = jsonArray;
        break;
    }
    case 2:
    {
        // get list of albums by GenreID
        genreId = list[1].toInt();
        QVector<EMSAlbum> albumsList;
        Database::instance()->getAlbumsByGenreId(&albumsList, genreId);
        const int listSize = albumsList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
          jsonArray << EMSAlbumToJson(albumsList[i]);
        obj["albums"] = jsonArray;
        break;
    }
    case 3:
    {
        // get list of tracks of albumId of GenreID
        albumId = list[2].toInt();
        QVector<EMSTrack> tracksList;
        Database::instance()->getTracksByAlbum(&tracksList, albumId);
        const int listSize = tracksList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
            jsonArray << EMSTrackToJson(tracksList[i]);
        obj["tracks"] = jsonArray;
        break;
    }
    default:
        ok = false;
        break;
    }
    return obj;
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

QJsonObject JsonApi::EMSArtistToJson(const EMSArtist &artist) const
{
    QJsonObject obj;

    obj["id"] = (qint64)artist.id;
    obj["name"] = artist.name;
    obj["picture"] = artist.picture;

    return obj;
}

QJsonObject JsonApi::EMSAlbumToJson(const EMSAlbum &album) const
{
    QJsonObject obj;

    obj["id"] = (qint64)album.id;
    obj["name"] = album.name;
    obj["picture"] = album.cover;
    return obj;
}

QJsonObject JsonApi::EMSGenreToJson(const EMSGenre &genre) const
{
    QJsonObject obj;

    obj["id"] = (qint64)genre.id;
    obj["name"] = genre.name;
    obj["picture"] = genre.picture;
    return obj;
}

QJsonObject JsonApi::EMSTrackToJson(const EMSTrack &track) const
{
    QJsonObject obj;

    obj["id"] = (qint64)track.id;
    obj["position"] = (int)track.position;
    obj["name"] = track.name;
    obj["filename"] = track.filename;
    obj["sha1"] = track.sha1;
    obj["format"] = track.format;
    obj["sample_rate"] = (int)track.sample_rate;
    obj["duration"] = (int)track.duration;
    obj["format_parameters"] = track.format_parameters;
    return obj;
}
