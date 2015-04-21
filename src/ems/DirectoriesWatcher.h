#ifndef DIRECTORIESWATCHER_H
#define DIRECTORIESWATCHER_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QVector>
#include <QDebug>
#include <QMutex>
#include <QTimer>
#include <QThread>
#include <QSet>

class LocalFileScanner;

class DirectoriesWatcher : public QObject
{
    Q_OBJECT

public:
    DirectoriesWatcher();
    ~DirectoriesWatcher();

    void addLocation(const QString &location);
    void start();

private:
    void printDebugWatchedDirectories();
    void addDirectoryInWatcher(QString directory);
    bool needToWaitOpenedFile();

    LocalFileScanner *m_localFileScanner;
    QVector<QString> m_rootDirectories;
    QFileSystemWatcher m_qtWatcher;
    QMap<QString, qint64> m_awaited_files;

    // Timer to trigger the start of the localFileScanner
    QTimer *m_fileScannerTrigger;
    QMutex m_mutex;
    QThread m_localFileScannerWorker;

private slots:
    void handleDirectoryChanged(const QString &path);
    void startLocalFileScanner(void);
};

#endif // DIRECTORIESWATCHER_H
