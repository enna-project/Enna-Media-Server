#include <QDir>
#include <QSettings>
#include <QNetworkInterface>
#include <memory>
#include "CdromManager.h"
#include "DefaultSettings.h"
#include "JsonApi.h"
#include "Player.h"
#include "CdromManager.h"
#include "NetworkCtl.h"


#define TIMEOUT 10
#define STATE_START "start"

const QString JSON_OBJECT_LIBRARY_MUSIC = "{\"menus\": [{\"name\": \"Artists\",\"url\": \"library://music/artists\"},{\"name\": \"Albums\",\"url\": \"library://music/albums\"},{\"name\": \"Tracks\",\"url\": \"library://music/tracks\"},{\"name\": \"Genre\",\"url\": \"library://music/genres\"},{\"name\": \"Compositors\",\"url\": \"library://music/compositor\"}]}}";
JsonApi::JsonApi(QWebSocket *webSocket, bool isLocal) :
    m_webSocket(webSocket),
    m_isLocal(isLocal)
{
    QSettings settings;

    EMS_LOAD_SETTINGS(cacheDirectory, "main/cache_directory",
                      QStandardPaths::standardLocations(QStandardPaths::CacheLocation)[0], String);

    int port = 0;
    EMS_LOAD_SETTINGS(port, "main/http_port", EMS_HTTP_PORT, Int);
    if (port != 0)
    {
        httpPort = QString("%1").arg(port);
    }

    QString str;
    EMS_LOAD_SETTINGS(str, "main/music_extensions",
                      EMS_MUSIC_EXTENSIONS, String);
    // Remove space in strings
    m_supportedFormat = str.split(",");
    for (int i = 0;i < m_supportedFormat.size();i++)
        m_supportedFormat.replace(i, m_supportedFormat.at(i).trimmed());

    EMS_LOAD_SETTINGS(m_directoriesBasePath, "directories/path",
                      EMS_DIRECTORIES_BASE_PATH, String);


    updateNewUrl();

}
JsonApi::~JsonApi()
{

}

void JsonApi::updateNewUrl()
{
    /* TODO: use the NetworkManager instead */
    QString ipAddr;
    QList<QHostAddress> ipAddrs = QNetworkInterface::allAddresses();

    if (m_isLocal)
    {
        ipAddr = QHostAddress(QHostAddress::LocalHost).toString();
    }
    else
    {
        for(int addrId=0; addrId<ipAddrs.count(); addrId++)
        {
            if(!ipAddrs[addrId].isLoopback())
            {
                if (ipAddrs[addrId].protocol() == QAbstractSocket::IPv4Protocol )
                {
                    ipAddr = ipAddrs[addrId].toString();
                    break;
                }
            }
        }
    }
    if (!ipAddr.isEmpty())
    {
        httpSrvUrl = "http://" + ipAddr;
        if (!httpPort.isEmpty())
        {
             httpSrvUrl += ":" + httpPort;
        }
    }
    else
    {
        qCritical() << "Unable to get my own IP address";
    }
}

void JsonApi::ipChanged(QString newIp)
{
    if (!newIp.isEmpty())
    {
        httpSrvUrl = "http://" + newIp;
        if (!httpPort.isEmpty())
        {
             httpSrvUrl += ":" + httpPort;
        }
    }
}

QString JsonApi::convertImageUrl(QString url)
{
    QString out;

    if (url.isEmpty())
    {
        return out;
    }

    if (url.startsWith(cacheDirectory + QDir::separator()))
    {
        url.remove(0, cacheDirectory.size() + 1);

        QString uniformUrl; /* For windows, transform backslash in slash */
        QStringList dirs = url.split(QDir::separator());
        for (int i=0; i<dirs.size(); i++)
        {
            QString dir = dirs.at(i);
            if (i != 0)
            {
                uniformUrl += "/";
            }
            uniformUrl += dir;
        }

        if (!httpSrvUrl.isEmpty())
        {
            out = httpSrvUrl + "/" + url;
        }
    }
    else
    {
        qCritical() << "Local image is not in the cache directory : ";
        qCritical() << "Image path is " << url;
        qCritical() << "Cache path is " << cacheDirectory;
    }
    return out;
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
        QJsonObject answer;
        QJsonObject answerData;

        switch (toMessageType(j.object()["msg"].toString()))
        {
        case EMS_BROWSE:
            qDebug() << "QUERY BROWSE:" << j.object()["url"].toString();
            answerData = processMessageBrowse(j.object(), ret);
            /* Each query anwser have the same url */
            answer["url"] = j.object()["url"].toString();
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
        case EMS_AUTH:
            ret = processMessageAuthentication(j.object());
            break;
        case EMS_CD_RIP:
            ret = processMessageCDRip(j.object());
            break;
        case EMS_NETWORK:
            qDebug() << "QUERY NETWORK:"
                     << j.object()["action"].toString();
            ret = processMessageNetwork(j.object());
            break;
        default:
            ret = false;
            break;
        }

        if (!ret)
            return false;

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
    else if (type == "EMS_AUTH")
        return EMS_AUTH;
    else if (type == "EMS_CD_RIP")
        return EMS_CD_RIP;
    else if (type == "EMS_NETWORK")
        return EMS_NETWORK;
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
        obj = processMessageBrowseMenu(message, ok);
        break;
    case SCHEME_LIBRARY:
        obj = processMessageBrowseLibrary(message, ok);
        break;
    case SCHEME_CDDA:
        obj = processMessageBrowseCdrom(message, ok);
        break;
    case SCHEME_PLAYLIST:
        obj = processMessageBrowsePlaylist(message, ok);
        break;
    case SCHEME_FILE:
        obj = processMessageBrowseDirectory(message, ok);
        break;
    default:
        return obj;
    }

    ok = true;
    return obj;
}

