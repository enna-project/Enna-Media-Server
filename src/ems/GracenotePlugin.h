#ifndef GRACENOTEPLUGIN_H
#define GRACENOTEPLUGIN_H

#include "MetadataPlugin.h"
#include "Data.h"
#include <gnsdk3/gnsdk.h>

class GracenotePlugin : public MetadataPlugin
{
    Q_OBJECT

public:
    GracenotePlugin();
    ~GracenotePlugin();

    bool update(EMSTrack *track);

private:
    bool configured;
    QString clientID;
    QString clientIDTags;
    QString licensePath;
    QString userHandlePath;
    gnsdk_user_handle_t userHandle;

    QString albumsCacheDir;
    QString genresCacheDir;
    QString artistsCacheDir;

    bool getUserHandle();
    bool configure();
    bool lookupByDiscID(EMSTrack *track, EMSCdrom cdrom);
    bool albumGdoToEMSTrack(gnsdk_gdo_handle_t albumGdo, EMSTrack *track);
    void displayLastError();
};

#endif // GRACENOTEPLUGIN_H
