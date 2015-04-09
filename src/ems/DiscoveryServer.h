#ifndef DISCOVERYSERVER_H
#define DISCOVERYSERVER_H

#include <QObject>
#include <QUdpSocket>

#define BCAST_UDP_PORT 3131

typedef struct client_connection_param {
    QHostAddress ip;
    quint16      port;
} ClientConnectionParam;


class EMSClient;

class DiscoveryServer : public QObject
{
    Q_OBJECT
public:
    explicit DiscoveryServer(quint16 port, QObject *parent = 0);
    ~DiscoveryServer();

signals:

public slots:
    void readyRead();

private:
    void sendDiscoveryAnswer(const ClientConnectionParam &client_param,
                             const QString &status);

    void sendAcceptAnswer(const ClientConnectionParam &client_param);

    void sendAuthenticationRequest(const QString &uuid,
                                   const ClientConnectionParam &client_param);

    QUdpSocket *m_socket;
    QMap<QString, ClientConnectionParam> m_pending_clients;

};

#endif // DISCOVERYSERVER_H
