#include <QDebug>
#include "SmartmontoolsNotifier.h"

#define INTERFACE "com.metronome_technologie.Smart"
#define SIGNAL_NAME "warnUser"

smartmontoolsNotifier::smartmontoolsNotifier(QObject *parent) : QObject(parent)
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

smartmontoolsNotifier::~smartmontoolsNotifier()
{

}

void smartmontoolsNotifier::messageReceived(QString message)
{
    qDebug() << message;
}
