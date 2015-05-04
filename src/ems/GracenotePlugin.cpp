#include <QSettings>
#include <cdio/cdio.h>

#include "DefaultSettings.h"
#include "GracenotePlugin.h"
#include "CdromManager.h"
#include "Database.h"

#define CLIENT_APP_VERSION "1.0.0.0"
#define EMS_GRACENOTE_USER_HANDLE_FILE "user.txt"

GracenotePlugin::GracenotePlugin()
{
    QSettings settings;

    capabilities << "discid";
    capabilities << "text";
    capabilities << "fingerprint";

    imageFormats.clear();
    imageFormats << ".jpeg";
    imageFormats << ".png";

    genreListHandle = GNSDK_NULL;
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
    if (userHandle != GNSDK_NULL)
    {
        gnsdk_manager_user_release(userHandle);
        userHandle = GNSDK_NULL;
    }

    if (genreListHandle != GNSDK_NULL)
    {
        gnsdk_manager_list_release(genreListHandle);
        genreListHandle = GNSDK_NULL;
    }
    gnsdk_manager_shutdown();
}

/* ---------------------------------------------------------
 *                      PUBLIC API
 * --------------------------------------------------------- */

bool GracenotePlugin::update(EMSTrack *track)
{
    bool ret;

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

    QString discid;
    if (track->type == TRACK_TYPE_CDROM)
    {
        EMSCdrom cdrom;
        if (CdromManager::instance()->getCdrom(track->filename, &cdrom))
        {
            discid = cdrom.disc_id;
        }
    }
    else /* Lookup for a file named "discid" near to the track file */
    {
        QString dirPath = QFileInfo(track->filename).dir().path();
        QFile discidFile(dirPath + QDir::separator() + "discid");
        if (discidFile.exists())
        {
            if(discidFile.open(QIODevice::ReadOnly))
            {
                discid = discidFile.readLine();
                discidFile.close();
            }
        }
        if (!discid.isEmpty() && track->position == 0)
        {
            /* Having the disc ID is not enough to retrieve track specific data
             * like the track title, etc.
             * So we need to know the position of this track in the CDROM
             * When a CDROM is ripped, the track name is formated like "track[id].[extension]".
             * Parse
             */
            QString name = QFileInfo(track->filename).baseName();
            if (name.startsWith("track"))
            {
                name.remove("track");
                bool isNumber;
                unsigned int pos = name.toUInt(&isNumber);
                if (isNumber)
                {
                    track->position = pos;
                }
            }
        }
    }

    /* Lookup using the disc ID */
    if (!discid.isEmpty())
    {
        /* The disc ID is formated like : ID TRACKNUM TOC DURATION */
        QString toc;
        QStringList parts = discid.split(' ');
        for (int i=2; i<(parts.size()-1); i++)
        {
            if (!toc.isEmpty())
            {
                toc += " ";
            }
            toc += parts.at(i);
        }

        qDebug() << "Gracenote: lookup using disc TOC : " << toc;
        ret = lookupByDiscID(track, toc);

        /* If discID returned data, assume we have complete data */
        if (ret && !track->album.name.isEmpty() && !track->name.isEmpty())
        {
            return true;
        }
    }

    /* Lookup using other methods like "text" search
     * For album lookup, assume there at least two of these data :
     * - album name
     * - track name
     * - artist name
     * The track data will be filled if it can be positioned in the album
     * To locate a track inside an album, we need one of these data :
     * - track name
     * - track postion
     */
    if (track->type == TRACK_TYPE_CDROM && track->position > 1) /* Try to use previously found album name */
    {
        EMSCdrom cdrom;
        if (CdromManager::instance()->getCdrom(track->filename, &cdrom))
        {
            if (cdrom.tracks.size() > 0)
            {
                track->album = cdrom.tracks.first().album;
                track->artists = cdrom.tracks.first().artists;
                track->genres = cdrom.tracks.first().genres;
            }

        }
    }
    int dataFilled = 0;
    if(!track->album.name.isEmpty())
    {
        dataFilled++;
    }
    if(!track->name.isEmpty())
    {
        dataFilled++;
    }
    foreach(EMSArtist artist, track->artists)
    {
        if(!artist.name.isEmpty())
        {
            dataFilled++;
            break;
        }
    }
    if (dataFilled >= 2)
    {
        qDebug() << "Gracenote: lookup using texts from other metadata plugins for file " << track->filename;
        ret = lookupByText(track);

        /* If lookup returned data, assume we have complete data */
        if (ret && !track->album.name.isEmpty() && !track->name.isEmpty())
        {
            return true;
        }
    }

    /* If no data found, try to use "fingerprint" */
    if(track->album.name.isEmpty())
    {
        /* Fingerprint lookup is for now only available for WAV files */
        if ((track->format == "wav") || (track->format == "raw"))
        {
            qDebug() << "Gracenote: lookup using fingerprint of file " << track->filename;
            ret = lookupByFingerprint(track);
        }
    }

    return ret;
}

