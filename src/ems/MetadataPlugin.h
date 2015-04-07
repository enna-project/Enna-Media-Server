#ifndef METADATAPLUGIN_H
#define METADATAPLUGIN_H

#include <QMutex>
#include <QObject>
#include <QStringList>
#include "Data.h"


/* This class handle synchronous look up of data about an EMSTrack
 * This is an abstract class. An underlying plugin should inherit this class
 * The plugins selection is done using a word : the "capability". This word
 * must match that is defined in the plugin code.
 * For metadata specific to a file format, the capability is the name of the
 * extention. For example : "flac" or "dsf".
 */
class MetadataPlugin : public QObject
{
    Q_OBJECT

public:
    virtual bool update(EMSTrack *track) = 0;

    QStringList getCapabilities() { QStringList out; mutex.lock(); out = capabilities; mutex.unlock(); return out; }
    void lock() { mutex.lock(); }
    void unlock() { mutex.unlock(); }

protected:
    QMutex mutex;
    QStringList capabilities;
};

#endif // METADATAPLUGIN_H
