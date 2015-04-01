#include "Application.h"
#include "DefaultSettings.h"
#include "Database.h"
#include "Player.h"
#include "SmartmontoolsNotifier.h"
#include "CdromManager.h"
#include "OnlineDBPluginManager.h"
#include "Database.h"
#include "Player.h"
#include "HttpServer.h"
#include "MetadataManager.h"

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
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    /* Read value for websocket port */
    if (settings.contains("main/websocket_port"))
        m_websocketPort = settings.value("main/websocket_port").toInt();
    /* Save websocket back if it's not found in initial config file */
    else
    {
        m_websocketPort = EMS_WEBSOCKET_PORT;
        settings.setValue("main/websocket_port", m_websocketPort);
    }


    /* Read value for websocket port */
    if (settings.contains("main/http_port"))
        m_httpPort = settings.value("main/http_port").toInt();
    /* Save websocket back if it's not found in initial config file */
    else
    {
        m_httpPort = EMS_HTTP_PORT;
        settings.setValue("main/http_port", m_httpPort);
    }

    /* Read locations path in config and add them to the scanner object */
    int size = settings.beginReadArray("locations");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        m_scanner.locationAdd(settings.value("path").toString());
    }
    settings.endArray();
    if (size == 0) /* If there is no location in the settings, use the default */
    {
        QStringList locations = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
        for (int i=0; i<locations.size(); i++)
        {
            m_scanner.locationAdd(locations.at(i));
        }
    }
    /* Re-scan locations to perform a database update */
    m_scanner.moveToThread(&m_localFileScannerWorker);
    connect(&m_localFileScannerWorker, &QThread::started, &m_scanner, &LocalFileScanner::startScan);
    connect(&m_localFileScannerWorker, &QThread::finished, &m_scanner, &LocalFileScanner::stopScan);
    m_localFileScannerWorker.start();

    /* Add online database plugins */
    MetadataManager::instance()->registerAllPlugins();

    /* Open Database */
    Database *db = Database::instance();
    db->lock();
    db->open();
    db->unlock();


    /* Start the CDROM manager */
    CdromManager::instance()->moveToThread(&m_cdromManagerWorker);
    connect(&m_cdromManagerWorker, &QThread::started, CdromManager::instance(), &CdromManager::startMonitor);
    connect(&m_cdromManagerWorker, &QThread::finished, CdromManager::instance(), &CdromManager::stopMonitor);
    m_cdromManagerWorker.start();
    /* Start the player */
    Player::instance()->start();
    /* Create Web Server */
    m_httpServer = new HttpServer(this);
    m_httpServer->start(m_httpPort);
    /* Create Websocket server */
    m_webSocketServer = new WebSocketServer(m_websocketPort, this);

    /* Create Discovery Server */
    m_discoveryServer = new DiscoveryServer(BCAST_UDP_PORT, this);
    m_smartmontools = new SmartmontoolsNotifier(this);

    m_smartmontools = new SmartmontoolsNotifier(this);
}

Application::~Application()
{
    /* Stop the CDROM manager */
    m_cdromManagerWorker.quit();
    m_cdromManagerWorker.wait(100);

    /* Stop the scanner */
    m_localFileScannerWorker.quit();
    m_localFileScannerWorker.wait(100);

    /* Stop the player thread */
    Player::instance()->kill();
    Player::instance()->wait(1000);

    /* Close properly the database */
    Database::instance()->close();

}

