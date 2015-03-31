#ifndef GRACENOTEPLUGIN_H
#define GRACENOTEPLUGIN_H

#include "OnlineDBPlugin.h"
#include "Data.h"

class GracenotePlugin : public OnlineDBPlugin
{
    Q_OBJECT

public:
    GracenotePlugin();
    ~GracenotePlugin();

    bool lookup(EMSTrack *track);
};

#endif // GRACENOTEPLUGIN_H
