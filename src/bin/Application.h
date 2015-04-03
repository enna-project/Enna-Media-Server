#ifndef APPLICATION_H
#define APPLICATION_H

#include <QCoreApplication>
#include <QObject>
#include <QSettings>

#include "HttpServer.h"
#include "WebSocketServer.h"
#include "DiscoveryServer.h"
#include "LocalFileScanner.h"
#include "SmartmontoolsNotifier.h"

class Application : public QCoreApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);
    ~Application();

private:
    short m_websocketPort;
    short m_httpPort;
    LocalFileScanner m_scanner;
    HttpServer *m_httpServer;
    WebSocketServer *m_webSocketServer;
    DiscoveryServer *m_discoveryServer;
    SmartmontoolsNotifier *m_smartmontools;

    /* Run detached from the main event loop */
    QThread m_cdromManagerWorker;
    QThread m_localFileScannerWorker;

};

#endif // APPLICATION_H
