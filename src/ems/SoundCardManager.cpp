#include <QDebug>
#include <QtDBus>
#include "SoundCardManager.h"

#define INTERFACE "com.EnnaMediaServer.SoundCard"
#define SIGNAL_NAME_PLUGGED "plugged"
#define SIGNAL_NAME_UNPLUGGED "unplugged"

void SoundCardManager::dbusMessagePlugged(QString message)
{

}

void SoundCardManager::dbusMessageUnplugged(QString message)
{

}

SoundCardManager::SoundCardManager(QObject *parent) : QObject(parent), bus(QDBusConnection::systemBus())
{
    if (bus.isConnected())
    {
        bus.connect(QString(), QString(), INTERFACE , SIGNAL_NAME_PLUGGED, this, SLOT(dbusMessagePlugged(QString)));
        bus.connect(QString(), QString(), INTERFACE , SIGNAL_NAME_UNPLUGGED, this, SLOT(dbusMessageUnplugged(QString)));
    }
}

SoundCardManager::~SoundCardManager()
{
    if (bus.isConnected())
    {
        bus.disconnect(QString(), QString(), INTERFACE , SIGNAL_NAME_PLUGGED, this, SLOT(dbusMessagePlugged(QString)));
        bus.disconnect(QString(), QString(), INTERFACE , SIGNAL_NAME_UNPLUGGED, this, SLOT(dbusMessageUnplugged(QString)));
    }
}