/* ---------------------------------------------------------
 *                    LOOKUP FUNCTIONS
 * --------------------------------------------------------- */

bool GracenotePlugin::lookupAlbumByQueryHandle(EMSTrack *track, gnsdk_musicid_query_handle_t queryHandle)
{
    gnsdk_error_t error = GNSDK_SUCCESS;
    gnsdk_gdo_handle_t findResult = GNSDK_NULL;
    gnsdk_gdo_handle_t albumGdo = GNSDK_NULL;
    gnsdk_uint32_t count = 0;
    gnsdk_cstr_t full = GNSDK_NULL;

    bool checkChildTrackNumber = false;
    gnsdk_uint32_t trackNumber = 0;
    if (track->type == TRACK_TYPE_CDROM)
    {
        /* For CDROM album, ensure we retrieve a CDROM which have the same number of track */
        checkChildTrackNumber = true;
        EMSCdrom cdrom;
        CdromManager::instance()->getCdrom(track->filename, &cdrom);
        trackNumber = cdrom.tracks.size();
    }

    /* Set option for the query */
    gnsdk_musicid_query_option_set(queryHandle, GNSDK_MUSICID_OPTION_RESULT_PREFER_COVERART, GNSDK_VALUE_TRUE);
    if (!checkChildTrackNumber)
    {
        gnsdk_musicid_query_option_set(queryHandle, GNSDK_MUSICID_OPTION_RESULT_SINGLE, GNSDK_VALUE_TRUE);
    }
    else
    {
        gnsdk_musicid_query_option_set(queryHandle, GNSDK_MUSICID_OPTION_RESULT_SINGLE, GNSDK_VALUE_FALSE);
        gnsdk_musicid_query_option_set(queryHandle, GNSDK_MUSICID_OPTION_RESULT_RANGE_SIZE, "100");
    }

    /* Perform the query */
    error = gnsdk_musicid_query_find_albums(queryHandle, &findResult);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        return false;
    }

    /* Get how many album match */
    error = gnsdk_manager_gdo_child_count(findResult, GNSDK_GDO_CHILD_ALBUM, &count);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_manager_gdo_release(findResult);
        return false;
    }

    if (count == 0)
    {
        qDebug() << "No disc info for this album.";
        gnsdk_manager_gdo_release(findResult);
        return false;
    }

    if (!checkChildTrackNumber)
    {
        /* Choose the first album */
        error = gnsdk_manager_gdo_child_get(findResult, GNSDK_GDO_CHILD_ALBUM, 1, &albumGdo);
        if (error != GNSDK_SUCCESS)
        {
            displayLastError();
            gnsdk_manager_gdo_release(findResult);
            return false;
        }
    }
    else
    {
        /* Get the first album with the same track number */
        unsigned int end = 0;
        if (count > 1)
        {
            end = count;
        }
        else if (count == 1)
        {
            end = 1;
        }
        bool found = false;
        for (unsigned int i = 1; i <= end; i++)
        {
            error = gnsdk_manager_gdo_child_get(findResult, GNSDK_GDO_CHILD_ALBUM, i, &albumGdo);
            if (error != GNSDK_SUCCESS)
            {
                displayLastError();
                gnsdk_manager_gdo_release(findResult);
                return false;
            }
            gnsdk_uint32_t currentTrackNumber = 0;
            error = gnsdk_manager_gdo_child_count(albumGdo, GNSDK_GDO_CHILD_TRACK, &currentTrackNumber);
            if (error != GNSDK_SUCCESS)
            {
                displayLastError();
                gnsdk_manager_gdo_release(findResult);
                return false;
            }
            if(currentTrackNumber == trackNumber)
            {
                /* Found ! */
                found = true;
                break;
            }
            else
            {
                gnsdk_manager_gdo_release(albumGdo);
                albumGdo = GNSDK_NULL;
            }
        }
        if (!found)
        {
            qDebug() << "No valid album found.";
            gnsdk_manager_gdo_release(findResult);
            return false;
        }
    }

    error = gnsdk_manager_gdo_value_get(albumGdo, GNSDK_GDO_VALUE_FULL_RESULT, 1, &full);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_manager_gdo_release(findResult);
        gnsdk_manager_gdo_release(albumGdo);
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
            return false;
        }

        /* New search */
        gnsdk_manager_gdo_release(findResult);
        error = gnsdk_musicid_query_find_albums(queryHandle, &findResult);
        if (error != GNSDK_SUCCESS)
        {
            displayLastError();
            return false;
        }

        /* Get new album with more data */
        gnsdk_manager_gdo_release(albumGdo);
        error = gnsdk_manager_gdo_child_get(findResult, GNSDK_GDO_CHILD_ALBUM, 1, &albumGdo);
        if (error != GNSDK_SUCCESS)
        {
            displayLastError();
            gnsdk_manager_gdo_release(findResult);
            return false;
        }
    }

    /* We have found the album, update the given track object */
    albumGdoToEMSTrack(albumGdo, track);

    gnsdk_manager_gdo_release(albumGdo);
    gnsdk_manager_gdo_release(findResult);
    return true;
}

