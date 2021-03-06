#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QList>
#include <QByteArray>

#include <JsonApi.h>

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

class WebSocketServer : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketServer(quint16 port, QObject *parent = 0);
    ~WebSocketServer();

public slots:
    void sendAuthRequestToLocalUI(const EMSClient client);

private Q_SLOTS:
    void onNewConnection();
    void processMessage(QString message);
    void socketDisconnected();

    void broadcastPlaylist(EMSPlaylist newPlaylist);
    void broadcastStatus(EMSPlayerStatus newStatus);
    void broadcastRipProgress(EMSRipProgress ripProgress);
    void broadcastMenuChange(EMSCdrom cdromChanged);
    void broadcastWifiConnected();
    void broadcastEthConnected();
    void broadcastWifiList();

private:
    QWebSocketServer *m_pWebSocketServer;
    QMap<QWebSocket*, JsonApi *> m_clients;
};

#endif // WEBSOCKETSERVER_H
