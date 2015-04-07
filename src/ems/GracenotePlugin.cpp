#include <QSettings>

#include "DefaultSettings.h"
#include "GracenotePlugin.h"
#include "CdromManager.h"

#define CLIENT_APP_VERSION "1.0.0.0"
#define EMS_GRACENOTE_USER_HANDLE_FILE "user.txt"

GracenotePlugin::GracenotePlugin()
{
    QSettings settings;

    capabilities << "discid";
    /* Not implemented :
    capabilities << "fingerprint";
    capabilities << "album_name";
    capabilities << "track_name";
    capabilities << "track_gnid";
    capabilities << "album_name";
    capabilities << "album_gnid";
    */

    userHandle = GNSDK_NULL;
    configured = false;

    EMS_LOAD_SETTINGS(clientID, "gracenote/client_id", "", String);
    EMS_LOAD_SETTINGS(clientIDTags, "gracenote/client_id_tags", "", String);
    EMS_LOAD_SETTINGS(licensePath, "gracenote/license_path", "", String);
    EMS_LOAD_SETTINGS(userHandlePath, "gracenote/user_handle_file", EMS_GRACENOTE_USER_HANDLE_FILE, String);

    /* Download directory */
    QString cacheDirPath;
    EMS_LOAD_SETTINGS(cacheDirPath, "main/cache_directory",
                      QStandardPaths::standardLocations(QStandardPaths::CacheLocation)[0], String);
    QDir cacheDir(cacheDirPath);
    cacheDir.mkpath("gracenote");
    QDir gracenoteCacheDir(cacheDirPath + QDir::separator() + "gracenote");
    gracenoteCacheDir.mkdir("albums");
    gracenoteCacheDir.mkdir("genres");
    gracenoteCacheDir.mkdir("artists");
    albumsCacheDir = cacheDirPath + QDir::separator() + "gracenote" + QDir::separator() + "albums";
    genresCacheDir = cacheDirPath + QDir::separator() + "gracenote" + QDir::separator() + "genres";
    artistsCacheDir = cacheDirPath + QDir::separator() + "gracenote" + QDir::separator() + "artists";
}

GracenotePlugin::~GracenotePlugin()
{

}

bool GracenotePlugin::update(EMSTrack *track)
{
    if(!configured)
    {
        configured = configure();
        if (!configured)
        {
            qDebug() << "Failed to configure connection to Gracenote server";
            qDebug() << "Have you set a valid licence in the configuration file ?";
            return false;
        }
        qDebug() << "Connected to Gracenote server";
    }

    if (track->type == TRACK_TYPE_CDROM)
    {
        EMSCdrom cdrom;
        if (CdromManager::instance()->getCdrom(track->filename, &cdrom))
        {
            lookupByDiscID(track, cdrom);
        }
    }
    return true;
}

void GracenotePlugin::displayLastError()
{
    const gnsdk_error_info_t* error_info = gnsdk_manager_error_info();
    qCritical() << "Gracenote error : "
                << QString().sprintf("(%s) : %08x %s", error_info->error_api, error_info->error_code, error_info->error_description);
}

