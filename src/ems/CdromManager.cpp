#include <QDebug>
#include <cdio/cdio.h>
#include "CdromManager.h"
#include "MetadataManager.h"
#include "Player.h"

#ifdef EMS_CDROM_RIPPER
#include "CdromRipper.h"
#endif

#define INTERFACE "com.EnnaMediaServer.Cdrom"
#define SIGNAL_NAME_INSERT "inserted"
#define SIGNAL_NAME_REMOVE "removed"

/* ---------------------------------------------------------
 *                          UTILS
 * --------------------------------------------------------- */
static inline int cddb_sum(int n)
{
    int result = 0;
    while (n > 0)
    {
        result += n % 10;
        n /= 10;
    }
    return result;
}

void CdromManager::getAvailableCdroms(QVector<EMSCdrom> *cdromsOut)
{
    cdromsOut->clear();
    mutex.lock();
    for (int i=0; i<cdroms.size(); i++)
    {
        cdromsOut->append(cdroms.at(i));
    }
    mutex.unlock();
}

bool CdromManager::getCdrom(QString device, EMSCdrom *cdrom)
{
    bool found = false;

    mutex.lock();
    for (int i=0; i<cdroms.size(); i++)
    {
        if (cdroms.at(i).device == device)
        {
            *cdrom = cdroms.at(i);
            found = true;
            break;
        }
    }
    mutex.unlock();

    return found;
}

bool CdromManager::isRipInProgress()
{
#ifdef EMS_CDROM_RIPPER
    if (m_cdromRipper)
        return true;
#endif
    return false;
}

void CdromManager::setRipAudioFormat(const QString &ripAudioFormat)
{
    m_ripAudioFormat = ripAudioFormat;
}

/* ---------------------------------------------------------
 *                    INSERT/REMOVE HOOK
 * --------------------------------------------------------- */
