#include "CdromRipper.h"

#include <cdio/cd_types.h>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include "DefaultSettings.h"


CdromRipper::CdromRipper(QObject *parent)
    : QThread(parent)
{
    qRegisterMetaType<EMSRipProgress>("EMSRipProgress");

    m_ripProgressObserverTimer = new QTimer(this);
    connect(m_ripProgressObserverTimer, &QTimer::timeout, this, &CdromRipper::observeRipProgress);
    m_ripProgressObserverTimer->setInterval(1000); // in milliseconds
    m_ripProgressObserverTimer->start();
}

CdromRipper::~CdromRipper()
{
    m_ripProgressObserverTimer->stop();
    delete m_ripProgressObserverTimer;
}

void CdromRipper::setCdrom(const EMSCdrom &cdromProperties)
{
    m_cdromProperties = cdromProperties;
}

void CdromRipper::setAudioFormat(const QString &audioFormat)
{
    m_audioFormat = audioFormat;
}

/* ---------------------------------------------------------
 *                  THREAD MAIN FUNCTION
 * --------------------------------------------------------- */
/* Main thread function
 * This function is called once and must handle every blocking action.
 * All shared class members (= the one used by the API) MUST be protected by the global mutex.
 */
void CdromRipper::run()
{
    QString result("Rip: OK");

    if (!identifyDrive())
    {
        qCritical() << "CdromRipper: rip aborted";
        return;
    }
    qDebug() << "CdromRipper: device name: " << m_drive->cdda_device_name;

    if (m_cdromProperties.device != QString(m_drive->cdda_device_name))
    {
        qCritical() << "CdromRipper: the name of the physical drive does not match the saved device name";
        qCritical() << "CdromRipper: rip aborted";
        return;
    }

    if (!openDrive())
    {
        qCritical() << "CdromRipper: rip aborted";
        return;
    }

    initializeParanoia();
    computeDiskSectorQuantity();


    unsigned int nbTracks = m_cdromProperties.tracks.size();
    m_emsRipProgressMutex.lock();
    m_emsRipProgress.track_total = nbTracks;
    m_emsRipProgressMutex.unlock();

    for (unsigned int indexTrack = 0; indexTrack < nbTracks; ++indexTrack)
    {
        ripOneTrack(indexTrack);
    }

    closeDrive();

    qDebug() << "CdromRipper: end of the rip process";
    emit resultReady(result);
}

bool CdromRipper::identifyDrive()
{
    char **ppsz_cd_drives;  /* List of all drives with a loaded CDDA in it. */
    driver_id_t driver_id;

    /* See if we can find a device with a loaded CD-DA in it. If successful
       drive_id will be set.  */
    ppsz_cd_drives = cdio_get_devices_with_cap_ret(NULL, CDIO_FS_AUDIO,
                                                   false, &driver_id);

    if (ppsz_cd_drives && *ppsz_cd_drives)
    {
        /* Use the first drive in the list. */
        m_drive = cdda_identify(*ppsz_cd_drives, 1, NULL);
    }
    else
    {
        qCritical()<< "CdromRipper: unable find or access a CD-ROM drive with an audio CD in it";
        return false;
    }

    /* Don't need a list of CD's with CD-DA's any more. */
    cdio_free_device_list(ppsz_cd_drives);

    return true;
}

bool CdromRipper::openDrive()
{
    /* Set for verbose paranoia messages. */
    cdda_verbose_set(m_drive, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);

    if ( 0 != cdio_cddap_open(m_drive) ) {
        qCritical() << "CdromRipper: unable to open disc.";
        return false;
    }
    return true;
}

void CdromRipper::initializeParanoia()
{
    m_paranoiaStruc = paranoia_init(m_drive);

    /* Set reading mode for full paranoia, but allow skipping sectors. */
    paranoia_modeset(m_paranoiaStruc, PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP);
}

void CdromRipper::closeDrive()
{
    paranoia_free(m_paranoiaStruc);
    cdio_cddap_close(m_drive);
}

