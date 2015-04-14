#include <QDebug>
#include <cdio/cdio.h>
#include "CdromManager.h"
#include "MetadataManager.h"
#include "Player.h"
#include "CdromRipper.h"

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
    for (unsigned int i=0; i<cdroms.size(); i++)
    {
        cdromsOut->append(cdroms.at(i));
    }
    mutex.unlock();
}

bool CdromManager::getCdrom(QString device, EMSCdrom *cdrom)
{
    bool found = false;

    mutex.lock();
    for (unsigned int i=0; i<cdroms.size(); i++)
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
    if (Q_NULLPTR == m_cdromRipper)
        return false;

    return true;
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
    qDebug() << "Found " << QString("%1").arg(trackNumber-1) << " tracks in " << newCD.device;

    /* Retrieve each track data */
    for (track_t i=1; i<trackNumber; i++)
    {
        EMSTrack track;
        track.filename = newCD.device;
        track.name = QString("Track %1").arg(i);
        track.type = TRACK_TYPE_CDROM;
        track.position = i;
        track.lsnBegin = cdio_get_track_lsn(cdrom, i);
        track.lsnEnd = cdio_get_track_last_lsn(cdrom, i);
        track.duration = (track.lsnEnd - track.lsnBegin + 1) / CDIO_CD_FRAMES_PER_SEC; /* One sector = 1/75s */
        track.format = QString("cdda");
        track.sample_rate = 44100;
        newCD.tracks.append(track);
        /* For disc id computation */
        lba_t lba = cdio_lsn_to_lba (track.lsnBegin);
        sum += cddb_sum(lba / CDIO_CD_FRAMES_PER_SEC);
    }

    /* Plus last track which is the lead out */
    sum += cddb_sum(cdio_get_track_lba(cdrom, trackNumber) / CDIO_CD_FRAMES_PER_SEC);

    /* Compute the discid
     * Code copied from libcdio/example/discid.c
     */
    unsigned start_sec = cdio_get_track_lba(cdrom, 1) / CDIO_CD_FRAMES_PER_SEC;
    unsigned leadout_sec = cdio_get_track_lba(cdrom, CDIO_CDROM_LEADOUT_TRACK) / CDIO_CD_FRAMES_PER_SEC;
    unsigned total = leadout_sec - start_sec;
    unsigned id = ((sum % 0xff) << 24 | total << 8 | trackNumber);

    newCD.disc_id = QString().sprintf("%08X %d", id, trackNumber);
    for (track_t i=1; i <=trackNumber; i++)
    {
        lba_t lba = cdio_get_track_lba(cdrom, i);
        newCD.disc_id += QString().sprintf(" %ld", (long) lba);
    }
    newCD.disc_id += QString().sprintf(" %ld", leadout_sec);
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
            if(cdrom.tracks.at(i).position == track.position)
            {
                cdrom.tracks.replace(i, track);
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
    for (unsigned int i=0; i<cdroms.size(); i++)
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
    for (unsigned int i=0; i<playlist.tracks.size(); i++)
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
                                              bus(QDBusConnection::systemBus()),
                                              m_cdromRipper(Q_NULLPTR)
{
    cdio_init();

    connect(this, SIGNAL(cdromTrackNeedUpdate(EMSTrack,QStringList)), MetadataManager::instance(), SLOT(update(EMSTrack,QStringList)));
    connect(MetadataManager::instance(), SIGNAL(updated(EMSTrack,bool)), this, SLOT(cdromTrackUpdated(EMSTrack,bool)));
}

CdromManager::~CdromManager()
{
    disconnect(this, SIGNAL(cdromTrackNeedUpdate(EMSTrack,QStringList)), MetadataManager::instance(), SLOT(update(EMSTrack,QStringList)));
    disconnect(MetadataManager::instance(), SIGNAL(updated(EMSTrack,bool)), this, SLOT(cdromTrackUpdated(EMSTrack,bool)));

    if (m_cdromRipper)
        delete m_cdromRipper;
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
    /* Get first available CDROM */
    QVector<EMSCdrom> cdroms;
    this->getAvailableCdroms(&cdroms);

    /* Start the thread */
    if ((cdroms.size() > 0) && (!this->isRipInProgress()))
    {
        EMSCdrom cdrom = cdroms.at(0);
        m_cdromRipper = new CdromRipper(this);
        m_cdromRipper->setCdrom(cdrom);
        connect(m_cdromRipper, &CdromRipper::resultReady, this, &CdromManager::handleCdromRipperResults);
        connect(m_cdromRipper, &CdromRipper::finished, m_cdromRipper, &QObject::deleteLater);
        m_cdromRipper->start();
    }
}

void CdromManager::handleCdromRipperResults(const QString &result)
{
    qDebug() << "CdromManager handle cdrom ripper results: " << result;
    delete m_cdromRipper;
    m_cdromRipper = Q_NULLPTR;
}
