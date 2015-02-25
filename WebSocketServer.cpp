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
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketServer::processMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);

    m_clients << pSocket;
}

void WebSocketServer::processMessage(QString message)
{

    /* TODO: implement Protocol here */
    /* Implement protocol in EmsProtocol.cpp ? */

    /* Protocol is JSON based */
    /* Example of json objects sent from EMS to clients : */
    /* {
     *     message: EMS_PLAYING_STATE
     *     player_state: "playing",
     *     artist: "David Bowie",
     *     album: "Space Odity",
     *     track: "Life on Mars",
     *     length: 205
     * }
     */


    QWebSocket *pSender = qobject_cast<QWebSocket *>(sender());
    Q_FOREACH (QWebSocket *pClient, m_clients)
    {
        if (pClient != pSender) //don't echo message back to sender
        {
            pClient->sendTextMessage(message);
        }
    }
}

void WebSocketServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient)
    {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}