void CdromManager::dbusMessageInsert(QString message)
{
    EMSCdrom newCD;
    unsigned int sum = 0;

    /* Check the new CD is not already in the list */
    if (getCdrom(message, &newCD))
    {
        qCritical() << "CD already in the list of current CDROM.";
        return;
    }

    /* The given message only contain the device name */
    newCD.device = message;
    qDebug() << "New CDROM inserted : " << newCD.device;

    /* Open the CD and get track data (number, duration, ...) */
    CdIo_t *cdrom = cdio_open(newCD.device.toStdString().c_str(), DRIVER_UNKNOWN);
    if (!cdrom)
    {
        qCritical() << "Unable to open device " << newCD.device;
        return;
    }

    /* Check that the inserted cdrom is a CD-DA */
    discmode_t mode = cdio_get_discmode(cdrom);
    if (mode != CDIO_DISC_MODE_CD_DA)
    {
        /* Ignore not valid CDROM */
        qDebug() << "Inserted disk is not an audio CD " << QString("(%1)").arg(mode);
        return;
    }

    /* Get size of the disck */
    track_t trackNumber = cdio_get_num_tracks(cdrom);
    if (trackNumber <= 1)
    {
        qCritical() << "Unable to read CDROM TOC";
    }
    qDebug() << "Found " << QString("%1").arg(trackNumber) << " tracks in " << newCD.device;

    /* Retrieve each track data */
    for (track_t i=1; i<=trackNumber; i++)
    {
        EMSTrack track;
        track.filename = newCD.device;
        track.type = TRACK_TYPE_CDROM;
        track.position = i;
        lsn_t lsnBegin = cdio_get_track_lsn(cdrom, i);
        lsn_t lsnEnd = cdio_get_track_last_lsn(cdrom, i);
        track.duration = (lsnEnd - lsnBegin + 1) / CDIO_CD_FRAMES_PER_SEC; /* One sector = 1/75s */
        track.format = QString("cdda");
        track.sample_rate = 44100;

        /* Set a default name waiting metadata */
        track.name = QString("Track %1").arg(track.position);
        track.album.name = QString("Unknown album");

        /* Add the new track in the list only if it is an audio track */
        track_format_t format = cdio_get_track_format(cdrom, i);
        if (format == TRACK_FORMAT_AUDIO)
        {
            newCD.tracks.append(track);
        }

        /* For disc id computation */
        lba_t lba = cdio_lsn_to_lba (lsnBegin);
        sum += cddb_sum(lba / CDIO_CD_FRAMES_PER_SEC);

        QPair<lsn_t, lsn_t> trackSectors(lsnBegin, lsnEnd);
        newCD.trackSectors[i] = trackSectors;
    }

    /* Compute the discid
     * Code copied from libcdio/example/discid.c
     */
    unsigned start_sec = cdio_get_track_lba(cdrom, 1) / CDIO_CD_FRAMES_PER_SEC;
    unsigned int leadout = cdio_get_track_lba(cdrom, CDIO_CDROM_LEADOUT_TRACK);
    unsigned leadout_sec = leadout / CDIO_CD_FRAMES_PER_SEC;
    unsigned total = leadout_sec - start_sec;
    unsigned id = ((sum % 0xff) << 24 | total << 8 | trackNumber);

    newCD.disc_id = QString().sprintf("%08X %d", id, trackNumber);
    for (track_t i=1; i <=trackNumber; i++)
    {
        lba_t lba = cdio_get_track_lba(cdrom, i);
        newCD.disc_id += QString().sprintf(" %ld", (long) lba);
    }
    newCD.disc_id += QString().sprintf(" %u", leadout);
    cdio_destroy(cdrom);

    qDebug() << "Computed DISCID is " << newCD.disc_id;

    /* Add the new CDROM in the list and emit signal to warn listeners */
    mutex.lock();
    cdroms.append(newCD);
    mutex.unlock();
    emit cdromInserted(newCD);

    /* Ask metadata to all plugins which can handle "discid" lookup
     */
    QStringList capabilities;
    capabilities << "discid";
    for(int i=0; i<newCD.tracks.size(); i++)
    {
        emit cdromTrackNeedUpdate(newCD.tracks.at(i), capabilities);
    }
}

void CdromManager::cdromTrackUpdated(EMSTrack track, bool complete)
{
    bool found = false;
    unsigned cdromID = 0;

    if (track.type != TRACK_TYPE_CDROM)
    {
        return;
    }

    if (!complete)
    {
        return;
    }

    /* Retrieve the corresponding cdrom in the list of cdroms */
    mutex.lock();
    for (int i=0; i<cdroms.size(); i++)
    {
        if (cdroms.at(i).device == track.filename)
        {
            cdromID = i;
            found = true;
            break;
        }
    }

    if (found)
    {
        EMSCdrom cdrom = cdroms.at(cdromID);

        for(int i=0; i<cdrom.tracks.size(); i++)
        {
            /* Replace the new track */
            if(cdrom.tracks.at(i).position == track.position)
            {
                cdrom.tracks.replace(i, track);
                break;
            }

        }
        cdroms[cdromID] = cdrom;
    }

    if (found)
    {
        emit cdromChanged(cdroms.at(cdromID));
    }
    mutex.unlock();
}

void CdromManager::dbusMessageRemove(QString message)
{
    bool found = false;
    EMSCdrom cdrom;
    mutex.lock();
    for (int i=0; i<cdroms.size(); i++)
    {
        if (cdroms.at(i).device == message)
        {
            cdrom = cdroms.at(i);
            cdroms.removeAt(i);
            found = true;
            break;
        }
    }
    mutex.unlock();

    if (!found)
    {
        /* Ejected CDROM is not found */
        return;
    }

    qDebug() << "CDROM ejected : " << cdrom.device;

    /* Remove played CDROM from the current playlist */
    EMSPlaylist playlist = Player::instance()->getCurentPlaylist();
    for (int i=0; i<playlist.tracks.size(); i++)
    {
        EMSTrack track = playlist.tracks.at(i);
        if (track.type == TRACK_TYPE_CDROM && track.filename == cdrom.device)
        {
            Player::instance()->removeTrack(track);
        }
    }

    emit cdromEjected(cdrom);
}

