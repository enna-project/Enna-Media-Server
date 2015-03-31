#ifndef ONLINEDBPLUGIN_H
#define ONLINEDBPLUGIN_H

#include <QMutex>
#include <QObject>
#include "Data.h"


/* This class handle synchronous look up of data about an EMSTrack
 * This is an abstract class. An underlying plugin like Gracenote, CDDB or
 * musicbrainz implement these methods.
 */
class OnlineDBPlugin : public QObject
{
    Q_OBJECT

public:
    virtual bool lookup(EMSTrack *track) = 0;
    void lock() { mutex.lock(); }
    void unlock() { mutex.unlock(); }

protected:
    QMutex mutex;
};

#endif // ONLINEDBPLUGIN_H