bool CdromRipper::ripOneTrack(unsigned int indexTrack)
{
    unsigned int trackPosition = m_cdromProperties.tracks[indexTrack].position;
    lsn_t lsnBegin = m_cdromProperties.trackSectors[trackPosition].first;
    lsn_t lsnEnd = m_cdromProperties.trackSectors[trackPosition].second;
    lsn_t currentLsn = lsnBegin;
    lsn_t nbSectors = lsnEnd - lsnBegin + 1;
    unsigned int bufferSize = CDIO_CD_FRAMESIZE_RAW * nbSectors;
    uint8_t *audioTrackBuf = (uint8_t*)malloc(bufferSize);
    uint8_t *writeIndex = audioTrackBuf;
    memset(audioTrackBuf, 0, bufferSize);

    qDebug() << "CdromRipper: track begin: " << lsnBegin;
    qDebug() << "CdromRipper: track end  : " << lsnEnd;
    qDebug() << "CdromRipper: nbsectors  : " << nbSectors;
    qDebug() << "CdromRipper: size (in octet): " << bufferSize;

    paranoia_seek(m_paranoiaStruc, lsnBegin, SEEK_SET);

    while (currentLsn <= lsnEnd)
    {
        // read a sector
        int16_t *p_readbuf = paranoia_read(m_paranoiaStruc, Q_NULLPTR);
        char *psz_err=cdio_cddap_errors(m_drive);
        char *psz_mes=cdio_cddap_messages(m_drive);

        if (psz_mes)
            qWarning() << "CdromRipper: Paranoia mess: " << psz_mes;
        if (psz_err)
            qCritical() << "CdromRipper: Paranoia err: " << psz_err;

        if (psz_err != NULL)
        {
            free(psz_err);
        }
        if (psz_mes != NULL)
        {
            free(psz_mes);
        }

        if (!p_readbuf)
        {
            qCritical() << "CdromRipper: paranoia read error. Track rip aborted.";
            free(audioTrackBuf);
            return false;
        }

        memcpy(writeIndex, p_readbuf, CDIO_CD_FRAMESIZE_RAW);
        writeIndex += CDIO_CD_FRAMESIZE_RAW;

        buildRipProgressMessage(indexTrack, currentLsn,
                                      lsnBegin, lsnEnd);

        currentLsn++;
    }

    if(buildAudioFilenames(indexTrack))
    {
        // WAV encoding
        if (!m_wavEncoder.write(m_wavFilenameCurrentTrack, audioTrackBuf, bufferSize))
        {
                qCritical() << "CdromRipper: Track rip aborted (WAV audio encoding failed)";
                free(audioTrackBuf);
                return false;
        }
        else
        {
            if (m_audioFormat != "WAV")
            {
                // FLAC encoding
                m_flacEncoder.setInputFilename(m_wavFilenameCurrentTrack);
                m_flacEncoder.setOutputFilename(m_flacFilenameCurrentTrack);
                if (!m_flacEncoder.encode(&(m_cdromProperties.tracks[indexTrack])))
                {
                    qCritical() << "CdromRipper: FLAC audio encoding failed";
                    free(audioTrackBuf);
                    return false;
                }
                m_flacEncoder.clearFilenames();

                // remove the temporary WAV file
                if (!QFile::remove(m_wavFilenameCurrentTrack))
                {
                    qCritical() << "CdromRipper: remove the WAV temporary file failed";
                }
            }
        }
    }
    else
    {
        qCritical() << "CdromRipper: file audio creation failed";
    }

    free(audioTrackBuf);
    return true;
}

