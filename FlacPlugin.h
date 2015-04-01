#ifndef FLACPLUGIN_H
#define FLACPLUGIN_H

#include "MetadataPlugin.h"
#include "Data.h"

class FlacPlugin : public MetadataPlugin
{
    Q_OBJECT

public:
    FlacPlugin();
    ~FlacPlugin();

    bool update(EMSTrack *track);
};

#endif // FLACPLUGIN_H
