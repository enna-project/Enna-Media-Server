#include <QThread>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSettings>

#include "Scanner.h"
#include "DirectoryWorker.h"
#include "Grabbers.h"

Scanner::Scanner(QObject *parent) : QObject(parent)
{
    db = Database::instance();
    grabbersThread = new QThread;
    grabbers = new Grabbers();
    grabbers->moveToThread(grabbersThread);
    connect(this, SIGNAL(grab(QString, QByteArray)), grabbers, SLOT(grab(QString, QByteArray)));
    connect(grabbers, SIGNAL(metadataFound(QByteArray)), this, SLOT(metadataFound(QByteArray)));
    grabbersThread->start();
}

Scanner::~Scanner()
{
    if (grabbersThread)
        delete grabbersThread;
}

void Scanner::locationAdd(const QString &location)
{
    // Create a new qthread
    QThread *directoryThread = new QThread;
    // Create the DirectoryWorker object : it scan directory recusively in a thread
    // And send fileFound() signal when a new file is found
    DirectoryWorker* dirWorker = new DirectoryWorker(location, "*.flac, *.mp3, *.wav");
    // Move object to specific thread
    dirWorker->moveToThread(directoryThread);
    // Start scan as soon as the thread start
    connect(directoryThread, SIGNAL(started()), dirWorker, SLOT(process()));
    // Quit thread when directoryWorker send finished signal
    connect(dirWorker, SIGNAL(finished()), directoryThread, SLOT(quit()));
    // Delete dirWorker properly
    connect(dirWorker, SIGNAL(finished()), dirWorker, SLOT(deleteLater()));
    // Call filesProcessed slot when dirworker has finished
    connect(dirWorker, SIGNAL(finished()), this, SLOT(filesProcessed()));
    // Delete thread properly
    connect(directoryThread, SIGNAL(finished()), directoryThread, SLOT(deleteLater()));
    // Call fileFound slot when a new file is found
    connect(dirWorker, SIGNAL(fileFound(QString, QByteArray)), this, SLOT(fileFound(QString, QByteArray)), Qt::DirectConnection);

    //Start the thread
    directoryThread->start();
    qDebug() << Q_FUNC_INFO << QThread::currentThreadId();
}

void Scanner::fileFound(QString filename, QByteArray sha1)
{    
    qDebug() << Q_FUNC_INFO << QThread::currentThreadId();
    // Insert a new file in the db
    //db.fileInsert(filename, sha1);
    emit grab(filename, sha1);
}

void Scanner::filesProcessed()
{
    qDebug() << Q_FUNC_INFO << QThread::currentThreadId() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
    // End db transaction (commit)
    //db.endTransaction();
#if 0
    QJsonDocument doc(db.albumsListGet());
    qDebug() << "Albums list found : " << doc.toJson();

    QJsonDocument doc2(db.artistsListGet());
    qDebug() << "Artists list found : " << doc2.toJson();

    QJsonDocument doc3(db.songsListGet());
    qDebug() << "Songs list found : " << doc3.toJson();
#endif
}

void Scanner::metadataFound(QByteArray sha1)
{
    qDebug() << "Metadata for sha1 :" << sha1.toHex();
    QJsonObject *metadata = grabbers->metadataGet(sha1);
    QJsonDocument doc(*metadata);
    qDebug() << "Metadatas found : " << doc.toJson();

    //db.albumInsert(metadata->value("album").toString(), metadata);
    //db.artistInsert(metadata->value("artist").toString(), metadata);
    //db.songInsert(metadata->value("title").toString(), metadata);
    //db.artistAlbumInsert(metadata->value("artist").toString(),
    //                     metadata->value("album").toString(), metadata);
    delete metadata;
}