bool GracenotePlugin::lookupTrackByQueryHandle(EMSTrack *track, gnsdk_musicid_query_handle_t queryHandle)
{
    gnsdk_error_t error = GNSDK_SUCCESS;
    gnsdk_gdo_handle_t findResult = GNSDK_NULL;
    gnsdk_gdo_handle_t trackGdo = GNSDK_NULL;
    gnsdk_uint32_t count = 0;

    /* Perform the query */
    error = gnsdk_musicid_query_find_tracks(queryHandle, &findResult);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        return false;
    }

    /* Get how many album match */
    error = gnsdk_manager_gdo_child_count(findResult, GNSDK_GDO_CHILD_TRACK, &count);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_manager_gdo_release(findResult);
        return false;
    }

    if (count == 0)
    {
        qDebug() << "No track found for track " << track->filename;
        gnsdk_manager_gdo_release(findResult);
        return false;
    }

    /* Choose the first track */
    error = gnsdk_manager_gdo_child_get(findResult, GNSDK_GDO_CHILD_TRACK, 1, &trackGdo);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_manager_gdo_release(findResult);
        return false;
    }

    trackGdoToEMSTrack(trackGdo, track);
    gnsdk_manager_gdo_release(trackGdo);
    gnsdk_manager_gdo_release(findResult);
    return true;
}

bool GracenotePlugin::lookupByText(EMSTrack *track)
{
    gnsdk_error_t error = GNSDK_SUCCESS;
    gnsdk_musicid_query_handle_t queryHandle = GNSDK_NULL;
    gnsdk_gdo_handle_t findResult = GNSDK_NULL;
    gnsdk_gdo_handle_t albumGdo = GNSDK_NULL;
    gnsdk_uint32_t count = 0;
    gnsdk_cstr_t full = GNSDK_NULL;

    /* Create the query handle */
    error = gnsdk_musicid_query_create(userHandle, GNSDK_NULL, GNSDK_NULL, &queryHandle);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        return false;
    }

    /* Set all data we have about the track */
    if(!track->album.name.isEmpty())
    {
        gnsdk_musicid_query_set_text(queryHandle, GNSDK_MUSICID_FIELD_ALBUM, track->album.name.toUtf8().data());
    }
    if(!track->name.isEmpty())
    {
        gnsdk_musicid_query_set_text(queryHandle, GNSDK_MUSICID_FIELD_TITLE, track->name.toUtf8().data());
    }
    foreach(EMSArtist artist, track->artists)
    {
        if(!artist.name.isEmpty())
        {
            gnsdk_musicid_query_set_text(queryHandle, GNSDK_MUSICID_FIELD_TRACK_ARTIST, artist.name.toUtf8().data());
        }
    }

    /* Send the request */
    error = gnsdk_musicid_query_find_matches(queryHandle, &findResult);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_musicid_query_release(queryHandle);
        return false;
    }

    gnsdk_manager_gdo_child_count(findResult, GNSDK_GDO_CHILD_MATCH, &count);
    if (count == 0)
    {
        /* No match found... :'( */
        gnsdk_musicid_query_release(queryHandle);
        return false;
    }

    /* Choose the first match */
    error = gnsdk_manager_gdo_child_get(findResult, GNSDK_GDO_CHILD_ALBUM, 1, &albumGdo);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_manager_gdo_release(findResult);
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

    /* If partial result, get full result */
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

