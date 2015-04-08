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
    /* Connection data */
    bool configured;
    QString clientID;
    QString clientIDTags;
    QString licensePath;
    QString userHandlePath;

    /* Global GNSDK struct */
    gnsdk_user_handle_t userHandle;
    gnsdk_list_handle_t genreListHandle;

    /* Cache directories */
    QString albumsCacheDir;
    QString genresCacheDir;
    QString artistsCacheDir;

    /* Available image format (jpeg, png, ...) */
    QStringList imageFormats;

    /* Manage connection */
    bool getUserHandle();
    bool configure();
    void displayLastError();

    /* Entry points for look up */
    bool lookupByDiscID(EMSTrack *track, EMSCdrom cdrom);

    /* Internal utils */
    bool albumGdoToEMSTrack(gnsdk_gdo_handle_t albumGdo, EMSTrack *track);
    bool trackGdoToEMSTrack(gnsdk_gdo_handle_t trackGdo, EMSTrack *track);
    void gdoToEMSArtists(gnsdk_gdo_handle_t gdo, QVector<EMSArtist> *artists);
    void gdoToEMSGenres(gnsdk_gdo_handle_t gdo, QVector<EMSGenre> *genres);
    QStringList getGdoValue(gnsdk_gdo_handle_t gdo, gnsdk_cstr_t key);

    /* Image downloader for gracenote */
    bool lookForDownloadedImage(QString dir, QString basename, QString *fullpath);
    bool downloadImage(gnsdk_link_query_handle_t queryHandle, gnsdk_link_content_type_t type,
                       QString dir, QString basename, QString *computedPath);

};

#endif // GRACENOTEPLUGIN_H
