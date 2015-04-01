#ifndef GRACENOTEPLUGIN_H
#define GRACENOTEPLUGIN_H

#include "MetadataPlugin.h"
#include "Data.h"

class GracenotePlugin : public MetadataPlugin
{
    Q_OBJECT

public:
    GracenotePlugin();
    ~GracenotePlugin();

    bool update(EMSTrack *track);
};

#endif // GRACENOTEPLUGIN_H