bool GracenotePlugin::lookupByDiscID(EMSTrack *track, QString toc)
{
    gnsdk_error_t error = GNSDK_SUCCESS;
    gnsdk_musicid_query_handle_t queryHandle = GNSDK_NULL;

    /* Create the query handle */
    error = gnsdk_musicid_query_create(userHandle, GNSDK_NULL, GNSDK_NULL, &queryHandle);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        return false;
    }

    /* Set TOC */
    error = gnsdk_musicid_query_set_toc_string(queryHandle, toc.toStdString().c_str());
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        gnsdk_musicid_query_release(queryHandle);
        return false;
    }

    bool ret = lookupAlbumByQueryHandle(track, queryHandle);
    gnsdk_musicid_query_release(queryHandle);
    return ret;
}

bool GracenotePlugin::lookupByFingerprint(EMSTrack *track)
{
    gnsdk_musicid_query_handle_t queryHandle = GNSDK_NULL;
    gnsdk_error_t error = GNSDK_SUCCESS;
    unsigned int bps = 0, channels = 0, sampleRate = 0;

    /* First get stream data :
     * - Bits number per sample
     * - Sample rate
     * - Number of channels
     */
    if (track->type == TRACK_TYPE_CDROM)
    {
        sampleRate = 44100;
        bps = 16;
        channels = 2;
    }
    else
    {
        QStringList params = track->format_parameters.split(";");
        foreach (QString param, params)
        {
            if (param.startsWith("bits_per_sample:"))
            {
                param.remove("bits_per_sample:");
                bps = param.toUInt();
            }
            if (param.startsWith("channels:"))
            {
                param.remove("channels:");
                channels = param.toUInt();
            }
        }
        sampleRate = track->sample_rate;
    }

    if (bps == 0 || channels == 0 || sampleRate == 0)
    {
        /* Not enough data to read, return normally */
        qDebug() << "Can't find bps/channels/sample_rate for track " << track->filename;
        qDebug() << QString("bps = %1, channels = %2, sample_rate = %3").arg(bps).arg(channels).arg(sampleRate);
        return true;
    }

    /* Create query handle */
    error = gnsdk_musicid_query_create(userHandle, GNSDK_NULL, GNSDK_NULL, &queryHandle);
    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        return false;
    }

    /* Then read music samples until GNSDK have enough data to find a result
     */
    if (track->type == TRACK_TYPE_CDROM)
    {
        CdIo_t *cdrom = cdio_open(track->filename.toStdString().c_str(), DRIVER_UNKNOWN);
        if (!cdrom)
        {
            qCritical() << "Unable to open device " << track->filename;
            return false;
        }
        EMSCdrom emsCdrom;
        if(!CdromManager::instance()->getCdrom(track->filename, &emsCdrom))
        {
            qCritical() << "Unable to find cdrom " << track->filename;
            return false;
        }
        QPair<lsn_t, lsn_t> sectors = emsCdrom.trackSectors.value(track->position, QPair<lsn_t, lsn_t>(0,0));
        lsn_t begin = sectors.first;
        lsn_t end = sectors.second;
        gnsdk_bool_t complete = GNSDK_FALSE;
        char samples[CDIO_CD_FRAMESIZE_RAW];

        gnsdk_musicid_query_fingerprint_begin(queryHandle, GNSDK_MUSICID_FP_DATA_TYPE_STREAM6, sampleRate, bps, channels);
        lsn_t lsn=begin;
        while(lsn<end)
        {
            driver_return_code_t readCdioErr = cdio_read_audio_sector(cdrom, samples, lsn);
            if (readCdioErr != DRIVER_OP_SUCCESS)
            {
                break;
            }

            error = gnsdk_musicid_query_fingerprint_write(queryHandle, samples, CDIO_CD_FRAMESIZE_RAW, &complete);
            if (error != GNSDK_SUCCESS || complete == GNSDK_TRUE)
            {
                break;
            }
            lsn++;
        }
        gnsdk_musicid_query_fingerprint_end(queryHandle);
        cdio_destroy(cdrom);
    }
    else
    {
        QFile file(track->filename);
        if (!file.open(QIODevice::ReadOnly))
        {
            return false;
        }

        if (track->format == "wav") // Skip the header
        {
            file.seek(44);
        }

        /* Feed gracenote engine with samples */
        bool cont = true;
        char samples[2048];
        int samplesSize = 0;
        gnsdk_bool_t complete = GNSDK_FALSE;

        gnsdk_musicid_query_fingerprint_begin(queryHandle, GNSDK_MUSICID_FP_DATA_TYPE_FILE, sampleRate, bps, channels);
        while(cont)
        {
            samplesSize = file.read(samples, 2048);
            if (samplesSize != 2048 || file.atEnd())
            {
                cont = false;
            }
            error = gnsdk_musicid_query_fingerprint_write(queryHandle, samples, samplesSize, &complete);
            if (error != GNSDK_SUCCESS || complete == GNSDK_TRUE)
            {
                cont = false;
            }
        }
        gnsdk_musicid_query_fingerprint_end(queryHandle);
        file.close();
    }

    /* Lookup using the query handle */
    bool ret;
    ret = lookupTrackByQueryHandle(track, queryHandle);

    if (!ret)
    {
        gnsdk_musicid_query_release(queryHandle);
        return ret;
    }

    ret = lookupAlbumByQueryHandle(track, queryHandle);
    gnsdk_musicid_query_release(queryHandle);

    return ret;
}

