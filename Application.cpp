#include "Application.h"

/* Define WEB for websocket port FIXME: move to a globel .h file*/
#define EMS_WEBSOCKET_PORT 7337


/*
 * Derived form QCoreApplication
 * Set Organization name, domain and application name
 *
 * Create Websocket server for API
 * Create DiscoveryServer for autodetection of clients
 *
 */

Application::Application(int & argc, char ** argv) :
    QCoreApplication(argc, argv)
{  
    QCoreApplication::setOrganizationName("Enna");
    QCoreApplication::setOrganizationDomain("enna.me");
    QCoreApplication::setApplicationName("EnnaMediaServer");
    QSettings settings;

    /* Read value for websocket port */
    m_websocketPort = settings.value("main/websocket_port", EMS_WEBSOCKET_PORT).toInt();
    /* Save websocket back if it's not found in initial config file */
    settings.setValue("main/websocket_port", m_websocketPort);

    /* Read locations path in config and add them to the scanner object */
    int size = settings.beginReadArray("locations");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        m_scanner.locationAdd(settings.value("path").toString());
    }
    settings.endArray();

    /* Create Websocket server */
    m_webSocketServer = new WebSocketServer(m_websocketPort, this);
    /* Create Discovery Server */
    m_discoveryServer = new DiscoveryServer(BCAST_UDP_PORT, this);

}

Application::~Application()
{

}