/* ---------------------------------------------------------
 *                  START/STOP CDROM MANAGER
 * --------------------------------------------------------- */

CdromManager* CdromManager::_instance = 0;

CdromManager::CdromManager(QObject *parent) : QObject(parent),
                                              bus(QDBusConnection::systemBus())
{
    cdio_init();

    qRegisterMetaType<EMSCdrom>("EMSCdrom");

    connect(this, SIGNAL(cdromTrackNeedUpdate(EMSTrack,QStringList)), MetadataManager::instance(), SLOT(update(EMSTrack,QStringList)), Qt::DirectConnection);
    connect(MetadataManager::instance(), SIGNAL(updated(EMSTrack,bool)), this, SLOT(cdromTrackUpdated(EMSTrack,bool)));
}

CdromManager::~CdromManager()
{
#ifdef EMS_CDROM_RIPPER
    delete m_cdromRipper;
#endif
}

bool CdromManager::startMonitor()
{
    if (!bus.isConnected())
    {
        qCritical() << "Could not connect to session bus ! No CDROM management.";
        return false;
    }
    else
    {
        bus.connect(QString(), QString(), INTERFACE , SIGNAL_NAME_INSERT, this, SLOT(dbusMessageInsert(QString)));
        bus.connect(QString(), QString(), INTERFACE , SIGNAL_NAME_REMOVE, this, SLOT(dbusMessageRemove(QString)));
    }

    /* In case of CDROM already present, trigger actions using udevadm
     * Note: EMS must be launched with root rights or be able to trigger action using udev.
     */
    QProcess cmd;
    cmd.execute(QString("/bin/udevadm"), QString("trigger --action=change --sysname-match=sr[0-9]").split(' '));

    return true;
}

void CdromManager::stopMonitor()
{
    if (bus.isConnected())
    {
        bus.disconnect(QString(), QString(), INTERFACE , SIGNAL_NAME_INSERT, this, SLOT(dbusMessageInsert(QString)));
        bus.disconnect(QString(), QString(), INTERFACE , SIGNAL_NAME_REMOVE, this, SLOT(dbusMessageRemove(QString)));
    }
}

void CdromManager::startRip()
{
#ifdef EMS_CDROM_RIPPER

    /* Get first available CDROM */
    QVector<EMSCdrom> cdroms;
    getAvailableCdroms(&cdroms);

    /* Start the thread */
    if ((cdroms.size() > 0) && (!isRipInProgress()))
    {
        EMSCdrom cdrom = cdroms.at(0);
        m_cdromRipper = new CdromRipper(this);

        connect(m_cdromRipper, &CdromRipper::resultReady, this, &CdromManager::handleCdromRipperResults);
        connect(m_cdromRipper, &CdromRipper::finished, m_cdromRipper, &QObject::deleteLater);
        connect(m_cdromRipper, &CdromRipper::ripProgressChanged, this, &CdromManager::ripProgress);

        m_cdromRipper->setCdrom(cdrom);
        m_cdromRipper->setAudioFormat(m_ripAudioFormat);
        m_cdromRipper->start();
    }

#endif
}

void CdromManager::ripProgress(EMSRipProgress ripProgress)
{
    emit ripProgressChanged(ripProgress);
}

void CdromManager::handleCdromRipperResults(const QString &result )
{
#ifdef EMS_CDROM_RIPPER

    if (m_cdromRipper)
    {
        qDebug() << "CdromManager handle cdrom ripper results: " << result;
        delete m_cdromRipper;
        m_cdromRipper = Q_NULLPTR;
    }
#endif
}
