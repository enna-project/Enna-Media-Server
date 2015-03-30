#include <QDebug>
#include "CdromManager.h"

#include <cdio/cdio.h>

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

/* ---------------------------------------------------------
 *                    INSERT/REMOVE HOOK
 * --------------------------------------------------------- */
void CdromManager::dbusMessageInsert(QString message)
{
    EMSCdrom newCD;
    unsigned int sum = 0;

    /* The given message only contain the device name */
    newCD.device = message;
    qDebug() << "New CDROM inserted : " << newCD.device;

    cdio_init();

    /* Open the CD and get track data (number, duration, ...) */
    CdIo_t *cdrom = cdio_open(newCD.device.toStdString().c_str(), DRIVER_UNKNOWN);
    if (!cdrom)
    {
        /* Ignore not valid CDROM */
        qCritical() << "Unable to open device " << newCD.device;
        return;
    }

    /* Get size of the disck */
    track_t trackNumber = cdio_get_num_tracks(cdrom);
    if (trackNumber <= 0)
    {
        qCritical() << "Unable to read CDROM TOC";
    }
    qDebug() << "Found " << QString("%1").arg(trackNumber) << " tracks in " << newCD.device;

    /* Retrieve each track data */
    for (track_t i=1; i<=trackNumber; ++i)
    {
        EMSTrack track;
        track.filename = newCD.device;
        track.name = QString("Track %1").arg(i);
        track.type = TRACK_TYPE_CDROM;
        lsn_t begin = cdio_get_track_lsn(cdrom, i);
        lsn_t end = cdio_get_track_last_lsn(cdrom, i);
        track.duration = (end - begin + 1) / CDIO_CD_FRAMES_PER_SEC; /* One sector = 1/75s */
        newCD.tracks.append(track);
        /* For disc id computation */
        lba_t lba = cdio_lsn_to_lba (begin);
        sum += cddb_sum(lba / CDIO_CD_FRAMES_PER_SEC);
    }

    /* Compute the discid
     * Code copied from libcdio/example/discid.c
     */
    unsigned start_sec = cdio_get_track_lba(cdrom, 1) / CDIO_CD_FRAMES_PER_SEC;
    unsigned leadout_sec = cdio_get_track_lba(cdrom, CDIO_CDROM_LEADOUT_TRACK) / CDIO_CD_FRAMES_PER_SEC;
    unsigned total = leadout_sec - start_sec;
    unsigned id = ((sum % 0xff) << 24 | total << 8 | trackNumber);

    newCD.disc_id = QString().sprintf("%08X %d", id, trackNumber);
    for (track_t i=1; i <= trackNumber; ++i)
    {
        lba_t lba = cdio_get_track_lba(cdrom, i);
        newCD.disc_id += QString().sprintf(" %ld", (long) lba);
    }
    newCD.disc_id += QString().sprintf(" %ld\n", (long) cdio_get_track_lba(cdrom, CDIO_CDROM_LEADOUT_TRACK) / CDIO_CD_FRAMES_PER_SEC);
    cdio_destroy(cdrom);

    qDebug() << "Computed DISCID is " << newCD.disc_id;

    /* Add the new CDROM in the list and emit signal to warn listeners */
    mutex.lock();
    cdroms.append(newCD); /* TODO: check the CD is not already present */
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
    qDebug() << message;
    /* TODO: async.
     * ------
     * Cancel all active thread that concern this CDrom
     * ------
     * Remove CDrom tracks that are in the current playlist
     * ------
     * Emit signal
     */
}

/* ---------------------------------------------------------
 *                  START/STOP CDROM MANAGER
 * --------------------------------------------------------- */

CdromManager* CdromManager::_instance = 0;

CdromManager::CdromManager(QObject *parent) : QObject(parent), bus(QDBusConnection::systemBus())
{
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
