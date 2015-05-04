#include "Application.h"
#include "DefaultSettings.h"
#include "HttpServer.h"
#include "Database.h"
#include "Player.h"
#include "SmartmontoolsNotifier.h"
#include "CdromManager.h"
#include "MetadataManager.h"
#include "SoundCardManager.h"

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

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;

    /* Read and save value for websocket port */
    EMS_LOAD_SETTINGS(m_websocketPort, "main/websocket_port", EMS_WEBSOCKET_PORT, Int);
    /* Read and save value for http port */
    EMS_LOAD_SETTINGS(m_httpPort, "main/http_port", EMS_HTTP_PORT, Int);

    QString locations;
    EMS_LOAD_SETTINGS(locations, "main/locations",
                      QStandardPaths::standardLocations(QStandardPaths::MusicLocation)[0],
            String);

    QString cacheDirPath;
    EMS_LOAD_SETTINGS(cacheDirPath, "main/cache_directory",
                      QStandardPaths::standardLocations(QStandardPaths::CacheLocation)[0], String);

    /* Make sure the path exists */
    QDir().mkpath(locations);
    QDir().mkpath(cacheDirPath);

    /* Add online database plugins */
    MetadataManager::instance()->registerAllPlugins();
    MetadataManager::instance()->moveToThread(&m_metadataManagerWorker);
    m_metadataManagerWorker.start();

    /* Sound card manager */
    m_soundCardManager = new SoundCardManager(this);

    /* Open Database */
    Database *db = Database::instance();
    db->lock();
    db->open();
    db->unlock();

    /* Start the player */
    Player::instance()->start();

    /* Create Web Server */
    m_httpServer = new HttpServer(this);
    m_httpServer->start(m_httpPort);

    /* Start the CDROM manager */
    CdromManager::instance()->moveToThread(&m_cdromManagerWorker);
    connect(&m_cdromManagerWorker, &QThread::started, CdromManager::instance(), &CdromManager::startMonitor);
    connect(&m_cdromManagerWorker, &QThread::finished, CdromManager::instance(), &CdromManager::stopMonitor);
    m_cdromManagerWorker.start();

    /* Create Websocket server */
    m_webSocketServer = new WebSocketServer(m_websocketPort, this);

    /* Create Discovery Server */
    m_discoveryServer = new DiscoveryServer(BCAST_UDP_PORT, this);
    connect(m_discoveryServer, &DiscoveryServer::authenticationNeeded,
            m_webSocketServer, &WebSocketServer::sendAuthRequestToLocalUI);

    m_smartmontools = new SmartmontoolsNotifier(this);

    /* Scan locations to perform a database update */
    m_directoriesWatcher.addLocation(locations);
    m_directoriesWatcher.start();
}

Application::~Application()
{
    /* Stop the CDROM manager */
    m_cdromManagerWorker.quit();
    m_cdromManagerWorker.wait(100);

    /* Stop the metadata manager */
    m_metadataManagerWorker.quit();
    m_metadataManagerWorker.wait(100);

    /* Stop the player thread */
    Player::instance()->kill();
    Player::instance()->wait(1000);

    /* Close properly the database */
    Database::instance()->close();

    delete m_soundCardManager;
}
