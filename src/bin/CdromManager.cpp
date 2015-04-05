#include <QDebug>
#include <cdio/cdio.h>
#include "CdromManager.h"
#include "Player.h"

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
        lsn_t begin = cdio_get_track_lsn(cdrom, i);
        lsn_t end = cdio_get_track_last_lsn(cdrom, i);
        track.duration = (end - begin + 1) / CDIO_CD_FRAMES_PER_SEC; /* One sector = 1/75s */
        newCD.tracks.append(track);
        /* For disc id computation */
        lba_t lba = cdio_lsn_to_lba (begin);
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

    /* TODO:
     * If network, ask Gracenote additional data (using disc_id)
     * Fill CDrom tracks structure with additional data
     * Emit signal with new CDrom struct
     */
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

CdromManager::CdromManager(QObject *parent) : QObject(parent), bus(QDBusConnection::systemBus())
{
    cdio_init();
}

CdromManager::~CdromManager()
{

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