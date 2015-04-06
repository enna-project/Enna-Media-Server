#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <QObject>

class WebSocket : public QWebSocket
{
public:
    WebSocket();
    ~WebSocket();
};

#endif // WEBSOCKET_H
