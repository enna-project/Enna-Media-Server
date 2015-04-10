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
}

DiscoveryServer::~DiscoveryServer()
{
    delete m_socket;
}

void DiscoveryServer::readyRead()
{
    QByteArray buffer;
    buffer.resize(m_socket->pendingDatagramSize());
    ClientConnectionParam sender_param;
    QString client_uuid;
    bool isLocalGUI = false;
    bool isClientAlreadyAccepted = false;
    EMSClient client;
    Database *db = Database::instance();
    QHostAddress loopback_addr = QHostAddress::LocalHost;

    // Read data received
    m_socket->readDatagram(buffer.data(), buffer.size(),
                         &sender_param.ip, &sender_param.port);
    QJsonParseError err;
    QJsonDocument j = QJsonDocument::fromJson(buffer, &err);

    // Error reading Json
    if (err.error!= QJsonParseError::NoError)
    {
        return;
    }

    client_uuid =  j.object()["uuid"].toString();
    qDebug() << "UUID :" << client_uuid;

    db->lock();
    isClientAlreadyAccepted = db->getAuthorizedClient(client_uuid, &client);
    db->unlock();
    qDebug() << "Discovery: is client already accepted : " << isClientAlreadyAccepted;

    if (isClientAlreadyAccepted) {
        // Already accepted clients usecase:
        this->sendAcceptAnswer(sender_param);
        // Remove the client from the pending clients container
        m_pending_or_rejected_clients.remove(client_uuid);
    } else if (m_pending_or_rejected_clients.find(client_uuid) != m_pending_or_rejected_clients.end()) {
        // Do not process the DISCOVERY request if client is 'pending' or
        // 'rejected'
        qDebug() << "Discovery: client auth is pending or client is rejected (uuid: "
                 << client_uuid << ")";
    } else {
        // Unknown clients usecase:

        // Get the local address
        QHostAddress local_address;
        foreach (const QHostAddress &address, QNetworkInterface::allAddresses())
        {
            if (address.protocol() == QAbstractSocket::IPv4Protocol &&
                    address != QHostAddress(QHostAddress::LocalHost))
            {
                local_address = address;
            }
        }
        if ((sender_param.ip == local_address) ||
                (sender_param.ip == QHostAddress::LocalHost)) {
            isLocalGUI = true;
        }

        // Build the EMSClient data
        client.uuid = client_uuid;
        if (isLocalGUI) {
            client.hostname = loopback_addr.toString();
        } else {
            client.hostname = sender_param.ip.toString();
        }

        // Local client usecase: accept the client
        if (isLocalGUI) {
            db->lock();
            if (!db->insertNewAuthorizedClient(&client)) {
                qWarning() << "Error Discovery: can not insert local client in db";
            }
            db->unlock();
            this->sendAcceptAnswer(sender_param);
        }
        // Remote unknown clients usecase
        else {
            this->sendAuthenticationRequest(client);
        }
    }
}

void DiscoveryServer::sendDiscoveryAnswer(const ClientConnectionParam &client_param,
                                          const QString &status)
{
    QSettings settings;
    // Create object containing the answer
    QJsonObject jobj;
    jobj["action"] = "EMS_DISCOVER";
    jobj["status"] = status;
    jobj["ip"] = client_param.ip.toString();
    jobj["port"] = settings.value("main/websocket_port").toInt();
    qDebug() << "EMS_DISCOVER (status: " << status
             << ") Client Addr: " << client_param.ip.toString()
             << ", port: " << settings.value("main/websocket_port").toInt();
    QJsonDocument jdoc(jobj);
    // Convert json object into datagram
    QByteArray datagram = jdoc.toJson(QJsonDocument::Compact);
    // Send the data
    m_socket->writeDatagram(datagram.data(), datagram.size(), client_param.ip, client_param.port);
}

void DiscoveryServer::sendAcceptAnswer(const ClientConnectionParam &client_param)
{
    this->sendDiscoveryAnswer(client_param, "accepted");
}

void DiscoveryServer::sendAuthenticationRequest(const EMSClient client)
{
    // Save the client in the 'pending' container
    m_pending_or_rejected_clients[client.uuid] = client;

    // Start the authentication with the local UI
    emit authenticationNeeded(client);
}
