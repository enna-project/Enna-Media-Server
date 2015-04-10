#ifndef SNDFILEPLUGIN_H
#define SNDFILEPLUGIN_H

#include "MetadataPlugin.h"
#include "Data.h"

class SndfilePlugin : public MetadataPlugin
{
    Q_OBJECT

public:
    SndfilePlugin();
    ~SndfilePlugin();

    bool update(EMSTrack *track);

private:
};

#endif // SNDFILEPLUGIN_H
