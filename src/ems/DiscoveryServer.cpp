#include <QNetworkInterface>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QSettings>

#include "DiscoveryServer.h"

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
    QHostAddress sender;
    quint16 senderPort;

    // Read data received
    m_socket->readDatagram(buffer.data(), buffer.size(),
                         &sender, &senderPort);
    QJsonParseError err;
    QJsonDocument j = QJsonDocument::fromJson(buffer, &err);

    // Error reading Json
    if (err.error!= QJsonParseError::NoError)
    {
        return;
    }

    // TODO : check if UUID is accepted
    qDebug() << "UUID :" << j.object()["uuid"].toString();
    QHostAddress local_address;
    // Get the local address
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
        {
             local_address = address;
        }
    }

    QSettings settings;
    // Create object containing the answer
    QJsonObject jobj;
    jobj["action"] = "EMS_DISCOVER";
    jobj["status"] = "accepted";
    jobj["ip"] = local_address.toString();
    jobj["port"] = settings.value("main/websocket_port").toInt();
    qDebug() << "Port : " <<  settings.value("main/websocket_port").toInt();
    QJsonDocument jdoc(jobj);
    // Convert json object into datagram
    QByteArray datagram = jdoc.toJson(QJsonDocument::Compact);
    // Send the data
    m_socket->writeDatagram(datagram.data(), datagram.size(), sender, senderPort);
}

