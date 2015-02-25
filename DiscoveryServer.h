#ifndef DISCOVERYSERVER_H
#define DISCOVERYSERVER_H

#include <QObject>
#include <QUdpSocket>

#define BCAST_UDP_PORT 3131

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
    QUdpSocket *m_socket;

};

#endif // DISCOVERYSERVER_H