QJsonObject JsonApi::processMessageBrowseMenu(const QJsonObject &message, bool &ok)
{
    Q_UNUSED(message);

    QJsonObject menus;
    QJsonArray arr;
    QJsonObject library;
    library["enabled"] = true;
    library["name"] = "Library";
    library["url_scheme"] = "library://";

    QJsonObject cdda;
    cdda["enabled"] = true;
    cdda["name"] = "CD";
    cdda["url_scheme"] = "cdda://";

    QVector<EMSCdrom> cdroms;
    CdromManager::instance()->getAvailableCdroms(&cdroms);
    cdda["enabled"] = !!cdroms.count();

    QJsonObject playlists;
    playlists["enabled"] = true;
    playlists["name"] = "Playlists";
    playlists["url_scheme"] = "playlist://";

    QJsonObject directories;
    directories["enabled"] = true;
    directories["name"] = "Directories";
    directories["url_scheme"] = "file://";

    QJsonObject settings;
    settings["enabled"] = true;
    settings["name"] = "Settings";
    settings["url_scheme"] = "settings://";

    arr.append(library);
    arr.append(cdda);
    arr.append(playlists);
    arr.append(directories);
    arr.append(settings);

    menus["menus"] = arr;
    ok = true;
    return menus;
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
    Database *db = Database::instance();

    switch (list.size())
    {
    case 1:
    {
        //get list of all artists
        QVector<EMSArtist> artistsList;
        db->lock();
        db->getArtistsList(&artistsList);
        db->unlock();
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
         db->lock();
         db->getAlbumsByArtistId(&albumsList, artistId);
         db->unlock();
         const int listSize = albumsList.size();
         QJsonArray jsonArray;
         for (int i = 0; i < listSize; ++i)
             jsonArray << EMSAlbumToJsonWithArtists(albumsList[i]);
         obj["albums"] = jsonArray;
         break;
    }
    case 3:
    {
        // get list of tracks of albumId of ArtistId
        albumId = list[2].toInt();
        QVector<EMSTrack> tracksList;
        db->lock();
        db->getTracksByAlbum(&tracksList, albumId);
        db->unlock();
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
    Database *db = Database::instance();

    switch (list.size())
    {
    case 1:
    {
        //get list of all albums
        QVector<EMSAlbum> albumsList;
        db->lock();
        db->getAlbumsList(&albumsList);
        db->unlock();
        const int listSize = albumsList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
            jsonArray << EMSAlbumToJsonWithArtists(albumsList[i]);
        obj["albums"] = jsonArray;
        ok = true;
        break;
    }
    case 2:
    {
        // get list of tracks of albumId
        albumId = list[1].toInt();
        QVector<EMSTrack> tracksList;
        db->lock();
        db->getTracksByAlbum(&tracksList, albumId);
        db->unlock();
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
    Database *db = Database::instance();

    switch (list.size())
    {
    case 1:
    {
        QVector<EMSTrack> tracksList;
        db->lock();
        db->getTracks(&tracksList);
        db->unlock();
        const int listSize = tracksList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
            jsonArray << EMSTrackToJson(tracksList[i]);
        obj["tracks"] = jsonArray;
        break;
    }
    case 2:
    {
        unsigned int trackId = list[1].toUInt();
        EMSTrack track;
        db->lock();
        db->getTrackById(&track, trackId);
        db->unlock();
        obj["track"] = EMSTrackToJson(track);
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
    Database *db = Database::instance();

    switch (list.size())
    {
    case 1:
    {
        QVector<EMSGenre> genresList;
        db->lock();
        db->getGenresList(&genresList);
        db->unlock();
        const int listSize = genresList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
            jsonArray << EMSGenreToJson(genresList[i]);
        obj["genres"] = jsonArray;
        break;
    }
    case 2:
    {
        // get list of albums by GenreID
        genreId = list[1].toInt();
        QVector<EMSAlbum> albumsList;
        db->lock();
        db->getAlbumsByGenreId(&albumsList, genreId);
        db->unlock();
        const int listSize = albumsList.size();
        QJsonArray jsonArray;
        for (int i = 0; i < listSize; ++i)
          jsonArray << EMSAlbumToJsonWithArtists(albumsList[i]);
        obj["albums"] = jsonArray;
        break;
    }
    case 3:
    {
        // get list of tracks of albumId of GenreID
        albumId = list[2].toInt();
        QVector<EMSTrack> tracksList;
        db->lock();
        db->getTracksByAlbum(&tracksList, albumId);
        db->unlock();
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

QJsonObject JsonApi::processMessageBrowsePlaylist(const QJsonObject &message, bool &ok)
{
    QJsonObject obj;
    Database *db = Database::instance();
    QString url = message["url"].toString();

    if (url.isEmpty())
    {
        ok = false;
        return obj;
    }
    url.remove("playlist://");
    QStringList list =url.split("/");

    if (url == "current")
    {
        EMSPlaylist playlist = Player::instance()->getCurentPlaylist();
        obj = EMSPlaylistToJson(playlist);
        ok = true;
    }
    else
    {
        switch (list.size())
        {
        case 1:
        {
            if (list[0] == "")
            {
                // get list of all playlists stored in the database
                EMSPlaylistsListBySubdir playlistsListBySubdir;
                db->lock();
                db->getPlaylistsList(&playlistsListBySubdir);
                db->unlock();

                QJsonArray jsonArray;
                obj =  EMSPlaylistsListBySubdirToJson(playlistsListBySubdir);
                ok = true;
            }
            else
            {
                // get the list of tracks of PlaylistId
                int playlistId = url.toInt();
                EMSPlaylist playlist;
                QVector<EMSTrack> tracksList;
                db->lock();
                db->getPlaylistById(&playlist, playlistId);
                db->getTracksByPlaylist(&tracksList, playlistId);
                db->unlock();
                const int listSize = tracksList.size();
                QJsonArray jsonArray;
                obj = EMSPlaylistToJsonWithoutTrack(playlist);
                for (int i = 0; i < listSize; ++i)
                    jsonArray << EMSTrackToJson(tracksList[i]);
                obj["tracks"] = jsonArray;
                ok = true;
            }
            break;
        }
        default:
            ok = false;
            break;
        }
    }
    return obj;
}

QJsonObject JsonApi::processMessageBrowseCdrom(const QJsonObject &message, bool &ok)
{
    QJsonObject obj;

    QString url = message["url"].toString();

    if (url.isEmpty())
    {
        ok = false;
        return obj;
    }
    url.remove("cdda://");

    /* If no id, we get all the CDROM tracks */
    // Unused variable for the moment
    // int position = -1;
    //if (!url.isEmpty()) /* Otherwise, we return only one track */
    //{
    //    position = url.toUInt();
    //}
    QVector<EMSCdrom> cdroms;
    CdromManager::instance()->getAvailableCdroms(&cdroms);
    if (cdroms.size() <= 0)
    {
        ok = false;
        return obj;
    }
    EMSCdrom cdrom = cdroms.at(0);
    QJsonArray jsonArray;
    for (int i=0; i<cdrom.tracks.size(); i++)
    {
        jsonArray << EMSTrackToJson(cdrom.tracks.at(i));
    }
    obj["tracks"] = jsonArray;

    ok = true;
    return obj;
}

QJsonObject JsonApi::processMessageBrowseDirectory(const QJsonObject &message, bool &ok)
{
    QJsonObject obj;
    QString url = message["url"].toString();

    if (!url.startsWith("file://"))
    {
        ok = false;
        return obj;
    }

    url.remove("file://");
    if (url.isEmpty())
        url = m_directoriesBasePath;

    QDir dir(url);
    dir.makeAbsolute();

    if (!dir.exists())
    {
        ok = false;
        return obj;
    }

    url = dir.absolutePath();

    if (!url.startsWith(m_directoriesBasePath))
    {
        ok = false;
        return obj;
    }

    dir.setNameFilters(m_supportedFormat);
    dir.setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::DirsFirst);

    QStringList allFiles = dir.entryList();
    QJsonArray jsonArray;
    for (int i=0; i< allFiles.size(); i++)
    {
        QJsonObject obj;
        QString newUrl = url + QDir::separator() + allFiles[i];
        obj["name"] = allFiles[i];

        QFileInfo f(newUrl);

        if (f.isDir())
            obj["fileType"] = "directory";
        else if (f.isFile())
            obj["fileType"] = "music";
        else
            obj["fileType"] = "unknown";
        obj["url"] = "file://" + newUrl;


        jsonArray << obj;
    }
    obj["files"] = jsonArray;
    ok = true;
    return obj;
}


bool JsonApi::processMessageDisk(const QJsonObject &message)
{
    Q_UNUSED(message);
    return true;
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
        /* For play action, the client can add field "pos" or "filename"
         * to specify which track should be played.
         * The track must be in the playlist.
         */
        if (!message["pos"].isNull() && !message["pos"].toString().isEmpty())
        {
            unsigned int pos = message["pos"].toString().toUInt();
            Player::instance()->play(pos);
        }
        else if (!message["filename"].isNull() && !message["filename"].toString().isEmpty())
        {
            QString filename = message["filename"].toString();
            QVector<EMSTrack> trackList;
            getTracksFromFilename(&trackList, filename);
            if (trackList.size() > 0)
            {
                Player::instance()->play(trackList.first());
            }
            else
            {
                qCritical() << "Error: filename does not match any track.";
                return false;
            }
        }
        else
        {
            Player::instance()->play();
        }
    }
    else if (action == "seek")
    {
        if (!message["value"].isNull() && !message["value"].toString().isEmpty())
        {
            unsigned int percent = message["value"].toString().toUInt();
            Player::instance()->seek(percent);
        }
        else
        {
            qCritical() << "Error: no value to seek.";
            return false;
        }
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
        bool ok = false;
        unsigned long long playlistId = 0;

        if (action != "create" && action != "save")
        {
            playlistId = type.toUInt(&ok);
            if (!ok)
            {
                qCritical() << "Error: extract the playlist Id from the url failed.";
                return false;
            }
        }

        Database *db = Database::instance();

        if (action == "create")
        {
            QString playlistName = message["name"].toString();
            QString playlistSubdir = message["subdir"].toString();
            db->lock();
            // check if the playlist already exists
            if (!db->checkPlaylistExist(playlistSubdir, playlistName))
            {
                db->insertNewPlaylist(playlistSubdir, playlistName);
            }
            else
            {
                qDebug() << "JsonApi: the playlist allready exists";
            }
            db->unlock();
        }
        else if (action == "save")
        {
            // Save the current playlist
            QString playlistName = message["name"].toString();
            QString playlistSubdir = message["subdir"].toString();
            db->lock();
            // create the playlist if does not already exist
            if (!db->checkPlaylistExist(playlistSubdir, playlistName, &playlistId))
            {
                db->insertNewPlaylist(playlistSubdir, playlistName, &playlistId);
            }
            // append the tracks id of the current playlist
            EMSPlaylist currentPlaylist = Player::instance()->getCurentPlaylist();
            foreach (EMSTrack track, currentPlaylist.tracks)
            {
                db->addTrackInPlaylist(playlistId, track.id);
            }
            db->unlock();
        }
        else if (action == "load")
        {
            // Load one saved playlist into the current playlist
            db->lock();
            // 1- Get the tracks list
            QVector<EMSTrack> trackList;
            db->getTracksByPlaylist(&trackList, playlistId);
            // 2- Delete the tracks of the current playlist
            Player::instance()->removeAllTracks();
            // 3- Add the tracks list to the current playlist
            foreach (EMSTrack track, trackList)
            {
                Player::instance()->addTrack(track);
            }
            db->unlock();
            // 4- Start the playlist reading
            Player::instance()->play();
        }
        else if (action == "del" && message["filename"].toString().isEmpty())
        {
            // Delete the playlist
            db->lock();
            db->deletePlaylist(playlistId);
            db->unlock();
        }
        else if (action == "add" || action == "del")
        {
            QVector<EMSTrack> trackList;

            getTracksFromFilename(&trackList, message["filename"].toString());
            if (trackList.size() > 0)
            {
                db->lock();
                for (int i=0; i<trackList.size(); i++)
                {
                    if (action == "add")
                    {
                        db->addTrackInPlaylist(playlistId, trackList.at(i).id);
                    }
                    else if (action == "del")
                    {
                        db->removeTrackFromPlaylist(playlistId, trackList.at(i).id);
                    }
                }
                db->unlock();
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
    return true;
}


bool JsonApi::processMessageAuthentication(const QJsonObject &message)
{
    EMSClient acceptedClient;
    QString status = message["status"].toString();
    bool isAccepted  = false;
    Database *db = Database::instance();
    bool returnValue = false;     // 'false' if an error occurs

    if (status == "accepted")
        isAccepted = true;
    else if (status == "rejected" )
        isAccepted = false;
    else
    {
        qWarning() << "JsonApi: invalid status for the message authentication";
        return false;
    }

    if (isAccepted)
    {
        // Accepted client usecase
        db->lock();
        if (false == db->getAuthorizedClient(message["uuid"].toString(), &acceptedClient))
        {
            acceptedClient.uuid = message["uuid"].toString();
            acceptedClient.hostname = message["hostname"].toString();
            acceptedClient.username = message["username"].toString();

            if (db->insertNewAuthorizedClient(&acceptedClient))
                returnValue = true;
            else
                qWarning() << "Error JsonApi: can not insert client in db";
        }
        db->unlock();
    }
    else
    {
        // Rejected client usecase
        returnValue = true;
    }
    return returnValue;
}


bool JsonApi::processMessageCDRip(const QJsonObject &message)
{
    CdromManager *cdromManager = CdromManager::instance();

    if (cdromManager->isRipInProgress())
    {
        qDebug() << "JsonApi: rip already in progress!";
        return false;
    }
    else
    {
        cdromManager->setRipAudioFormat(message["audio_format"].toString());
        connect(this, &JsonApi::startCdromRip, cdromManager, &CdromManager::startRip);
        emit startCdromRip();
    }

    return true;
}


/* Handle network query
 * The field action should be :
 * - getWifiList : First scan the wifi and then get the list of wifi
 *                 after wifiListUpdated signal reception.
 * - connectWifi
 */
bool JsonApi::processMessageNetwork(const QJsonObject &message)
{
    QString action = message["action"].toString();
    bool ok = false;
    if (action == "is_wifi_enabled")
    {

        QJsonObject answer;
        bool wifiState = NetworkCtl::instance()->isWifiEnabled();

        answer["msg"] = message["msg"];
        answer["msg_id"] = message["msg_id"];
        answer["uuid"] = message["uuid"];
        answer["wifi_state"] = wifiState;
        QJsonDocument doc(answer);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
    else if (action == "connected_ssid_get")
    {
        EMSSsid* ssid=NetworkCtl::instance()->getConnectedWifi();
        QJsonObject answer;
        answer["msg"] = message["msg"];
        answer["msg_id"] = message["msg_id"];
        answer["uuid"] = message["uuid"];
        answer["connexion_result"] ="connected";
        answer["ssid"] = EMSSsidToJson(*ssid);
        QJsonDocument doc(answer);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
    else if (action == "connected_ethernet_get")
    {
        EMSEthernet* ethData=NetworkCtl::instance()->getConnectedEthernet();
        QJsonObject answer;
        answer["msg"] = message["msg"];
        answer["msg_id"] = message["msg_id"];
        answer["uuid"] = message["uuid"];
        answer["ethernet"] = EMSEthernetToJson(*ethData);
        QJsonDocument doc(answer);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
    else if (action == "update_network")
    {
        NetworkCtl::instance()->setEnableUpdate(message["update_enabled"].toBool());
        qDebug() << "Update enabled : " << message["update_enabled"].toBool();
    }
    else if (action == "wifi_disable")
    {
        NetworkCtl::instance()->enableWifi(false);
        qDebug() << "wifi disabled";
    }
    else if (action == "wifi_enable")
    {
        NetworkCtl::instance()->enableWifi(true);
        qDebug() << "wifi enabled";
    }
    else if (action == "ssid_list_get")
    {
        if (!NetworkCtl::instance()->isWifiEnabled())
        {
            NetworkCtl::instance()->enableWifi(true);
            auto connEnable = std::make_shared<QMetaObject::Connection>();
            *connEnable = connect(NetworkCtl::instance()->getTechnology("wifi"), &Technology::poweredChanged, [=]
            {
                NetworkCtl::instance()->scanWifi();
                NetworkCtl::instance()->getTechnology("wifi")->scan();
                disconnect(*connEnable);
            });
        }
        else
        {
            NetworkCtl::instance()->scanWifi();
        }

        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(NetworkCtl::instance(), &NetworkCtl::wifiListUpdated, [=]
        {
            QJsonArray jsonArray;
            QJsonObject obj;
            qDebug() << "Wifi updated";
            QList<EMSSsid> ssidList = NetworkCtl::instance()->getWifiList();
            for (int i = 0; i<ssidList.size();i++)
            {
                jsonArray << EMSSsidToJson(ssidList[i]);
            }

            obj["ssids"] = jsonArray;
            QJsonObject answer;
            answer["msg"] = message["msg"];
            answer["msg_id"] = message["msg_id"];
            answer["uuid"] = message["uuid"];
            answer["data"] = obj;
            QJsonDocument doc(answer);
            m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

            disconnect(*conn);
        });
    }
    else if (action == "wifi_connect")
    {
        QString ssid = message["ssid"].toString();
        QString password = message["password"].toString();
        if (NetworkCtl::instance()->isWifiPresent())
        {
            if (!NetworkCtl::instance()->isWifiEnabled())
            {
                NetworkCtl::instance()->enableWifi(true);
            }
            bool connected = NetworkCtl::instance()->isWifiConnected();
            QString connSsid=NetworkCtl::instance()->getConnectedWifi()->getName();
            if (!connected || (connected && ssid != connSsid))
            {
                NetworkCtl::instance()->scanWifi();
                //connexion to the desired wifi : ssid
                auto connWifiList = std::make_shared<QMetaObject::Connection>();
                auto connError = std::make_shared<QMetaObject::Connection>();
                auto connStatus = std::make_shared<QMetaObject::Connection>();
                auto connPassphrase = std::make_shared<QMetaObject::Connection>();
                *connWifiList = connect(NetworkCtl::instance(), &NetworkCtl::wifiListUpdated,[=]()
                {
                    Service*  wifiService = NetworkCtl::instance()->getWifiByName(ssid);
                    if (wifiService)
                    {
                        EMSEthernet* ethServiceEMS = NetworkCtl::instance()->getConnectedEthernet();
                        Service*  ethService = NetworkCtl::instance()->getEthByPath(ethServiceEMS->getPath());
                        //ethService->disconnect();
                        wifiService->connect();
                    }
                    else
                    {
                        qDebug() << "Wrong SSID";
                        disconnect(*connPassphrase);
                        disconnect(*connError);
                        disconnect(*connStatus);
                        disconnect(*connWifiList);
                    }
                    disconnect(*connWifiList);
                });

                // Authentication for secured connexion with password
                *connPassphrase = connect(NetworkCtl::instance()->getAgent(), &Agent::passphraseRequested, [=]()
                {
                    qDebug() << "Authentication required";
                    Agent::InputRequest *request = NetworkCtl::instance()->getAgent()->currentInputRequest();
                    request->response.passphrase = password;
                    qDebug() << "Password sent";

                    disconnect(*connWifiList);
                    disconnect(*connPassphrase);
                });

                //Authentication error manager
                *connError = connect(NetworkCtl::instance()->getAgent(), &Agent::errorRaised,[=]()
                {
                    Agent::ErrorRequest *requestErr = NetworkCtl::instance()->getAgent()->currentErrorRequest();
                    QJsonObject answer;
                    answer["msg"] = message["msg"];
                    answer["msg_id"] = message["msg_id"];
                    answer["uuid"] = message["uuid"];
                    answer["connexion_result"] = requestErr->error;
                    QJsonDocument doc(answer);
                    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

                    disconnect(*connStatus);
                    disconnect(*connWifiList);
                    disconnect(*connPassphrase);
                    disconnect(*connError);
                });
            }
            else // already conected to ssid
            {
                qDebug()<<"Wifi already conected";
                QJsonObject answer;
                answer["msg"] = message["msg"];
                answer["msg_id"] = message["msg_id"];
                answer["uuid"] = message["uuid"];
                answer["connexion_result"] ="Already connected";
                QJsonDocument doc(answer);
                m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
            }
        }
        else
        {
            qCritical()<< "Error: No Wifi present.";
        }
    }

    return ok;
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
    else if (scheme == "file")
        return SCHEME_FILE;
    else
        return SCHEME_UNKNOWN;
}

QJsonObject JsonApi::EMSArtistToJson(const EMSArtist &artist)
{
    QJsonObject obj;

    obj["id"] = (qint64)artist.id;
    obj["name"] = artist.name;
    obj["picture"] = convertImageUrl(artist.picture);

    return obj;
}

QJsonObject JsonApi::EMSAlbumToJson(const EMSAlbum &album)
{
    QJsonObject obj;

    obj["id"] = (qint64)album.id;
    obj["name"] = album.name;
    obj["picture"] = convertImageUrl(album.cover);

    return obj;
}

QJsonObject JsonApi::EMSAlbumToJsonWithArtists(const EMSAlbum &album)
{
    Database *db = Database::instance();
    QVector<EMSArtist> artists;
    QJsonObject obj = EMSAlbumToJson(album);

    db->lock();
    db->getArtistsByAlbumId(&artists, album.id);
    db->unlock();

    QJsonArray artistsJson;
    foreach(EMSArtist artist, artists)
    {
        artistsJson << EMSArtistToJson(artist);
    }
    obj["artists"] = artistsJson;

    return obj;
}

QJsonObject JsonApi::EMSGenreToJson(const EMSGenre &genre)
{
    QJsonObject obj;

    obj["id"] = (qint64)genre.id;
    obj["name"] = genre.name;
    obj["picture"] = convertImageUrl(genre.picture);

    return obj;
}

QString JsonApi::EMSTrackTypeToString(EMSTrackType type) const
{
    switch(type)
    {
    case TRACK_TYPE_CDROM:
        return QString("cdrom");
    case TRACK_TYPE_DB:
        return QString("database");
    case TRACK_TYPE_EXTERNAL:
        return QString("file");
    default:
        return QString("unknown");
    }
}

QJsonObject JsonApi::EMSTrackToJson(const EMSTrack &track)
{
    QJsonObject obj;

    obj["id"] = (qint64)track.id;
    obj["position"] = (int)track.position;
    obj["type"] = EMSTrackTypeToString(track.type);
    obj["name"] = track.name;
    obj["filename"] = track.filename;
    obj["sha1"] = track.sha1;
    obj["format"] = track.format;
    obj["sample_rate"] = (int)track.sample_rate;
    obj["duration"] = (int)track.duration;
    obj["format_parameters"] = track.format_parameters;
    obj["album"] = EMSAlbumToJson(track.album);

    QJsonArray artists;
    foreach(EMSArtist artist, track.artists)
    {
        artists << EMSArtistToJson(artist);
    }
    obj["artists"] = artists;

    QJsonArray genres;
    foreach(EMSGenre genre, track.genres)
    {
        genres << EMSGenreToJson(genre);
    }
    obj["genres"] = genres;

    return obj;
}

QJsonObject JsonApi::EMSPlaylistToJsonWithoutTrack(EMSPlaylist playlist)
{
    QJsonObject obj;

    obj["playlist_id"] = (qint64)playlist.id;
    obj["playlist_subdir"] = playlist.subdir;
    obj["playlist_name"] = playlist.name;

    return obj;
}

QJsonObject JsonApi::EMSPlaylistToJson(EMSPlaylist playlist)
{
    QJsonObject obj;

    obj["playlist_id"] = (qint64)playlist.id;
    obj["playlist_subdir"] = playlist.subdir;
    obj["playlist_name"] = playlist.name;

    QJsonArray jsonArray;
    foreach (EMSTrack track, playlist.tracks)
    {
        jsonArray << EMSTrackToJson(track);
    }
    obj["tracks"] = jsonArray;

    return obj;
}

QJsonObject JsonApi::EMSPlaylistsListBySubdirToJson(EMSPlaylistsListBySubdir playlistsListBySubdir)
{
    QJsonObject playlistsListObj;
    QJsonArray listsJsonArray;
    foreach (QString subdir, playlistsListBySubdir.keys())
    {
        QJsonObject subdirObj;
        subdirObj["subdir"] = subdir;
        QJsonArray jsonArray;
        QVector<EMSPlaylist> playlists = playlistsListBySubdir.value(subdir);

        foreach (EMSPlaylist playlist, playlists)
        {
            QJsonObject playlistJson;
            playlistJson["playlist_id"] = (qint64)playlist.id;
            playlistJson["playlist_name"] = playlist.name;

            jsonArray << playlistJson;
        }
        subdirObj["playlists"] = jsonArray;

        listsJsonArray << subdirObj;
    }
    playlistsListObj["playlists_by_subdir"] = listsJsonArray;

    return playlistsListObj;
}

QJsonObject JsonApi::EMSSsidToJson(const EMSSsid &ssid) const
{
    QJsonObject obj;
    obj["strength"] = ssid.getStrength();
    obj["path"] = ssid.getPath();
    obj["name"] = ssid.getName();
    obj["type"] = ssid.getType();
    obj["state"] = ssid.getState();
    obj["security"] = ssid.getSecurity().join(", ");

    return obj;
}

QJsonObject JsonApi::EMSEthernetToJson(const EMSEthernet &ethData) const
{
    QJsonObject obj;
    obj["type"] = ethData.getType();
    obj["path"] = ethData.getPath();
    obj["interface"] = ethData.getInterface();
    obj["state"] = ethData.getState();
    obj["ip_address"] = ethData.getIpAddress();

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
    Database *db = Database::instance();

    if (filename.startsWith("cdda://"))
    {
        /* Get first available CDROM */
        QVector<EMSCdrom> cdroms;
        CdromManager::instance()->getAvailableCdroms(&cdroms);
        if (cdroms.size() <= 0)
        {
            return;
        }
        EMSCdrom cdrom = cdroms.at(0);
        QString trackID = filename.remove("cdda://");
        if (trackID.isEmpty())
        {
            for (int i=0; i<cdrom.tracks.size(); i++)
            {
                EMSTrack track = cdrom.tracks.at(i);
                trackList->append(track);
            }
        }
        else
        {
            unsigned int position = trackID.toUInt();
            for (int i=0; i<cdrom.tracks.size(); i++)
            {
                EMSTrack track = cdrom.tracks.at(i);
                if (track.position == position)
                {
                    trackList->append(track);
                    break;
                }
            }
        }
    }
    else if (filename.startsWith("library://music/albums/"))
    {
        QString albumID = filename.remove("library://music/albums/");
        if (!albumID.isEmpty())
        {
            unsigned long long id = albumID.toULongLong();
            db->lock();
            db->getTracksByAlbum(trackList, id);
            db->unlock();
        }
    }
    else if (filename.startsWith("library://music/tracks/"))
    {
        QString trackID = filename.remove("library://music/tracks/");
        if (!trackID.isEmpty())
        {
            unsigned long long id = trackID.toULongLong();
            EMSTrack track;
            db->lock();
            bool success = db->getTrackById(&track, id);
            db->unlock();
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
    QJsonObject statusJsonObj;
    statusJsonObj["msg"] = "EMS_PLAYLIST";
    statusJsonObj["data"] = EMSPlaylistToJson(newPlaylist);
    QJsonDocument doc(statusJsonObj);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void JsonApi::sendAuthRequest(EMSClient client)
{
    QJsonObject authRequestJsonObj;
    authRequestJsonObj["msg"]    = "EMS_AUTH";
    authRequestJsonObj["status"] = "request";
    authRequestJsonObj["uuid"]   = client.uuid;
    authRequestJsonObj["hostname"]  = client.hostname;
    authRequestJsonObj["username"]  = client.username;

    QJsonDocument doc(authRequestJsonObj);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    qDebug() << "JsonApi: sent the 'authentication' request for " << client.uuid;
}

void JsonApi::sendRipProgress(EMSRipProgress ripProgress)
{
    QJsonObject ripProgressJsonObj;
    ripProgressJsonObj["msg"] = "EMS_CD_RIP";
    ripProgressJsonObj["overall_progress"] = (qint64)ripProgress.overall_progress;
    ripProgressJsonObj["track_in_progress"] = (qint64)ripProgress.track_in_progress;
    ripProgressJsonObj["track_total"] = (qint64)ripProgress.track_total;
    ripProgressJsonObj["track_progress"] = (qint64)ripProgress.track_progress;

    QJsonDocument doc(ripProgressJsonObj);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void JsonApi::sendMenu()
{
    QJsonObject unused;
    bool ok;
    QJsonObject obj;
    obj["msg"] = "EMS_MENUS";
    obj["data"] = processMessageBrowseMenu(unused, ok);
    QJsonDocument doc(obj);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void JsonApi::sendWifiConnected()
{
    if(NetworkCtl::instance()->isWifiConnected())
    {
        EMSSsid* ssid=NetworkCtl::instance()->getConnectedWifi();
        qDebug()<<"Connected to: "<< ssid->getName();
        QJsonObject connectedWifiJsonObj;
        connectedWifiJsonObj["msg"] = "EMS_NETWORK";
        connectedWifiJsonObj["connexion_result"] ="connected";
        connectedWifiJsonObj["ssid"] = EMSSsidToJson(*ssid);
        QJsonDocument doc(connectedWifiJsonObj);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
    else
    {
        qDebug()<<"Wifi disconnected ";
        QJsonObject connectedWifiJsonObj;
        connectedWifiJsonObj["msg"] = "EMS_NETWORK";
        connectedWifiJsonObj["connexion_result"] ="disconnected";
        QJsonDocument doc(connectedWifiJsonObj);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
}

void JsonApi::sendEthConnected()
{
    QJsonObject answer;
    if(NetworkCtl::instance()->isEthernetConnected())
    {
        EMSEthernet* ethData=NetworkCtl::instance()->getConnectedEthernet();
        answer["msg"] = "EMS_NETWORK";
        answer["ethernet"] = EMSEthernetToJson(*ethData);
        QJsonDocument doc(answer);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
    else
    {
        qDebug()<<"Eth disconnected ";
        EMSEthernet* ethParam = new EMSEthernet(" "," "," ","offline"," ");
        answer["msg"] = "EMS_NETWORK";
        answer["ethernet"] = EMSEthernetToJson(*ethParam);
        QJsonDocument doc(answer);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
}

void JsonApi::sendWifiList()
{
    if(NetworkCtl::instance()->getEnableUpdate())
    {
        QJsonArray jsonArray;
        QJsonObject obj;
        QList<EMSSsid> ssidList = NetworkCtl::instance()->getWifiList();
        qDebug() << "Wifi List updated";

        for(int i = 0; i<ssidList.size();i++)
        {
            jsonArray << EMSSsidToJson(ssidList[i]);
        }

        obj["ssids"] = jsonArray;
        QJsonObject wifiListJsonObj;
        wifiListJsonObj["msg"] = "EMS_NETWORK";
        wifiListJsonObj["data"] = obj;
        QJsonDocument doc(wifiListJsonObj);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
    else
    {
        qDebug() << "Wifi List update disabled ";
    }

}
