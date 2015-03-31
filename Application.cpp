#include "Application.h"
#include "DefaultSettings.h"
#include "Database.h"
#include "Player.h"
#include "SmartmontoolsNotifier.h"
#include "CdromManager.h"
#include "OnlineDBPluginManager.h"

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

    /* Add online database plugins */
    OnlineDBPluginManager::instance()->registerAllPlugins();

    /* Open Database */
    Database::instance()->open();

    /* Start the player */
    Player::instance()->start();

    /* Start the CDROM manager */
    CdromManager::instance()->moveToThread(&m_cdromManagerWorker);
    connect(&m_cdromManagerWorker, &QThread::started, CdromManager::instance(), &CdromManager::startMonitor);
    connect(&m_cdromManagerWorker, &QThread::finished, CdromManager::instance(), &CdromManager::stopMonitor);
    m_cdromManagerWorker.start();

    /* Create Websocket server */
    m_webSocketServer = new WebSocketServer(m_websocketPort, this);
    /* Create Discovery Server */
    m_discoveryServer = new DiscoveryServer(BCAST_UDP_PORT, this);
    m_smartmontools = new SmartmontoolsNotifier(this);
}

Application::~Application()
{
    /* Stop the CDROM manager */
    m_cdromManagerWorker.quit();
    m_cdromManagerWorker.wait();

    /* Stop the player thread */
    Player::instance()->kill();
    Player::instance()->wait(1000);

    /* Close properly the database */
    Database::instance()->close();

}

