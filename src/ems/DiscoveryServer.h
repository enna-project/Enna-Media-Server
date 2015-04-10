#ifndef DISCOVERYSERVER_H
#define DISCOVERYSERVER_H

#include <QObject>
#include <QUdpSocket>
#include "Data.h"

#define BCAST_UDP_PORT 3131

class ClientConnectionParam
{
public:
    QHostAddress ip;
    quint16      port;
};

class DiscoveryServer : public QObject
{
    Q_OBJECT
public:
    explicit DiscoveryServer(quint16 port, QObject *parent = 0);
    ~DiscoveryServer();

signals:
    void authenticationNeeded(const EMSClient client);

public slots:
    void readyRead();

private:
    void sendDiscoveryAnswer(const ClientConnectionParam &client_param,
                             const QString &status);

    void sendAcceptAnswer(const ClientConnectionParam &client_param);

    void sendAuthenticationRequest(const EMSClient client);

    QUdpSocket *m_socket;
    QMap<QString, EMSClient> m_pending_or_rejected_clients;
    QHostAddress m_server_address;
};

#endif // DISCOVERYSERVER_H
