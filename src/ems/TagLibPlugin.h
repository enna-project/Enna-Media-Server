#ifndef TAGLIBPLUGIN_H
#define TAGLIBPLUGIN_H

#include "MetadataPlugin.h"
#include "Data.h"

class TagLibPlugin : public MetadataPlugin
{
    Q_OBJECT

public:
    TagLibPlugin();
    ~TagLibPlugin();

    bool update(EMSTrack *track);

private:
    void addArtist(EMSTrack *track, QString str);
    void addGenre(EMSTrack *track, QString str);
};

#endif // TAGLIBPLUGIN_H
