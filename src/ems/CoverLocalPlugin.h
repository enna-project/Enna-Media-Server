#ifndef COVERLOCALPLUGIN_H
#define COVERLOCALPLUGIN_H

#include "MetadataPlugin.h"
#include "Data.h"

/* The CoverLocal Plugin search for cover files in the same directory where the
 * track is saved.
 */

class CoverLocalPlugin : public MetadataPlugin
{
    Q_OBJECT

public:
    CoverLocalPlugin();
    ~CoverLocalPlugin();

    bool update(EMSTrack *track);

private:

    /* Cache directories */
    QString m_albumsCacheDir;
    QString m_cacheDirPath;
};
#endif // COVERLOCALPLUGIN_H