bool GracenotePlugin::lookupByDiscID(EMSTrack *track, EMSCdrom cdrom)
{
    gnsdk_error_t error = GNSDK_SUCCESS;
    gnsdk_musicid_query_handle_t queryHandle = GNSDK_NULL;
    gnsdk_gdo_handle_t findResult = GNSDK_NULL;
    gnsdk_gdo_handle_t albumGdo = GNSDK_NULL;
    gnsdk_uint32_t count = 0;
    gnsdk_uint32_t choice = 0;
    gnsdk_cstr_t full = GNSDK_NULL;

    /* Create the query handle */
    error = gnsdk_musicid_query_create(userHandle, GNSDK_NULL, GNSDK_NULL, &queryHandle);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        return false;
    }

    /* The disc ID is formated like : ID TRACKNUM TOC DURATION */
    QString toc;
    QStringList parts = cdrom.disc_id.split(' ');
    for (int i=2; i<(parts.size()-1); i++)
    {
        if (!toc.isEmpty())
        {
            toc += " ";
        }
        toc += parts.at(i);
    }

    /* Set TOC */
    error = gnsdk_musicid_query_set_toc_string(queryHandle, toc.toStdString().c_str());
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_musicid_query_release(queryHandle);
        return false;
    }

    /* Perform the query */
    error = gnsdk_musicid_query_find_albums(queryHandle, &findResult);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_musicid_query_release(queryHandle);
        return false;
    }

    /* Get how many album match */
    error = gnsdk_manager_gdo_child_count(findResult, GNSDK_GDO_CHILD_ALBUM, &count);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_manager_gdo_release(findResult);
        gnsdk_musicid_query_release(queryHandle);
        return false;
    }

    if (count == 0)
    {
        qDebug() << "No disc info for this album.";
        gnsdk_manager_gdo_release(findResult);
        gnsdk_musicid_query_release(queryHandle);
        return true;
    }

    /* Choose the first album */
    choice = 1;

    error = gnsdk_manager_gdo_child_get(findResult, GNSDK_GDO_CHILD_ALBUM, choice, &albumGdo);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_manager_gdo_release(findResult);
        gnsdk_manager_gdo_release(albumGdo);
        gnsdk_musicid_query_release(queryHandle);
        return false;
    }

    error = gnsdk_manager_gdo_value_get(albumGdo, GNSDK_GDO_VALUE_FULL_RESULT, 1, &full);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_manager_gdo_release(findResult);
        gnsdk_manager_gdo_release(albumGdo);
        gnsdk_musicid_query_release(queryHandle);
        return false;
    }

    /* If partial result, do again the lookup but using albumGdo instead of the TOC */
    if (!strcmp(full, GNSDK_VALUE_FALSE))
    {
        error = gnsdk_musicid_query_set_gdo(queryHandle, albumGdo);
        if (error != GNSDK_SUCCESS)
        {
            displayLastError();
            gnsdk_manager_gdo_release(findResult);
            gnsdk_manager_gdo_release(albumGdo);
            gnsdk_musicid_query_release(queryHandle);
            return false;
        }

        /* New search */
        gnsdk_manager_gdo_release(findResult);
        error = gnsdk_musicid_query_find_albums(queryHandle, &findResult);
        if (error != GNSDK_SUCCESS)
        {
            displayLastError();
            gnsdk_musicid_query_release(queryHandle);
            return false;
        }

        /* Get new album with more data */
        gnsdk_manager_gdo_release(albumGdo);
        error = gnsdk_manager_gdo_child_get(findResult, GNSDK_GDO_CHILD_ALBUM, 1, &albumGdo);
        if (error != GNSDK_SUCCESS)
        {
            displayLastError();
            gnsdk_manager_gdo_release(findResult);
            gnsdk_musicid_query_release(queryHandle);
            return false;
        }
    }

    /* We have found the album, update the given track object */
    albumGdoToEMSTrack(albumGdo, track);

    gnsdk_manager_gdo_release(albumGdo);
    gnsdk_manager_gdo_release(findResult);
    gnsdk_musicid_query_release(queryHandle);
    return true;
}

bool GracenotePlugin::albumGdoToEMSTrack(gnsdk_gdo_handle_t albumGdo, EMSTrack *track)
{
    gnsdk_error_t error = GNSDK_SUCCESS;
    gnsdk_cstr_t value = GNSDK_NULL;
    gnsdk_link_query_handle_t queryHandle = GNSDK_NULL;

    /* Album GNID */
    QString gnid;
    error = gnsdk_manager_gdo_value_get(albumGdo, GNSDK_GDO_VALUE_GNID, 1, &value);
    if (error != GNSDK_SUCCESS)
    {
        qCritical() << "No GNID for this album.";
        return false;
    }
    gnid = value;

    /* Album Title */
    gnsdk_gdo_handle_t titleGdo = GNSDK_NULL;
    error = gnsdk_manager_gdo_child_get(albumGdo, GNSDK_GDO_CHILD_TITLE_OFFICIAL, 1, &titleGdo);
    if (error == GNSDK_SUCCESS)
    {
        error = gnsdk_manager_gdo_value_get(titleGdo, GNSDK_GDO_VALUE_DISPLAY, 1, &value);
        if (error == GNSDK_SUCCESS)
        {
            track->album.name = value;
        }
        gnsdk_manager_gdo_release(titleGdo);
    }

    /* Album cover */
    gnsdk_link_data_type_t dataType = gnsdk_link_data_unknown;
    gnsdk_byte_t* buffer = GNSDK_NULL;
    gnsdk_size_t bufferSize = 0;

    /* Look for an already downloaded cover */
    if (QFile(albumsCacheDir + QDir::separator() + gnid + ".unknown").exists())
    {
        track->album.cover = albumsCacheDir + QDir::separator() + gnid + ".unknown";
    }
    else if (QFile(albumsCacheDir + QDir::separator() + gnid + ".jpeg").exists())
    {
        track->album.cover = albumsCacheDir + QDir::separator() + gnid + ".jpeg";
    }
    else if (QFile(albumsCacheDir + QDir::separator() + gnid + ".png").exists())
    {
        track->album.cover = albumsCacheDir + QDir::separator() + gnid + ".png";
    }
    /* Otherwise get the largest available pictures*/
    else
    {
        error = gnsdk_link_query_create(userHandle, GNSDK_NULL, GNSDK_NULL, &queryHandle);
        if (error != GNSDK_SUCCESS)
        {
            displayLastError();
            return false; /* Should not happen */
        }
        gnsdk_link_query_set_gdo(queryHandle, albumGdo);
        gnsdk_link_query_option_set(queryHandle, GNSDK_LINK_OPTION_KEY_IMAGE_SIZE, GNSDK_LINK_OPTION_VALUE_IMAGE_SIZE_1080);
        error = gnsdk_link_query_content_retrieve(queryHandle, gnsdk_link_content_cover_art, 1, &dataType, &buffer, &bufferSize);
        /* A cover has been found, save it in the cache */
        if (error == GNSDK_SUCCESS)
        {
            QString extension = ".unknown";
            if (dataType == gnsdk_link_data_image_jpeg)
            {
                extension = ".jpeg";
            }
            else if (dataType == gnsdk_link_data_image_png)
            {
                extension = ".png";
            }
            QFile cover(albumsCacheDir + QDir::separator() + gnid + extension);
            if (!cover.exists() && cover.open(QIODevice::WriteOnly))
            {
                cover.write((const char*)buffer, bufferSize);
                cover.close();
                track->album.cover = cover.fileName();
            }
            error = gnsdk_link_query_content_free(buffer);
        }
        gnsdk_link_query_release(queryHandle);
    }

    /* Retrieve the track inside the AlbumGdo */
    //TODO
}

