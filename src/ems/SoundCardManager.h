#ifndef SOUNDCARDMANAGER_H
#define SOUNDCARDMANAGER_H

#include <QObject>
#include <QVector>
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
    void mpdOutputsChanged(QVector<EMSSndCard>);

private:
    QDBusConnection bus;
    QVector<EMSSndCard> sndCards;
    bool configured;

    QVector<EMSSndCard> getMpdIDbyType(QString type, bool onlyPresent);
    QStringList getTypeList();
    void getInitialPresence();
    void applyPolicy();

};

#endif // SOUNDCARDMANAGER_H
