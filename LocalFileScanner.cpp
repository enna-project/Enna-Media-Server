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
}

LocalFileScanner::~LocalFileScanner()
{
}

void LocalFileScanner::startScan()
{
    if (scanActive)
    {
        return;
    }
    scanActive = true;
    startTime = QDateTime::currentDateTime().toTime_t();

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
    qDebug() << "Scan finished.";

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

    track.filename = filename;
    track.sha1 = sha1;
    track.lastscan = startTime;

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

    /* 2) For unknown files, look for metadata inside the file */
    QString extension = QFileInfo(filename).suffix().toLower();
    track.format = extension;
    QVector<MetadataPlugin*> pluginsFileFormat = MetadataManager::instance()->getAvailablePlugins(extension);
    foreach (MetadataPlugin* pluginFileFormat, pluginsFileFormat)
    {
        pluginFileFormat->lock();
        pluginFileFormat->update(&track);
        pluginFileFormat->unlock();
    }

    /* 3) Ask online database to get more data (like cover, picture, etc.) */
    //TODO

    /* 4) Insert new track in the database */
    unsigned long long albumId;
    QString directory = QFileInfo(filename).dir().path();
    bool newAlbum = true;
    db->lock(); /* Do not allow to execute other queries before inserting both album and track */
    if (!track.album.name.isEmpty())
    {
        if(db->getAlbumIdByNameAndTrackFilename(&albumId, track.album.name, directory))
        {
            track.album.id = albumId;
            newAlbum = false;
        }
    }
    if (newAlbum)
    {
        db->insertNewAlbum(&(track.album));
    }
    db->insertNewTrack(&track);
    db->unlock();
}
