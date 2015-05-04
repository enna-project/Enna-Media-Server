#ifndef APPLICATION_H
#define APPLICATION_H

#include <QCoreApplication>
#include <QObject>
#include <QSettings>

#include "HttpServer.h"
#include "WebSocketServer.h"
#include "DiscoveryServer.h"
#include "SmartmontoolsNotifier.h"
#include "SoundCardManager.h"
#include "DirectoriesWatcher.h"

class Application : public QCoreApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);
    ~Application();

private:
    short m_websocketPort;
    short m_httpPort;
    HttpServer *m_httpServer;
    WebSocketServer *m_webSocketServer;
    DiscoveryServer *m_discoveryServer;
    SmartmontoolsNotifier *m_smartmontools;
    SoundCardManager *m_soundCardManager;
    DirectoriesWatcher m_directoriesWatcher;

    /* Run detached from the main event loop */
    QThread m_cdromManagerWorker;
    QThread m_metadataManagerWorker;
};

#endif // APPLICATION_H
