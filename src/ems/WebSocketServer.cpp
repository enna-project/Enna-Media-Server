#include "QtWebSockets/QWebSocketServer"
#include "QtWebSockets/QWebSocket"
#include <QtCore/QDebug>
#include "WebSocketServer.h"
#include "Player.h"

WebSocketServer::WebSocketServer(quint16 port, QObject *parent) :
        QObject(parent),
  m_pWebSocketServer(Q_NULLPTR),
  m_clients()
{
  m_pWebSocketServer = new QWebSocketServer(QStringLiteral("Chat Server"),
                                            QWebSocketServer::NonSecureMode,
                                            this);
  if (m_pWebSocketServer->listen(QHostAddress::Any, port))
  {
      qDebug() << "WebSocket server listening on port " << port;
      connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
              this, &WebSocketServer::onNewConnection);
      connect(Player::instance(), &Player::statusChanged,
              this, &WebSocketServer::broadcastStatus);
      connect(Player::instance(), &Player::playlistChanged,
              this, &WebSocketServer::broadcastPlaylist);
  }
}

WebSocketServer::~WebSocketServer()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void WebSocketServer::onNewConnection()
{
    QWebSocket *socket = m_pWebSocketServer->nextPendingConnection();

    connect(socket, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);

    JsonApi *api = new JsonApi(socket);
    connect(socket, &QWebSocket::textMessageReceived, api, &JsonApi::processMessage);

    m_clients[socket] = api;
}

void WebSocketServer::broadcastStatus(EMSPlayerStatus newStatus)
{
    foreach( JsonApi *api, m_clients.values() )
    {
        if (api)
        {
            api->sendStatus(newStatus);
        }
    }
}

void WebSocketServer::broadcastPlaylist(EMSPlaylist newPlaylist)
{
    foreach( JsonApi *api, m_clients.values() )
    {
        if (api)
        {
            api->sendPlaylist(newPlaylist);
        }
    }
}

void WebSocketServer::sendAuthRequestToLocalUI(const EMSClient client)
{
    QMapIterator<QWebSocket*, JsonApi*> client_it(m_clients);
    qDebug() << "WebSocketServer: sent the 'authentication' request for " << client.uuid;
    qDebug() << "WebSocketServer: nb connected clients = " << m_clients.size();

    // Search the websocket for the local UI,
    while (client_it.hasNext())
    {
        client_it.next();
        if (client_it.key()->peerAddress() == QHostAddress::LocalHost)
        {
            // and send the authentication request message
            client_it.value()->sendAuthRequest(client);
            return;
        }
    }
}

void WebSocketServer::processMessage(QString message)
{
   QWebSocket *client = qobject_cast<QWebSocket *>(sender());
   m_clients[client]->processMessage(message);
}

void WebSocketServer::socketDisconnected()
{
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    if (client)
    {
        delete m_clients[client];
        m_clients.remove(client);
    }
}