/* ---------------------------------------------------------
 *            UTILITIES FOR PARSING GRACENOTE DATA
 * --------------------------------------------------------- */

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
        QStringList albumNames;
        albumNames = getGdoValue(titleGdo, GNSDK_GDO_VALUE_DISPLAY);
        if (albumNames.size() > 0)
        {
            track->album.name = albumNames.first();
        }
        else
        {
            /* Use the default album */
            Database::instance()->getAlbumById(&(track->album), 0);
        }
        gnsdk_manager_gdo_release(titleGdo);
    }

    /* Album cover */
    /* Look for an already downloaded cover */
    QString coverPath;
    if (lookForDownloadedImage(albumsCacheDir, gnid, &coverPath))
    {
        track->album.cover = coverPath;
    }
    else /* Otherwise get the largest available pictures*/
    {
        error = gnsdk_link_query_create(userHandle, GNSDK_NULL, GNSDK_NULL, &queryHandle);
        if (error != GNSDK_SUCCESS)
        {
            displayLastError();
            return false; /* Should not happen */
        }
        gnsdk_link_query_set_gdo(queryHandle, albumGdo);
        downloadImage(queryHandle, gnsdk_link_content_cover_art, albumsCacheDir, gnid,&coverPath);
        track->album.cover = coverPath;
        gnsdk_link_query_release(queryHandle);
    }

    /* Retrieve the track inside the AlbumGdo */
    gnsdk_gdo_handle_t trackGdo = GNSDK_NULL;
    if (track->position != 0)
    {
        error = gnsdk_manager_gdo_child_get(albumGdo, GNSDK_GDO_CHILD_TRACK_BY_NUMBER, track->position, &trackGdo);
    }
    else
    {
        error = gnsdk_manager_gdo_child_get(albumGdo, GNSDK_GDO_CHILD_TRACK_MATCHED, 1, &trackGdo);
    }

    if (error == GNSDK_SUCCESS)
    {
        trackGdoToEMSTrack(trackGdo, track);
        gnsdk_manager_gdo_release(trackGdo);
    }

    /* If genre or artist are missing in the track GDO, use the ones linked to the album
     * Gracenote metadata most often have genre for the whole album instead of for each track
     */
    gdoToEMSGenres(albumGdo, &(track->genres));
    gdoToEMSArtists(albumGdo, &(track->artists));

    return true;
}

