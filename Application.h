#ifndef APPLICATION_H
#define APPLICATION_H

#include <QCoreApplication>
#include <QObject>
#include <QSettings>

#include "WebSocketServer.h"
#include "DiscoveryServer.h"
#include "Scanner.h"

class Application : public QCoreApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);
    ~Application();

private:
    short m_websocketPort;
    Scanner m_scanner;
    WebSocketServer *m_webSocketServer;
    DiscoveryServer *m_discoveryServer;
};

#endif // APPLICATION_H
