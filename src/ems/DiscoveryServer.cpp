#include <QNetworkInterface>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QSettings>

#include "DiscoveryServer.h"
#include "Data.h"
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
    bool isLocalGUI = false;
    bool isClientAlreadyAccepted = false;
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

    qDebug() << "UUID :" << j.object()["uuid"].toString();
    QHostAddress local_address;

    // Get the local address
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
    EMSClient client;
    client.uuid = j.object()["uuid"].toString();
    if (isLocalGUI) {
        client.hostname = loopback_addr.toString();
    } else {
        client.hostname = sender_param.ip.toString();
    }

    isClientAlreadyAccepted = db->getAuthorizedClient(client.uuid, &client);
    qDebug() << "Database: is client already accepted : " << isClientAlreadyAccepted;

    // Local client usecase: accept the client
    if (!isClientAlreadyAccepted && isLocalGUI) {
        db->insertNewAuthorizedClient(&client);
        this->sendAcceptAnswer(&client, sender_param);
    }
    // Already accepted clients usecase
    else if (isClientAlreadyAccepted) {
        this->sendAcceptAnswer(&client, sender_param);
    }
    // Other unknown clients usecase
    else{
        this->sendAuthenticationRequest(&client, sender_param);
    }
}

void DiscoveryServer::sendAcceptAnswer(const EMSClient * const client,
                                       const ClientConnectionParam &client_param)
{
    QSettings settings;
    // Create object containing the answer
    QJsonObject jobj;
    jobj["action"] = "EMS_DISCOVER";
    jobj["status"] = "accepted";
    jobj["ip"] = client->hostname;
    jobj["port"] = settings.value("main/websocket_port").toInt();
    qDebug() << "Client Address: " << client->hostname
             << "Port:    " << settings.value("main/websocket_port").toInt();
    QJsonDocument jdoc(jobj);
    // Convert json object into datagram
    QByteArray datagram = jdoc.toJson(QJsonDocument::Compact);
    // Send the data
    m_socket->writeDatagram(datagram.data(), datagram.size(), client_param.ip, client_param.port);
}

void DiscoveryServer::sendAuthenticationRequest(const EMSClient * const client,
                                                const ClientConnectionParam &client_param)
{
    Q_UNUSED(client);
    Q_UNUSED(client_param);
}