bool GracenotePlugin::trackGdoToEMSTrack(gnsdk_gdo_handle_t trackGdo, EMSTrack *track)
{
    gnsdk_error_t error = GNSDK_SUCCESS;

    /* Track title */
    gnsdk_gdo_handle_t titleGdo = GNSDK_NULL;
    error = gnsdk_manager_gdo_child_get(trackGdo, GNSDK_GDO_CHILD_TITLE_OFFICIAL, 1, &titleGdo);
    if (error == GNSDK_SUCCESS)
    {
        QStringList trackNames;
        trackNames = getGdoValue(titleGdo, GNSDK_GDO_VALUE_DISPLAY);
        if (trackNames.size() > 0)
        {
            track->name = trackNames.first();
        }
        gnsdk_manager_gdo_release(titleGdo);
    }

    /* Track position */
    if (track->position == 0)
    {
        QStringList values = getGdoValue(trackGdo, GNSDK_GDO_VALUE_TRACK_NUMBER);
        if (values.size() > 0)
        {
            track->position = values.first().toUInt();
        }
    }

    /* Track genres */
    gdoToEMSGenres(trackGdo, &(track->genres));

    /* Track artists */
    gdoToEMSArtists(trackGdo, &(track->artists));

    return true;
}

void GracenotePlugin::gdoToEMSArtists(gnsdk_gdo_handle_t gdo, QVector<EMSArtist> *artists)
{
    gnsdk_error_t error = GNSDK_SUCCESS;
    gnsdk_link_query_handle_t queryHandle = GNSDK_NULL;

    /* Get artist GDO */
    gnsdk_gdo_handle_t artistGdo = GNSDK_NULL;
    error = gnsdk_manager_gdo_child_get(gdo, GNSDK_GDO_CHILD_ARTIST, 1, &artistGdo);
    if (error != GNSDK_SUCCESS)
    {
        return;
    }

    /* Get all contributors */
    gnsdk_uint32_t count = 0;
    error = gnsdk_manager_gdo_value_count(artistGdo, GNSDK_GDO_CHILD_CONTRIBUTOR, &count);
    if ((error != GNSDK_SUCCESS) || (count == 0))
    {
        return;
    }
    unsigned int end = 0;
    if (count > 1)
    {
        end = count;
    }
    else if (count == 1)
    {
        end = 1;
    }

    /* Fixme: for now, if Gracenote give us some data, we re-create the complete list
     * to avoid from duplicate entries. The string comparison can be somtimes wrong.
     * For example, Gracenote can give artist "Julien doré" and the metadata "Julien dore".
     */
    artists->clear();

    for (unsigned int i = 1; i <= end; i++)
    {
        gnsdk_gdo_handle_t contributorGdo = GNSDK_NULL;
        error = gnsdk_manager_gdo_child_get(artistGdo, GNSDK_GDO_CHILD_CONTRIBUTOR, i, &contributorGdo);
        if (error == GNSDK_SUCCESS)
        {
            gnsdk_gdo_handle_t contributorNameGdo = GNSDK_NULL;
            error = gnsdk_manager_gdo_child_get(contributorGdo, GNSDK_GDO_CHILD_NAME_OFFICIAL, 1, &contributorNameGdo);
            if (error == GNSDK_SUCCESS)
            {
                QStringList artistNames = getGdoValue(contributorNameGdo, GNSDK_GDO_VALUE_DISPLAY);
                if (artistNames.size() > 0)
                {
                    EMSArtist artist;
                    artist.name = artistNames.first();

                    /* Get artist image */
                    QString imagePath;
                    QString basename(QByteArray().append(artist.name).toHex());
                    if (lookForDownloadedImage(artistsCacheDir, basename, &imagePath))
                    {
                        artist.picture = imagePath;
                    }
                    else /* Otherwise get the largest available image*/
                    {
                        error = gnsdk_link_query_create(userHandle, GNSDK_NULL, GNSDK_NULL, &queryHandle);
                        if (error != GNSDK_SUCCESS)
                        {
                            displayLastError();
                            return; /* Should not happen */
                        }

                        /* Try to get image from the contributor GDO,
                         * if it fails, use the album/track GDO to get artist picture */
                        gnsdk_link_query_set_gdo(queryHandle, contributorGdo);
                        if(downloadImage(queryHandle, gnsdk_link_content_image_artist, artistsCacheDir, basename ,&imagePath))
                        {
                            artist.picture = imagePath;
                        }
                        else
                        {
                            gnsdk_link_query_release(queryHandle);
                            gnsdk_link_query_create(userHandle, GNSDK_NULL, GNSDK_NULL, &queryHandle);
                            gnsdk_link_query_set_gdo(queryHandle, gdo);
                            if(downloadImage(queryHandle, gnsdk_link_content_image_artist, artistsCacheDir, basename ,&imagePath))
                            {
                                artist.picture = imagePath;
                            }
                        }
                        gnsdk_link_query_release(queryHandle);
                    }

                    /* Add the new artist in the list
                     * Or replace the existing one with the same name
                     */
                    bool found = false;
                    for(int i=0; i<artists->size(); i++)
                    {
                        EMSArtist existingArtist = artists->at(i);
                        if (existingArtist.name == artist.name)
                        {
                            if (existingArtist.picture.isEmpty())
                            {
                                artists->replace(i, artist);
                            }
                            found = true;
                            break;
                        }

                    }
                    if (!found)
                    {
                        artists->append(artist);
                    }



                }
                gnsdk_manager_gdo_release(contributorNameGdo);
            }
            gnsdk_manager_gdo_release(contributorGdo);
        }
    }
    gnsdk_manager_gdo_release(artistGdo);
}

