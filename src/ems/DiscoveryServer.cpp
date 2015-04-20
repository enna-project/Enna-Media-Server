#include <QNetworkInterface>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QSettings>

#include "DiscoveryServer.h"
#include "Database.h"

/* TODO: implement a security process based on activation
 * Client send Discovery process with an UUID, EMS check if
 * this UUID is known, if it's the case, we accept the request
 * and send back the DISCOVERY answer.
 * If it's not the case, we need to find a way to display a popup
 * on the screen to ask the user to accept the connection request
 * for this new UUID.
 * How to do that ? Creating a pipe for Inter process communication
 * between EMS and the UI, only for connexion inside the same machine ?
 * Have the connection between EMS and the UI on the same device
 * through a simple TCPSocket instead through websocket ? And let the
 * websockets connexion for the UI outside machine ?
 */

DiscoveryServer::DiscoveryServer(quint16 port, QObject *parent) : QObject(parent)
{
    m_socket = new QUdpSocket(this);
    m_socket->bind(QHostAddress::AnyIPv4, port);
    qDebug() << "Udp Discovery server listening on port " << port;
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));

    // Get the local address
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
                address != QHostAddress(QHostAddress::LocalHost))
        {
            m_serverAddress = address;
        }
    }
}

DiscoveryServer::~DiscoveryServer()
{
    delete m_socket;
}

void DiscoveryServer::readyRead()
{
    QByteArray buffer;
    buffer.resize(m_socket->pendingDatagramSize());
    ClientConnectionParam senderParam;
    QString clientUuid;
    bool isLocalGUI = false;
    bool isClientAlreadyAccepted = false;
    EMSClient client;
    Database *db = Database::instance();
    QHostAddress loopbackAddr = QHostAddress::LocalHost;

    // Read data received
    m_socket->readDatagram(buffer.data(), buffer.size(),
                           &senderParam.ip, &senderParam.port);
    QJsonParseError err;
    QJsonDocument j = QJsonDocument::fromJson(buffer, &err);

    // Error reading Json
    if (err.error!= QJsonParseError::NoError)
    {
        return;
    }

    clientUuid =  j.object()["uuid"].toString();
    qDebug() << "UUID :" << clientUuid;

    db->lock();
    isClientAlreadyAccepted = db->getAuthorizedClient(clientUuid, &client);
    db->unlock();
    qDebug() << "Discovery: is client already accepted : " << isClientAlreadyAccepted;

    if (isClientAlreadyAccepted)
    {
        // Already accepted clients usecase:
        sendAcceptAnswer(senderParam);
        // Remove the client from the pending clients container
        m_pendingOrRejectedClients.remove(clientUuid);
    }
    else if (m_pendingOrRejectedClients.find(clientUuid) != m_pendingOrRejectedClients.end())
    {
        // Do not process the DISCOVERY request if client is 'pending' or
        // 'rejected'
        qDebug() << "Discovery: client auth is pending or client is rejected (uuid: "
                 << clientUuid << ")";
    }
    else
    {
        // Unknown clients usecase:

        if ((senderParam.ip == m_serverAddress) ||
            (senderParam.ip == QHostAddress::LocalHost))
        {
            isLocalGUI = true;
        }

        // Build the EMSClient data
        client.uuid = clientUuid;
        if (isLocalGUI)
            client.hostname = loopbackAddr.toString();
        else
            client.hostname = senderParam.ip.toString();

        // Local client usecase: accept the client
        if (isLocalGUI)
        {
            db->lock();
            if (!db->insertNewAuthorizedClient(&client))
            {
                qWarning() << "Error Discovery: can not insert local client in db";
            }
            db->unlock();
            sendAcceptAnswer(senderParam);
        }
        else
        {
            // Remote unknown clients usecase
            sendAuthenticationRequest(client);
        }
    }
}

void DiscoveryServer::sendDiscoveryAnswer(const ClientConnectionParam &clientParam,
                                          const QString &status)
{
    QSettings settings;
    // Create object containing the answer
    QJsonObject jobj;
    jobj["action"] = "EMS_DISCOVER";
    jobj["status"] = status;
    jobj["ip"] = m_serverAddress.toString();
    jobj["port"] = settings.value("main/websocket_port").toInt();
    qDebug() << "EMS_DISCOVER (status: " << status
             << ") Client Addr: " << clientParam.ip.toString()
             << ", port: " << settings.value("main/websocket_port").toInt();
    QJsonDocument jdoc(jobj);
    // Convert json object into datagram
    QByteArray datagram = jdoc.toJson(QJsonDocument::Compact);
    // Send the data
    m_socket->writeDatagram(datagram.data(), datagram.size(), clientParam.ip, clientParam.port);
}

void DiscoveryServer::sendAcceptAnswer(const ClientConnectionParam &clientParam)
{
    sendDiscoveryAnswer(clientParam, "accepted");
}

void DiscoveryServer::sendAuthenticationRequest(const EMSClient client)
{
    // Save the client in the 'pending' container
    m_pendingOrRejectedClients[client.uuid] = client;

    // Start the authentication with the local UI
    emit authenticationNeeded(client);
}
