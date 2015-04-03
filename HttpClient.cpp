#include <QFile>
#include "HttpClient.h"
#include "Data.h"
#include "Database.h"
#include "DefaultSettings.h"

int onMessageBeginCb(http_parser *parser)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    client->m_parseUrl.clear();
    client->m_bodyMessage.clear();
    client->m_parsingDone = false;
    client->m_hasField = false;
    client->m_hasValue = false;
    client->m_hField.clear();
    client->m_hValue.clear();
    return 0;
}

int onUrlCb(http_parser *parser, const char *at, size_t length)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);
    client->m_parseUrl.append(at, length);

    return 0;
}

int onStatusCompleteCb(http_parser *parser)
{

    return 0;
}

int onHeaderFieldCb(http_parser *parser, const char *at, size_t length)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    if (client->m_hasField && client->m_hasValue)
    {
        client->m_requestHeaders[client->m_hField.toLower()] = client->m_hValue;
        client->m_hasField = false;
        client->m_hasValue = false;
        client->m_hField.clear();
        client->m_hValue.clear();
    }

    if (!client->m_hasField)
        client->m_hasField = true;

    client->m_hField.append(at, length);

    return 0;
}

int onHeaderValueCb(http_parser *parser, const char *at, size_t length)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    if (!client->m_hasValue)
        client->m_hasValue = true;

    client->m_hValue.append(at, length);

    return 0;
}

int onHeadersCompleteCb(http_parser *parser)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    if (client->m_hasField && client->m_hasValue)
    {
        client->m_requestHeaders[client->m_hField.toLower()] = client->m_hValue;
        client->m_hasField = false;
        client->m_hasValue = false;
        client->m_hField.clear();
        client->m_hValue.clear();
    }
    return 0;
}

int onBodyCb(http_parser *parser, const char *at, size_t length)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    client->m_bodyMessage.append(at, length);


    return 0;
}

int onMessageCompleteCb(http_parser *parser)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    client->m_parsingDone = true;
    client->m_requestMethod = parser->method;

    return 0;
}


HttpClient::HttpClient(QTcpSocket *socket, QString cacheDirectory, QObject *parent) :
    QObject(parent),
    m_cacheDirectory(cacheDirectory),
    m_socket(socket)
{


    m_httpParser = (http_parser*) calloc(1, sizeof(http_parser));
    http_parser_init(m_httpParser, HTTP_REQUEST);
    m_httpParser->data = this;

    m_httpParserSettings = (http_parser_settings*) calloc(1, sizeof(http_parser_settings));
    m_httpParserSettings->on_message_begin = onMessageBeginCb;
    m_httpParserSettings->on_url = onUrlCb;
    m_httpParserSettings->on_status_complete = onStatusCompleteCb;
    m_httpParserSettings->on_header_field = onHeaderFieldCb;
    m_httpParserSettings->on_header_value = onHeaderValueCb;
    m_httpParserSettings->on_headers_complete = onHeadersCompleteCb;
    m_httpParserSettings->on_body = onBodyCb;
    m_httpParserSettings->on_message_complete = onMessageCompleteCb;

    connect(socket, &QTcpSocket::readyRead, [=]{
        parseRequest();

        qDebug() << "url : " << m_parseUrl;
        qDebug() << "headers : " << m_requestHeaders;
        QString url(m_parseUrl);
        QStringList arg = url.split('/');
        if (arg[1] == "cover" && arg[2] == "artist")
        {
            int id = arg[3].toInt();
            EMSArtist artist;
            Database::instance()->getArtistById(&artist, id);
            QFile f(m_cacheDirectory + "/" + artist.picture);
            QHash<QString, QString> headers;
            headers["Connection"] = "Close";
            headers["Content-Type"] = "image/png";
            f.open(QIODevice::ReadOnly);

            QByteArray body = f.readAll();
            int written = m_socket->write(buildHttpResponse(HTTP_200, headers, body));
            socket->flush();
            CloseConnection();
            qDebug() << "Writte " << written << " bytes";
        }

    });
}

HttpClient::~HttpClient()
{

}

void HttpClient::CloseConnection()
{
    qDebug() << "Closing connection";
    deleteLater();
}

void HttpClient::parseRequest()
{
    while (m_socket->bytesAvailable())
    {
        QByteArray data = m_socket->readAll();
        int n = http_parser_execute(m_httpParser, m_httpParserSettings, data.constData(), data.size());
        if (n != data.size())
        {
            // Errro close connection
            CloseConnection();
        }
    }
}

QByteArray HttpClient::buildHttpResponse(QString code, QHash<QString,QString> &headers, QByteArray &body)
{
    QByteArray res;
    res = code.toLatin1() + "\r\n";

    if (!headers.contains("Content-Length"))
        headers["Content-Length"] = QString("%1").arg(body.length());

    foreach(QString key, headers.keys())
    {
        res +=  key + ": " + headers[key] + "\r\n";
    }
    res += "\r\n";
    res += body;
    return res;
}
