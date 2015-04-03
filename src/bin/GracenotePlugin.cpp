#include <QSettings>

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

    clientID = settings.value("gracenote/client_id", "").toString();
    clientIDTags = settings.value("gracenote/client_id_tags", "").toString();
    licensePath = settings.value("gracenote/license_path", "").toString();
    userHandlePath = settings.value("gracenote/user_handle_file", EMS_GRACENOTE_USER_HANDLE_FILE).toString();
}

GracenotePlugin::~GracenotePlugin()
{

}

bool GracenotePlugin::update(EMSTrack *track)
{
    if(!configured)
    {
        configured = configure();
    }
    if (!configured)
    {
        emit updated(track);
        return false;
    }

    if (track->type == TRACK_TYPE_CDROM)
    {
        EMSCdrom cdrom;
        if (CdromManager::instance()->getCdrom(track->filename, &cdrom))
        {
            lookupByDiscID(track, cdrom);
        }
    }
    emit updated(track);
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
    qDebug() << "Computed TOC : " << toc;

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

    //TODO
    //display_albumGdo(albumGdo);
    gnsdk_manager_gdo_release(albumGdo);
    gnsdk_manager_gdo_release(findResult);
    gnsdk_musicid_query_release(queryHandle);
    return true;
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
