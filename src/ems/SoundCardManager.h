#ifndef SOUNDCARDMANAGER_H
#define SOUNDCARDMANAGER_H

#include <QObject>
#include <QtDBus/QtDBus>

#include "Data.h"

class SoundCardManager : public QObject
{
    Q_OBJECT

public:
    SoundCardManager(QObject *parent = 0);
    ~SoundCardManager();

public slots:
    void dbusMessagePlugged(QString message);
    void dbusMessageUnplugged(QString message);

private:
    QDBusConnection bus;

};

#endif // SOUNDCARDMANAGER_H