bool GracenotePlugin::configure()
{
    gnsdk_manager_handle_t sdkmgrHandle = GNSDK_NULL;
    gnsdk_error_t error = GNSDK_SUCCESS;

    /* Check all required data are filled */
    if(licensePath.isEmpty() || clientID.isEmpty() || clientIDTags.isEmpty())
    {
        return false;
    }

    /* Initialize the GNSDK Manager */
    error = gnsdk_manager_initialize(&sdkmgrHandle, licensePath.toStdString().c_str(), GNSDK_MANAGER_LICENSEDATA_FILENAME);
    if (error != GNSDK_SUCCESS)
    {
        return false;
    }

    /* Initialize the Storage SQLite Library */
    error = gnsdk_storage_sqlite_initialize(sdkmgrHandle);
    if (error != GNSDK_SUCCESS)
    {
        return false;
    }

    /* Initialize the MusicID Library */
    error = gnsdk_musicid_initialize(sdkmgrHandle);
    if (error != GNSDK_SUCCESS)
    {
        return false;
    }

    /* Initialize the Link Library */
    error = gnsdk_link_initialize(sdkmgrHandle);
    if (error != GNSDK_SUCCESS)
    {
        return false;
    }

    /* Get a user handle for our client ID.  This will be passed in for all queries */
    if (getUserHandle())
    {
        return true;
    }
    else
    {
        gnsdk_manager_user_release(userHandle);
        userHandle = GNSDK_NULL;
        gnsdk_manager_shutdown();
        return false;
    }
}

bool GracenotePlugin::getUserHandle()
{
    gnsdk_str_t serializedUser = GNSDK_NULL;
    gnsdk_char_t serializedUserBuf[1024] = {0};
    gnsdk_error_t error = GNSDK_SUCCESS;
    FILE* file = NULL;

    /* Case 1 : a file contains serialized user data, no need to ask them again */
    //TODO: use "settings" in database to store it
    file = fopen(userHandlePath.toStdString().c_str(), "r");
    if (file)
    {
        fgets(serializedUserBuf, sizeof(serializedUserBuf), file);
        fclose(file);

        error = gnsdk_manager_user_create(serializedUserBuf, clientID.toStdString().c_str(), &userHandle);
        if (GNSDK_SUCCESS == error)
        {
            return true;
        }
        gnsdk_manager_user_release(userHandle);
    }

    /* Case 2 : no local info, register online */
    error = gnsdk_manager_user_register(GNSDK_USER_REGISTER_MODE_ONLINE, clientID.toStdString().c_str(),
                                        clientIDTags.toStdString().c_str(), CLIENT_APP_VERSION,
                                        &serializedUser);
    if (error != GNSDK_SUCCESS)
    {
        return false;
    }

    /* Create the user handle from the newly registered user */
    error = gnsdk_manager_user_create(serializedUser, clientID.toStdString().c_str(), &userHandle);
    if (error != GNSDK_SUCCESS)
    {
        return false;
    }

    /* Save the user data for the next time */
    file = fopen(userHandlePath.toStdString().c_str(), "w");
    if (file)
    {
        fputs(serializedUser, file);
        fclose(file);
    }
    gnsdk_manager_string_free(serializedUser);
    return true;
}