bool CdromRipper::buildAudioFilenames(unsigned int indexTrack)
{
    QString mainDirectoryPath("/tmp");
    QString defaultArtistName("DefaultArtistName");
    QString defaultAlbumName("DefaultAlbumName");
    QString defaultTrackName("trackname");

    QString artistName = defaultArtistName;
    QString albumName = defaultAlbumName;
    QString trackName = defaultTrackName;

    unsigned int trackPosition = m_cdromProperties.tracks[indexTrack].position;

    // Find the main directory path
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;

    QString locations;
    EMS_LOAD_SETTINGS(locations, "main/locations",
                      QStandardPaths::standardLocations(QStandardPaths::MusicLocation)[0],
                      String);
    QStringList locationList = locations.split( " " );
    if (locationList.size() > 0)
    {
        mainDirectoryPath = locationList[0];
    }

    // Find the artist
    if (m_cdromProperties.tracks[indexTrack].artists.size() > 0)
    {
        if (!m_cdromProperties.tracks[indexTrack].artists[0].name.isEmpty())
        {
            artistName = m_cdromProperties.tracks[indexTrack].artists[0].name;
        }
    }

    // Find the album
    if (!m_cdromProperties.tracks[indexTrack].album.name.isEmpty())
    {
        albumName = m_cdromProperties.tracks[indexTrack].album.name;
    }

    // Find the trackname
    if (!m_cdromProperties.tracks[indexTrack].name.isEmpty())
    {
        trackName = m_cdromProperties.tracks[indexTrack].name;
    }

    QString directoryPath = mainDirectoryPath;
    QString filename;
    QString wavExtension = ".wav";
    QString flacExtension = ".flac";

    // if the wanted audio format is not WAV, the wav file will be temporary
    if (m_audioFormat != "WAV")
    {
        wavExtension += ".tmp";
    }

    directoryPath += "/";
    directoryPath += artistName;
    directoryPath += "/";
    directoryPath += albumName;
    directoryPath += "/";

    if (trackPosition <= 9)
    {
        filename += "0";
    }
    filename += QString::number(trackPosition);
    filename += "-";
    filename += trackName;

    QString wavFilenameWithExtension = filename + wavExtension;
    QString flacFilenameWithExtension = filename + flacExtension;
    m_wavFilenameCurrentTrack = directoryPath + wavFilenameWithExtension;
    m_flacFilenameCurrentTrack = directoryPath + flacFilenameWithExtension;
    qDebug() << "CdromRipper: WAV audio filename: " << m_wavFilenameCurrentTrack;
    qDebug() << "CdromRipper: FLAC audio filename: " << m_flacFilenameCurrentTrack;

    // Create the directory if does not exist
    QDir dirManager("/");
    if (!dirManager.mkpath(directoryPath))
    {
        qCritical() << "CdromRipper: directory creation failed";
        return false;
    }

    // Create the discid file associated to this track
    if (!writeDiscId(directoryPath))
    {
        qCritical() << "CDromRipper: the 'disc_id' writing failed";
    }

    return true;
}

bool CdromRipper::writeRawFile(uint8_t *audioTrackBuf,
                               unsigned int bufferSize,
                               const QString &wavFilename)
{
    QFile trackFile(wavFilename);
    if (!trackFile.open(QIODevice::WriteOnly))
    {
        qCritical() << "CdromRipper: cannot open file for writing: "
                    << qPrintable(trackFile.errorString());
        return false;
    }
    QDataStream out(&trackFile);
    out.setVersion(QDataStream::Qt_5_2);
    out.writeRawData((const char*)audioTrackBuf, bufferSize);
    trackFile.close();

    return true;
}

void CdromRipper::buildRipProgressMessage(unsigned int indexTrack,
                                          lsn_t currentSector,
                                          lsn_t firstSector,
                                          lsn_t lastSector)
{
    m_emsRipProgressMutex.lock();

    m_emsRipProgress.track_in_progress = indexTrack + 1;
    m_emsRipProgress.overall_progress = 100 * currentSector / m_diskSectorQuantity;
    m_emsRipProgress.track_progress = (100 * (currentSector - firstSector + 1)) /
                                      (lastSector - firstSector + 1);

    m_emsRipProgressMutex.unlock();
}

void CdromRipper::computeDiskSectorQuantity()
{
    int nbTracks = m_cdromProperties.tracks.size();
    m_diskSectorQuantity = 0;

    if (nbTracks > 0)
    {
        unsigned int position = m_cdromProperties.tracks[0].position;
        lsn_t firstSector = m_cdromProperties.trackSectors[position].first;

        position = m_cdromProperties.tracks[nbTracks - 1].position;
        lsn_t lastSector = m_cdromProperties.trackSectors[position].second;

        m_diskSectorQuantity = lastSector - firstSector + 1;
    }
}

bool CdromRipper::writeDiscId(const QString &dirPath)
{
    QString filename = dirPath;
    filename += "discid";

    QFile discIdFile(filename);
    if (!discIdFile.open(QIODevice::WriteOnly))
    {
        qCritical() << "CdromRipper: cannot open file for writing: "
                    << qPrintable(discIdFile.errorString());
        return false;
    }
    QDataStream out(&discIdFile);
    out.setVersion(QDataStream::Qt_5_2);
    out.writeRawData((const char*)qPrintable(m_cdromProperties.disc_id),
                     m_cdromProperties.disc_id.size());
    discIdFile.close();
    return true;
}

void CdromRipper::observeRipProgress()
{
    m_emsRipProgressMutex.lock();
    emit ripProgressChanged(m_emsRipProgress);
    m_emsRipProgressMutex.unlock();
}
