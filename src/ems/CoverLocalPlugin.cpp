#include <QFile>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>

#include "DefaultSettings.h"
#include "CoverLocalPlugin.h"


CoverLocalPlugin::CoverLocalPlugin()
{
    QSettings settings;
    capabilities << "cover";

    /* Download directory */
    QString cacheDirPath;
    EMS_LOAD_SETTINGS(cacheDirPath, "main/cache_directory",
                      QStandardPaths::standardLocations(QStandardPaths::CacheLocation)[0], String);
    QDir cacheDir(cacheDirPath);
    cacheDir.mkpath("local");
    QDir localCoverCacheDir(cacheDirPath + QDir::separator() + "local");
    localCoverCacheDir.mkdir("albums");
    m_albumsCacheDir = cacheDirPath + QDir::separator() + "local" + QDir::separator() + "albums";
    m_cacheDirPath = cacheDirPath;
}

CoverLocalPlugin::~CoverLocalPlugin()
{

}


bool CoverLocalPlugin::update(EMSTrack *track)
{
    QStringList lookup;

    // Search for specific file, preference for png
    lookup << "cover.png" << "folder.png" << "front.png" << "cover.jpg" << "folder.jpg" << "front.jpg";

    foreach(QString filename, lookup)
    {
        QString f =  QFileInfo(track->filename).absolutePath() + QDir::separator() +  filename;
        if (QFile::exists(f))
        {
            QString coverName = track->album.name.simplified();
            // FIXME : http server is not able to serve file with space in the name
            // remove space for now
            coverName = coverName.replace(" ", "");
            coverName +=  "_";
            coverName +=  filename;
            QString newFile = m_albumsCacheDir + QDir::separator() + coverName;
            // Copy, found file in cache directory
            QFile::copy(f, newFile);

            // Set the cover of album
            track->album.cover = newFile;
            return true;
        }
    }

    return false;
}

