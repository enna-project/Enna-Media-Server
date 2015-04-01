#include "GracenotePlugin.h"
#include <gnsdk3/gnsdk_defines.h>

GracenotePlugin::GracenotePlugin()
{
    capabilities << "discid";
    /* Not implemented :
    capabilities << "fingerprint";
    capabilities << "album_name";
    capabilities << "track_name";
    capabilities << "track_gnid";
    capabilities << "album_name";
    capabilities << "album_gnid";
    */
}

GracenotePlugin::~GracenotePlugin()
{

}

bool GracenotePlugin::update(EMSTrack *track)
{
    //TODO
    return true;
}
