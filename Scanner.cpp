#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSettings>
#include <QThread>

#include "Scanner.h"
#include "DirectoryWorker.h"
#include "Grabbers.h"

Scanner::Scanner(QObject *parent) : QObject(parent)
{
}

Scanner::~Scanner()
{
}

void Scanner::startScan()
{
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

void Scanner::workerFinished(DirectoryWorker* worker)
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

void Scanner::scanEnd()
{
    //TODO
}

void Scanner::locationAdd(const QString &location)
{
    locations.append(location);
}

void Scanner::fileFound(QString filename, QByteArray sha1)
{    
    //TODO
}
