#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSettings>
#include <QThread>

#include "LocalFileScanner.h"
#include "DirectoryWorker.h"
#include "MetadataManager.h"
#include "Database.h"

LocalFileScanner::LocalFileScanner(QObject *parent) : QObject(parent)
{
    supportedFormat = "*.flac, *.mp3, *.wav";
    scanActive = false;

    connect(this, SIGNAL(trackNeedUpdate(EMSTrack, QStringList)), MetadataManager::instance(), SLOT(update(EMSTrack,QStringList)));
    connect(MetadataManager::instance(), SIGNAL(updated(EMSTrack,bool)), this, SLOT(trackUpdated(EMSTrack,bool)));
}

LocalFileScanner::~LocalFileScanner()
{
    disconnect(this, SIGNAL(trackNeedUpdate(EMSTrack,QStringList)), MetadataManager::instance(), SLOT(update(EMSTrack,QStringList)));
    connect(MetadataManager::instance(), SIGNAL(updated(EMSTrack,bool)), this, SLOT(trackUpdated(EMSTrack,bool)));
}

void LocalFileScanner::startScan()
{
    if (scanActive)
    {
        return;
    }
    scanActive = true;
    startTime = QDateTime::currentDateTime().toTime_t();
    measureTime.start();

    qDebug() << "Starting local file scanner...";

    for (int i=0; i<locations.size(); i++)
    {
        QString location = locations.at(i);
        // Create a new qthread
        QThread *directoryThread = new QThread;
        // Create the DirectoryWorker object : it scan directory recusively in a thread
        // And send fileFound() signal when a new file is found
        DirectoryWorker* dirWorker = new DirectoryWorker(location, supportedFormat);
        workers.append(dirWorker);
        // Move object to specific thread
        dirWorker->moveToThread(directoryThread);
        // Call fileFound slot when a new file is found
        connect(directoryThread, SIGNAL(started()), dirWorker, SLOT(process()));
        connect(dirWorker, SIGNAL(fileFound(QString, QString)), this, SLOT(fileFound(QString, QString)), Qt::DirectConnection);
        connect(dirWorker, SIGNAL(finished(DirectoryWorker*)), this, SLOT(workerFinished(DirectoryWorker*)));
        // Start the thread
        directoryThread->start();
    }
}

void LocalFileScanner::workerFinished(DirectoryWorker* worker)
{
    int workerId = -1;

    /* Find the thread in the list of workers */
    for (int i=0; i<workers.size(); i++)
    {
        if (worker == workers.at(i))
        {
            workerId = i;
            break;
        }
    }

    if (workerId == -1)
    {
        qCritical() << "Received event workerFinished from unknown worker.";
        return;
    }

    QThread *thread = worker->thread();
    if (thread != QCoreApplication::instance()->thread())
    {
        thread->quit();
        thread->wait(100);
        workers.removeAt(workerId);
        delete thread;
        delete worker;
    }

    if (workers.empty())
    {
        /* No more worker */
        scanEnd();
    }
}

void LocalFileScanner::scanEnd()
{
    Database *db = Database::instance();
    scanActive = false;
    QTime t(0, 0);
    t = t.addMSecs(measureTime.elapsed());
    qDebug() << "Scan finished. (duration: " << t.toString("HH:mm:ss.zzz") << ")";

    qDebug() << "Clean database... (remove non-existent files, ...)";
    db->lock();
    for (int i=0; i<locations.size(); i++)
    {
        QString location = locations.at(i);
        db->removeOldFiles(location, startTime);
    }
    db->cleanOrphans();
    db->unlock();
}

void LocalFileScanner::stopScan()
{
    if (scanActive)
    {
        foreach(DirectoryWorker* worker, workers)
        {
            QThread *thread = worker->thread();
            if (thread != QCoreApplication::instance()->thread())
            {
                thread->quit();
                thread->wait(100);
                delete thread;
                delete worker;
            }
        }
        workers.clear();
        scanActive = false;
    }
}

void LocalFileScanner::locationAdd(const QString &location)
{
    qDebug() << "Directory " << location << " is registered in the scanner.";
    locations.append(location);
}

void LocalFileScanner::fileFound(QString filename, QString sha1)
{
    EMSTrack track;
    Database *db = Database::instance();

    track.type = TRACK_TYPE_DB;
    track.filename = filename;
    track.sha1 = sha1;
    track.lastscan = startTime;
    QString extension = QFileInfo(filename).suffix().toLower();
    track.format = extension;
    track.id = 0;

    /* 1) Search existing sha1 in the database */
    unsigned long long trackID;
    db->lock();
    if(db->getTrackIdBySha1(&trackID, track.sha1))
    {
        /* Do nothing as we assume the metadata have correctly been seeked */
        db->insertNewFilename(track.filename, trackID, startTime);
        db->unlock();
        return;
    }
    db->unlock();

    /* Compute which plugins can be used to retrieve metadata */
    QStringList capabilities;
    /* 1) Use extension name to find all plugins which can handle this format */
    capabilities << track.format;
    /* 2) When first plugins add data to this track, try to use online database to get more data
     *    and covers using ... */
    //TODO.

    emit trackNeedUpdate(track, capabilities);
}

/* This slot is called when new data are available for this track. */
void LocalFileScanner::trackUpdated(EMSTrack track, bool complete)
{
    Database *db = Database::instance();

    /* First of all, check that the signal concern a track handled in this class */
    if (track.type != TRACK_TYPE_DB)
    {
        return;
    }

    /* Wait the last plugin before adding the track in the database */
    if (!complete)
    {
        return;
    }

    /* Make sure the track have a "name", otherwise choose a default one */
    if (track.name.isEmpty())
    {
        track.name = QFileInfo(track.filename).baseName();
    }

    /* Insert new track in database */
    unsigned long long albumId;
    QString directory = QFileInfo(track.filename).dir().path();
    bool newAlbum = true;
    db->lock(); /* Do not allow to execute other queries before inserting both album and track */
    if (track.album.name.isEmpty())
    {
        newAlbum = false;
        track.album.id = 0; /* Unknown album */
    }
    else if(db->getAlbumIdByNameAndTrackFilename(&albumId, track.album.name, directory))
    {
        track.album.id = albumId;
        newAlbum = false;
    }

    if (newAlbum)
    {
        db->insertNewAlbum(&(track.album));
    }

    db->insertNewTrack(&track);
    db->unlock();
}


