#ifndef SCANNER_H
#define SCANNER_H

#include <QObject>
#include <QThread>
#include "Database.h"
#include "DirectoryWorker.h"
#include "DirectoriesWatcher.h"

class LocalFileScanner : public QObject
{
    Q_OBJECT
public:
    explicit LocalFileScanner(QObject *parent = 0);
    ~LocalFileScanner();

    void locationAdd(const QString &location);
    QVector<QString> getLocations() const;

    void startDirectoriesWatcher();
    bool isScanActive();

private:
    QVector<QString> m_locations;
    QVector<DirectoryWorker*> m_workers;
    QString m_supportedFormat;
    bool m_scanActive;
    unsigned long long m_startTime;
    QTime m_measureTime;
    DirectoriesWatcher m_directoriesWatcher;

    void scanEnd();

signals:
    void trackNeedUpdate(EMSTrack track, QStringList capabilities);

public slots:
    void fileFound(QString filename, QString sha1);
    void trackUpdated(EMSTrack track, bool complete);
    void workerFinished(DirectoryWorker* worker);
    void startScan();
    void stopScan();
};

#endif // SCANNER_H