void GracenotePlugin::gdoToEMSGenres(gnsdk_gdo_handle_t gdo, QVector<EMSGenre> *genres)
{
    gnsdk_error_t error = GNSDK_SUCCESS;

    /* First get the names of all genres related to the GDO */
    QStringList genresStr;
    genresStr << getGdoValue(gdo, GNSDK_GDO_VALUE_GENRE_LEVEL1);
    genresStr << getGdoValue(gdo, GNSDK_GDO_VALUE_GENRE_LEVEL2);
    genresStr << getGdoValue(gdo, GNSDK_GDO_VALUE_GENRE_LEVEL3);

    /* Fixme: for now, if Gracenote give us some data, we re-create the complete list
     * to avoid from duplicate entries. The string comparison can be somtimes wrong.
     */
    if (genresStr.size() > 0)
    {
        genres->clear();
    }

    foreach (QString genreStr, genresStr)
    {
        /* Create a new genre */
        EMSGenre genre;
        genre.name = genreStr;

        /* Retrieve the genre art */
        QString imagePath;
        QString basename(QByteArray().append(genreStr).toHex());
        if (lookForDownloadedImage(genresCacheDir, basename, &imagePath))
        {
            genre.picture = imagePath;
        }
        else
        {
            gnsdk_link_query_handle_t queryHandle = GNSDK_NULL;
            gnsdk_list_element_handle_t elementHandle = GNSDK_NULL;

            error = gnsdk_link_query_create(userHandle, GNSDK_NULL, GNSDK_NULL, &queryHandle);
            if (error != GNSDK_SUCCESS)
            {
                displayLastError();
                continue;
            }
            error = gnsdk_manager_list_get_element_by_string(genreListHandle, genreStr.toStdString().c_str(), &elementHandle);
            if (error != GNSDK_SUCCESS)
            {
                displayLastError();
                continue;
            }
            gnsdk_link_query_set_list_element(queryHandle, elementHandle);
            if (downloadImage(queryHandle, gnsdk_link_content_genre_art, genresCacheDir, basename, &imagePath))
            {
                genre.picture = imagePath;
            }

            gnsdk_manager_list_element_release(elementHandle);
            gnsdk_link_query_release(queryHandle);
        }

        /* Add the new genre in the list
         * Or replace the existing one with the same name
         */
        bool found = false;
        for(int i=0; i<genres->size(); i++)
        {
            EMSGenre existingGenre = genres->at(i);
            if (existingGenre.name == genre.name)
            {
                if (existingGenre.picture.isEmpty())
                {
                    genres->replace(i, genre);
                }
                found = true;
                break;
            }

        }
        if (!found)
        {
            genres->append(genre);
        }

    }
}

QStringList GracenotePlugin::getGdoValue(gnsdk_gdo_handle_t gdo, gnsdk_cstr_t key)
{
    gnsdk_error_t error = GNSDK_SUCCESS;
    gnsdk_cstr_t value = GNSDK_NULL;
    gnsdk_uint32_t count = 0;
    QStringList out;

    /* Get the number of element for this key */
    error = gnsdk_manager_gdo_value_count(gdo, key, &count);

    if (error != GNSDK_SUCCESS)
    {
        displayLastError();
        return out;
    }
    else if (count > 1)
    {
        for (unsigned int i = 1; i <= count; i++)
        {
            error = gnsdk_manager_gdo_value_get(gdo, key, i, &value);
            if (error == GNSDK_SUCCESS)
            {
                out << value;
            }
        }
    }
    else if (count > 0)
    {
        error = gnsdk_manager_gdo_value_get(gdo, key, 1, &value);
        if (error == GNSDK_SUCCESS)
        {
            out << value;
        }
    }

    return out;
}

