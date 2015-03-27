#include <QDir>
#include "JsonApi.h"
#include "Player.h"

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
        QJsonObject answerData;


        switch (toMessageType(j.object()["msg"].toString()))
        {
        case EMS_BROWSE:
            qDebug() << "QUERY BROWSE:" << j.object()["url"].toString();
            answerData = processMessageBrowse(j.object(), ret);
            break;
        case EMS_DISK:
            ret = processMessageDisk(j.object());
            break;
        case EMS_PLAYER:
            qDebug() << "QUERY PLAYER:" << j.object()["action"].toString();
            ret = processMessagePlayer(j.object());
            break;
        case EMS_PLAYLIST:
            qDebug() << "QUERY PLAYLIST:"
                     << j.object()["url"].toString()
                     << j.object()["action"].toString()
                     << j.object()["filename"].toString();
            ret = processMessagePlaylist(j.object());
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
    else if (type == "EMS_PLAYLIST")
        return EMS_PLAYLIST;
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

/* Handle player query
 * The field action should be :
 * - next : Next song
 * - prev : Previous song
 * - play : Play the current song
 * - pause : Pause the current song
 * - stop : Stop the playing
 * - toggle : Toggle the state of the player (play/pause)
 * - shuffle_on : Set the shuffle state on
 * - shuffle_off : Set the shuffle state off
 * - repeat_on : Set the repeat state on
 * - repeat_off : Set the repeat state off
 */
bool JsonApi::processMessagePlayer(const QJsonObject &message)
{
    QString action = message["action"].toString();
    if (action == "next")
    {
        Player::instance()->next();
    }
    else if (action == "prev")
    {
        Player::instance()->prev();
    }
    else if (action == "play")
    {
        Player::instance()->play();
    }
    else if (action == "pause")
    {
        Player::instance()->pause();
    }
    else if (action == "stop")
    {
        Player::instance()->stop();
    }
    else if (action == "toggle")
    {
        Player::instance()->toggle();
    }
    else if (action == "shuffle_on")
    {
        Player::instance()->setRandom(true);
    }
    else if (action == "shuffle_off")
    {
        Player::instance()->setRandom(false);
    }
    else if (action == "repeat_on")
    {
        Player::instance()->setRepeat(true);
    }
    else if (action == "repeat_off")
    {
        Player::instance()->setRepeat(false);
    }
    else
    {
        qCritical() << "Error: unknow player action " << action;
        return false;
    }

    return true;
}

/* Handle playlist query */
bool JsonApi::processMessagePlaylist(const QJsonObject &message)
{
    QString type = message["url"].toString().remove("playlist://");
    QString action = message["action"].toString();

    /* Manage the player playlist */
    if (type == "current")
    {
        if (action == "clear")
        {
            Player::instance()->removeAllTracks();
        }
        else if (action == "add" || action == "del")
        {
            QVector<EMSTrack> trackList;
            getTracksFromFilename(&trackList, message["filename"].toString());
            if (trackList.size() > 0)
            {
                for (int i=0; i<trackList.size(); i++)
                {
                    if (action == "add")
                    {
                        Player::instance()->addTrack(trackList.at(i));
                    }
                    else if (action == "del")
                    {
                        Player::instance()->removeTrack(trackList.at(i));
                    }
                }
            }
            else
            {
                qCritical() << "Error: filename does not match any track.";
                return false;
            }

        }
        else
        {
            qCritical() << "Error: unknown action " << action;
            return false;
        }
    }
    else /* Manage the saved playlist */
    {
        ; //TODO!
    }
    return true;
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

/* This function retrieve the list of track from a given filename.
 * Filename could be :
 * - cdda:// : all track in the CDROM
 * - cdda://[id] : track [id] of the CDROM
 * - library://music/tracks/[id] : track [id] from the DB
 * - library://music/albums/[id] : add the album [id] from the DB
 * - file://[absolute path] : local path defining either a file or a directory
 */
void JsonApi::getTracksFromFilename(QVector<EMSTrack> *trackList, QString filename)
{
    if (filename.startsWith("cdda://"))
    {
        QString trackID = filename.remove("cdda://");
        if (trackID.isEmpty())
        {
            //TODO: ask CDROM module the list of track in the CDROM
            for (int i=1; i<10; i++)
            {
                EMSTrack track;
                track.type = TRACK_TYPE_CDROM;
                track.filename = "/dev/sr0";
                track.position = i;
                track.name = QString("track%1").arg(track.position);
                trackList->append(track);
            }
        }
        else
        {
            //TODO: ask CDROM module the track in the CDROM
            EMSTrack track;
            track.type = TRACK_TYPE_CDROM;
            track.filename = "/dev/sr0";
            track.position = trackID.toUInt();
            track.name = QString("track%1").arg(track.position);
            trackList->append(track);
        }
    }
    else if (filename.startsWith("library://music/albums/"))
    {
        QString albumID = filename.remove("library://music/albums/");
        if (!albumID.isEmpty())
        {
            unsigned long long id = albumID.toULongLong();
            Database::instance()->getTracksByAlbum(trackList, id);
        }
    }
    else if (filename.startsWith("library://music/tracks/"))
    {
        QString trackID = filename.remove("library://music/tracks/");
        if (!trackID.isEmpty())
        {
            unsigned long long id = trackID.toULongLong();
            EMSTrack track;
            bool success = Database::instance()->getTrackById(&track, id);
            if (success)
            {
                trackList->append(track);
            }

        }
    }
    else if (filename.startsWith("file://"))
    {
        QString path = filename.remove("file://");
        if (!path.isEmpty() && path.startsWith("/"))
        {
            //TODO: Use the filewatcher module instead
            QFileInfo fileInfo(path); /* Directory */
            if (fileInfo.isDir())
            {
                QDir directory(path);
                directory.setFilter(QDir::Files);
                QStringList files = directory.entryList();
                for (int i=0; i<files.size(); i++)
                {
                    EMSTrack track;
                    track.type = TRACK_TYPE_EXTERNAL;
                    track.filename = path + "/" + files.at(i);
                    track.name = files.at(i);
                    trackList->append(track);
                }
            }
            else if (fileInfo.exists()) /* Only one file */
            {
                EMSTrack track;
                track.type = TRACK_TYPE_EXTERNAL;
                track.filename = path;
                track.name = QFile(path).fileName();
                trackList->append(track);
            }
        }
    }
}

/* Asynchronous API */
void JsonApi::sendStatus(EMSPlayerStatus status)
{

    QJsonObject statusJsonObj;
    statusJsonObj["msg"] = "EMS_PLAYER";
    statusJsonObj["player_state"] = Player::instance()->stateToString(status.state);

    if (status.state == STATUS_PLAY || status.state == STATUS_PAUSE)
    {
        statusJsonObj["progress"] = (qint64)status.progress;
        statusJsonObj["position"] = (qint64)status.posInPlaylist;
        statusJsonObj["repeat"] = status.repeat;
        statusJsonObj["random"] = status.random;

        /* As we are kind, send track data in this message */
        EMSPlaylist currentPlaylist = Player::instance()->getCurentPlaylist();
        if (status.posInPlaylist >= 0 && status.posInPlaylist < currentPlaylist.tracks.size())
        {
            EMSTrack currentTrack = currentPlaylist.tracks.at(status.posInPlaylist);
            statusJsonObj["track"] = EMSTrackToJson(currentTrack);
        }
    }

    QJsonDocument doc(statusJsonObj);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void JsonApi::sendPlaylist(EMSPlaylist newPlaylist)
{
    /* TODO */
}
