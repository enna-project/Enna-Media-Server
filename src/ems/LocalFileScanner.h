#ifndef SCANNER_H
#define SCANNER_H

#include <QObject>
#include <QThread>
#include "Database.h"
#include "DirectoryWorker.h"

class LocalFileScanner : public QObject
{
    Q_OBJECT
public:
    explicit LocalFileScanner(QObject *parent = 0);
    ~LocalFileScanner();

    void locationAdd(const QString &location);

private:
    QVector<QString> locations;
    QVector<DirectoryWorker*> workers;
    QVector<EMSTrack*> tracks;
    QString supportedFormat;
    bool scanActive;
    unsigned long long startTime;
    QTime measureTime;

    void scanEnd();

signals:
    void trackNeedUpdate(EMSTrack *track, QStringList capabilities);

public slots:
    void fileFound(QString filename, QString sha1);
    void trackUpdated(EMSTrack *track, bool complete);
    void workerFinished(DirectoryWorker* worker);
    void startScan();
    void stopScan();
};

#endif // SCANNER_H
