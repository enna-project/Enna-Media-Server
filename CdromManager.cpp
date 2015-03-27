#include <QDebug>
#include "CdromManager.h"

#define INTERFACE "com.metronome_technologie.Cdrom"
#define SIGNAL_NAME_INSERT "insert"
#define SIGNAL_NAME_REMOVE "remove"

CdromManager::CdromManager(QObject *parent) : QObject(parent), bus(QDBusConnection::systemBus())
{
    if (!bus.isConnected())
    {
        qCritical() << "Could not connect to session bus ! No CDROM management.";
        return;
    }

    bus.connect(QString(), QString(), INTERFACE , SIGNAL_NAME_INSERT, this, SLOT(dbusMessageInsert(QString)));
    bus.connect(QString(), QString(), INTERFACE , SIGNAL_NAME_REMOVE, this, SLOT(dbusMessageRemove(QString)));
}

CdromManager::~CdromManager()
{

}

void CdromManager::dbusMessageInsert(QString message)
{
    qDebug() << message;
    /* TODO: async.
     * ------
     * chmod 777 [dev]
     * hdparm -B 255 [dev]
     * hdparm -E 10 [dev]
     * hdparm -S 5 [dev]
     * ------
     * cdio => list tracks + durations
     * disc-id => get discid
     * ------
     * Construct CDrom struct and emit signal
     * ------
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
