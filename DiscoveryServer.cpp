#include <QNetworkInterface>

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

    qDebug() << "Read Read";

    QHostAddress sender;
    quint16 senderPort;
    m_socket->readDatagram(buffer.data(), buffer.size(),
                         &sender, &senderPort);
    /* TODO: Read the datagram, and check the UUID */
    qDebug() << "Message from: " << sender.toString();
    qDebug() << "Message port: " << senderPort;
    qDebug() << "Message: " << buffer;

    //QByteArray datagram = "EMS_DISCOVER";
    //m_udpSocket->writeDatagram(datagram.data(),datagram.size(), QHostAddress::Broadcast , BCAST_UDP_PORT);
    QHostAddress local_address;
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
        {
             local_address = address;
        }
    }
    QByteArray datagram;
    datagram.append("EMS_IP ");
    datagram.append(local_address.toString());

    m_socket->writeDatagram(datagram.data(), datagram.size(), sender, senderPort);

}

