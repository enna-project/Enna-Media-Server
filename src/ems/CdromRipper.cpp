#include "CdromRipper.h"

#include <cdio/cd_types.h>
#include <QDebug>
#include <QFile>


CdromRipper::CdromRipper(QObject *parent)
    : QThread(parent)
{}

void CdromRipper::setCdrom(const EMSCdrom &cdromProperties)
{
    m_cdromProperties = cdromProperties;
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

    if (!this->identifyDrive())
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

    if (!this->openDrive())
    {
        qCritical() << "CdromRipper: rip aborted";
        return;
    }

    this->initializeParanoia();
    this->computeDiskSectorQuantity();

    unsigned int nbTracks = m_cdromProperties.tracks.size();
    for (unsigned int indexTrack = 0; indexTrack < nbTracks; ++indexTrack)
    {
        this->ripOneTrack(indexTrack);
    }

    this->closeDrive();
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

        cdio_cddap_free_messages(psz_err);
        cdio_cddap_free_messages(psz_mes);

        if (!p_readbuf)
        {
            qCritical() << "CdromRipper: paranoia read error. Track rip aborted.";
            free(audioTrackBuf);
            return false;
        }

        memcpy(writeIndex, p_readbuf, CDIO_CD_FRAMESIZE_RAW);
        writeIndex += CDIO_CD_FRAMESIZE_RAW;

        this->buildRipProgressMessage(indexTrack, currentLsn,
                                      lsnBegin, lsnEnd);

        currentLsn++;
    }

    QString wavFilename = this->buildWavFilename(trackPosition);

    if (!this->writeRawFile(audioTrackBuf, bufferSize, wavFilename))
    {
            qCritical() << "CdromRipper: Track rip aborted.";
            free(audioTrackBuf);
            return false;
    }

    free(audioTrackBuf);
    return true;
}

QString CdromRipper::buildWavFilename(unsigned int trackPosition)
{
    QString wavName("/tmp/track_mbo_");
    wavName += QString::number(trackPosition);
    wavName += ".wav";
    qDebug() << "CdromRipper: raw audio filename: " << wavName;

    return wavName;
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
    m_emsRipProgress.track_in_progress = indexTrack + 1;
    m_emsRipProgress.overall_progress = 100 * currentSector / m_diskSectorQuantity;
    m_emsRipProgress.track_progress = (100 * (currentSector - firstSector)) /
                                      (lastSector - firstSector + 1);
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