/* ---------------------------------------------------------
 *             IMAGES MANAGEMENT (DOWNLOAD/CACHE)
 * --------------------------------------------------------- */

bool GracenotePlugin::lookForDownloadedImage(QString dir, QString basename, QString *fullpath)
{
    /* Look for an already downloaded picture */
    foreach(QString extension, imageFormats)
    {
        if (QFile(dir + QDir::separator() + basename + extension).exists())
        {
            *fullpath = dir + QDir::separator() + basename + extension;
            return true;
        }
    }
    return false;
}

bool GracenotePlugin::downloadImage(gnsdk_link_query_handle_t queryHandle,
                                    gnsdk_link_content_type_t type,
                                    QString dir,
                                    QString basename,
                                    QString *computedPath)
{
    gnsdk_error_t error = GNSDK_SUCCESS;
    gnsdk_link_data_type_t dataType = gnsdk_link_data_unknown;
    gnsdk_byte_t* buffer = GNSDK_NULL;
    gnsdk_size_t bufferSize = 0;

    /* Get biggest pictures */
    gnsdk_link_query_option_set(queryHandle, GNSDK_LINK_OPTION_KEY_IMAGE_SIZE, GNSDK_LINK_OPTION_VALUE_IMAGE_SIZE_1080);

    error = gnsdk_link_query_content_retrieve(queryHandle, type, 1, &dataType, &buffer, &bufferSize);
    /* An image has been found, save it in the cache */
    if (error == GNSDK_SUCCESS)
    {
        QString extension;
        if (dataType == gnsdk_link_data_image_jpeg)
        {
            extension = ".jpeg";
        }
        else if (dataType == gnsdk_link_data_image_png)
        {
            extension = ".png";
        }
        if (!extension.isEmpty())
        {
            QFile image(dir + QDir::separator() + basename + extension);
            if (!image.exists() && image.open(QIODevice::WriteOnly))
            {
                image.write((const char*)buffer, bufferSize);
                image.close();
                *computedPath = image.fileName();
                qDebug() << "Downloaded image : " << image.fileName();

                gnsdk_link_query_content_free(buffer);
                return true;
            }
        }
        gnsdk_link_query_content_free(buffer);
    }

    return false;
}

/* ---------------------------------------------------------
 *                 CONNECTION MANAGEMENT
 * --------------------------------------------------------- */

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

    /* Initialize the DSP Library */
    error = gnsdk_dsp_initialize(sdkmgrHandle);
    if (error != GNSDK_SUCCESS)
    {
        return false;
    }

    /* Get a user handle for our client ID.  This will be passed in for all queries */
    if (!getUserHandle())
    {
        gnsdk_manager_user_release(userHandle);
        userHandle = GNSDK_NULL;
        gnsdk_manager_shutdown();
        return false;
    }

    /* Set locale (english by default) */
    gnsdk_locale_handle_t localeHandle = GNSDK_NULL;
    error = gnsdk_manager_locale_load(GNSDK_LOCALE_GROUP_MUSIC, GNSDK_LANG_ENGLISH, GNSDK_REGION_DEFAULT,
                                      GNSDK_DESCRIPTOR_SIMPLIFIED, userHandle, GNSDK_NULL, 0, &localeHandle);
    if (error != GNSDK_SUCCESS)
    {
        return false;
    }
    error = gnsdk_manager_locale_set_group_default(localeHandle);
    if (error != GNSDK_SUCCESS)
    {
        return false;
    }
    gnsdk_manager_locale_release(localeHandle);

    /* Get genre list handle */
    error = gnsdk_manager_list_retrieve(GNSDK_LIST_TYPE_GENRES, GNSDK_LANG_ENGLISH, GNSDK_REGION_DEFAULT,
                                        GNSDK_DESCRIPTOR_SIMPLIFIED, userHandle, GNSDK_NULL, GNSDK_NULL, &genreListHandle);
    if (error != GNSDK_SUCCESS)
    {
        return false;
    }

    return true;
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

void GracenotePlugin::displayLastError()
{
    const gnsdk_error_info_t* error_info = gnsdk_manager_error_info();
    qCritical() << "Gracenote error : "
                << QString().sprintf("(%s) : %08x %s", error_info->error_api, error_info->error_code, error_info->error_description);
}
