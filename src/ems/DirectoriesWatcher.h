#ifndef DIRECTORIESWATCHER_H
#define DIRECTORIESWATCHER_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QVector>
#include <QDebug>
#include <QMutex>
#include <QTimer>
#include <QThread>

class LocalFileScanner;

class DirectoriesWatcher : public QObject
{
    Q_OBJECT

public:
    DirectoriesWatcher();
    ~DirectoriesWatcher();

    void start(LocalFileScanner *localFileScanner);

private:
    void printDebugWatchedDirectories();
    void addDirectoryInWatcher(QString directory);

    LocalFileScanner *m_localFileScanner;
    QVector<QString> m_rootDirectories;
    QFileSystemWatcher m_qtWatcher;

    // Timer to trigger the start of the localFileScanner
    QTimer *m_fileScannerTrigger;
    QMutex m_mutex;
    QThread m_localFileScannerWorker;

private slots:
    void handleDirectoryChanged(const QString &path);
    void startLocalFileScanner(void);
};

#endif // DIRECTORIESWATCHER_H
