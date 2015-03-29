#include <QDebug>
#include "CdromManager.h"

#include <cdio/cdda.h>

#define INTERFACE "com.EnnaMediaServer.Cdrom"
#define SIGNAL_NAME_INSERT "inserted"
#define SIGNAL_NAME_REMOVE "removed"

/* ---------------------------------------------------------
 *                    INSERT/REMOVE HOOK
 * --------------------------------------------------------- */
void CdromManager::dbusMessageInsert(QString message)
{
    EMSCdrom newCD;

    /* The given message only contain the device name */
    newCD.device = message;

    /* Open the CD and get track data (number, duration, ...) */
    cdrom_drive_t *cdrom = cdio_cddap_identify(newCD.device.toStdString().c_str(), 1, NULL);
    if (!cdrom)
    {
        /* Ignore not valid CDROM */
        return;
    }
    if (cdio_cddap_open(cdrom))
    {
        qCritical() << "Unable to open device " << newCD.device;
    }

    track_t trackNumber = cdio_cddap_tracks(cdrom);
    if (trackNumber <= 0)
    {
        qCritical() << "Unable to CDROM TOC";
    }

    for (track_t i=0; i<trackNumber; i++)
    {
        EMSTrack track;
        track.filename = newCD.device;
        track.name = QString("Track %1").arg(i);
        track.type = TRACK_TYPE_CDROM;
        lsn_t begin = cdio_cddap_disc_firstsector(cdrom);
        lsn_t end = cdio_cddap_disc_firstsector(cdrom);
        track.duration = (end - begin + 1) / 75; /* One sector = 1/75s */
        newCD.tracks.append(track);
    }


    /* TODO: get the discid */

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
}

void CdromManager::stopMonitor()
{
    if (bus.isConnected())
    {
        bus.disconnect(QString(), QString(), INTERFACE , SIGNAL_NAME_INSERT, this, SLOT(dbusMessageInsert(QString)));
        bus.disconnect(QString(), QString(), INTERFACE , SIGNAL_NAME_REMOVE, this, SLOT(dbusMessageRemove(QString)));
    }
}
