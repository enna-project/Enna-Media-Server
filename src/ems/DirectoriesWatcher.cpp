#include "DirectoriesWatcher.h"

#include <QDirIterator>
#include "LocalFileScanner.h"

#define TIME_RESERVE 5000  // in milliseconds

DirectoriesWatcher::DirectoriesWatcher()
    : QObject()
{
    m_fileScannerTrigger = new QTimer(this);
    m_fileScannerTrigger->setSingleShot(true);

    connect(m_fileScannerTrigger, &QTimer::timeout,
            this, &DirectoriesWatcher::startLocalFileScanner);
}

DirectoriesWatcher::~DirectoriesWatcher()
{
    if (m_fileScannerTrigger)
    {
        m_fileScannerTrigger->stop();
        delete m_fileScannerTrigger;
    }

    // Stop the current thread
    m_localFileScannerWorker.quit();
    m_localFileScannerWorker.wait();

    disconnect(m_fileScannerTrigger, &QTimer::timeout,
               this, &DirectoriesWatcher::startLocalFileScanner);
    disconnect(&m_qtWatcher, &QFileSystemWatcher::directoryChanged,
               this, &DirectoriesWatcher::handleDirectoryChanged);
    disconnect(&m_localFileScannerWorker, &QThread::started,
               m_localFileScanner, &LocalFileScanner::startScan);
    disconnect(&m_localFileScannerWorker, &QThread::finished,
               m_localFileScanner, &LocalFileScanner::stopScan);
}

void DirectoriesWatcher::start(LocalFileScanner *localFileScanner)
{
    if (!localFileScanner)
    {
        qCritical() << "DirectoriesWatcher: locaFileScanner is null";
        return;
    }

    connect(&m_qtWatcher, &QFileSystemWatcher::directoryChanged,
            this, &DirectoriesWatcher::handleDirectoryChanged);

    m_localFileScanner = localFileScanner;

    // Initialize the localFileScanner thread
    m_localFileScanner->moveToThread(&m_localFileScannerWorker);
    connect(&m_localFileScannerWorker, &QThread::started,
            m_localFileScanner, &LocalFileScanner::startScan);
    connect(&m_localFileScannerWorker, &QThread::finished,
            m_localFileScanner, &LocalFileScanner::stopScan);

    m_rootDirectories = m_localFileScanner->getLocations();
    foreach (QString loc, m_rootDirectories)
    {
        qDebug() << "DirectoriesWatcher: root_directory: " << loc;
    }

    // Find all directories and subdirectories
    QVector<QString> allDirectories;
    foreach (QString rootDirectory, m_rootDirectories)
    {
        allDirectories.append(rootDirectory);
        QDirIterator iterator(rootDirectory, QDirIterator::Subdirectories);
        while (iterator.hasNext())
        {
            iterator.next();
            if (iterator.fileInfo().isDir())
            {
                allDirectories.append(iterator.filePath());
            }
        }
    }

    qWarning() << "DirectoriesWatcher: nb directories to watch: " << allDirectories.size();

    // Add all the directories and subdirectories in the Qt watcher
    foreach(QString dir, allDirectories)
    {
        this->addDirectoryInWatcher(dir);
    }
    this->printDebugWatchedDirectories();
}

void DirectoriesWatcher::handleDirectoryChanged(const QString &path)
{
    m_mutex.lock();
    {
        qDebug() << "DirectoriesWatcher: handle the directory 'changed' signal :" << path;

        // 1- Update the watched directory list

        // Get the already watched directory list
        QStringList alreadyWatched  = m_qtWatcher.directories();

        // Add in the Qt watcher all the subdirectories of the directory that was modified
        QDirIterator iterator(path, QDirIterator::Subdirectories);
        while (iterator.hasNext())
        {
            iterator.next();
            if (iterator.fileInfo().isDir() && (!alreadyWatched.contains(iterator.filePath())))
            {
                this->addDirectoryInWatcher(iterator.filePath());
            }
        }
        this->printDebugWatchedDirectories();

        // 2-Update the timer
        if (m_fileScannerTrigger->isActive())
        {
            m_fileScannerTrigger->setInterval(TIME_RESERVE);
        }
        else
        {
            m_fileScannerTrigger->setInterval(TIME_RESERVE);
            m_fileScannerTrigger->start();
        }
    }
    m_mutex.unlock();
}

void DirectoriesWatcher::printDebugWatchedDirectories()
{
    // Used to debug only:
    /*
    QStringList allWatchedDirectories = m_qtWatcher.directories();

    qDebug() << "DirectoriesWatcher:: all the watched directories: "
             << allWatchedDirectories.size() << "dir";
    foreach(QString watchedDirectory, allWatchedDirectories)
    {
        qDebug() << "DirectoriesWatcher:: watched dir: " << watchedDirectory;
    }
    */
}

void DirectoriesWatcher::addDirectoryInWatcher(QString directory)
{
    if ((!directory.contains("/.")) && (!directory.contains("/..")))
    {
        if (!m_qtWatcher.addPath(directory))
            qCritical() << "DirectoriesWatcher: access failure or max nb watched files is reached: "
                        << directory;
    }
}

void DirectoriesWatcher::startLocalFileScanner()
{
    qDebug() << "DirectoriesWatcher: " << TIME_RESERVE / 1000 << " seconds after the last directory change";
    if (m_localFileScanner)
    {
        if (m_localFileScanner->isScanActive())
        {
            qDebug() << "DirectoriesWatcher: the LocalFileScanner is already running";

            // Restart the timer: rerun the LocalFileScanner in TIME_RESERVE milliseconds
            m_mutex.lock();
            {
                m_fileScannerTrigger->setInterval(TIME_RESERVE);
                m_fileScannerTrigger->start();
            }
            m_mutex.unlock();
        }
        else
        {
            // Stop the current thread
            m_localFileScannerWorker.quit();
            m_localFileScannerWorker.wait();

            // Start a new one
            qDebug() << "DirectoriesWatcher: start the LocalFileScanner";
            m_localFileScannerWorker.start();
        }
    }
}
