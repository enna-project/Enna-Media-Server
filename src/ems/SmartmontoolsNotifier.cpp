#include <QDebug>
#include "SmartmontoolsNotifier.h"

#define INTERFACE "com.EnnaMediaServer.Smart"
#define SIGNAL_NAME "warnUser"

SmartmontoolsNotifier::SmartmontoolsNotifier(QObject *parent) : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::systemBus();
    if ( !bus.isConnected() )
    {
        qDebug() << "Could not connect to session bus";
    }
    else
    {
        QDBusConnection::systemBus().connect(QString(), QString(), INTERFACE , SIGNAL_NAME, this, SLOT(messageReceived(QString)));
    }
}

SmartmontoolsNotifier::~SmartmontoolsNotifier()
{

}

void SmartmontoolsNotifier::messageReceived(QString message)
{
    qDebug() << message;
}
