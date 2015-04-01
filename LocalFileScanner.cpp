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

    for (int i=0; i<locations.size(); i++)
    {
        QString location = locations.at(i);
        // Create a new qthread
        QThread *directoryThread = new QThread;
        // Create the DirectoryWorker object : it scan directory recusively in a thread
        // And send fileFound() signal when a new file is found
        DirectoryWorker* dirWorker = new DirectoryWorker(location, "*.flac, *.mp3, *.wav");
        workers.append(dirWorker);
        // Move object to specific thread
        dirWorker->moveToThread(directoryThread);
        // Call fileFound slot when a new file is found
        connect(dirWorker, SIGNAL(fileFound(QString, QByteArray)), this, SLOT(fileFound(QString, QByteArray)), Qt::DirectConnection);
        connect(dirWorker, SIGNAL(finished(DirectoryWorker*)), this, SLOT(workerFinished(DirectoryWorker*)));
        // Start the thread
        directoryThread->start();
    }
}

void LocalFileScanner::workerFinished(DirectoryWorker* worker)
{
    int workerId = -1;

    /* Find the thread in the list of workers */
    for (unsigned int i=0; i<workers.size(); i++)
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
    //TODO
    scanActive = false;
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
    locations.append(location);
}

void LocalFileScanner::fileFound(QString filename, QByteArray sha1)
{    
    EMSTrack track;
    Database *db = Database::instance();

    track.filename = filename;
    track.sha1 = QString(sha1);
    /* 1) Search existing sha1 in the database */
    unsigned long long trackID;
    if(db->getTrackIdBySha1(&trackID, track.sha1))
    {
        /* Do nothing as we assume the metadata have correctly been seeked */
        //TODO: update timestamp
        return;
    }

    /* 2) For unknown files, look for metadata inside the file */
    QString extension = QFileInfo(filename).suffix();
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
        db->lock();
        db->insertNewAlbum(&(track.album));
        db->unlock();
    }
    db->lock();
    db->insertNewTrack(&track);
    db->unlock();
}
