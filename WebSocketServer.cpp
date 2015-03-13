#include "QtWebSockets/QWebSocketServer"
#include "QtWebSockets/QWebSocket"
#include <QtCore/QDebug>
#include "WebSocketServer.h"

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
