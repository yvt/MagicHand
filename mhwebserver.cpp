#include "mhwebserver.h"
#include <QTcpSocket>
#include <QRegExp>
#include <QCryptographicHash>
#include <QStringBuilder>
#include <QDebug>

class MHWebServer::Connection
{
    MHWebServer& server;
    std::unique_ptr<QTcpSocket> socket;
    QRegExp regex_request {"(OPTIONS|GET|HEAD|POST|PUT|DELETE|TRACE|CONNECT)"
                          " (\\/.*) HTTP\\/(\\d+)\\.(\\d+)"};
    QRegExp regex_header {"([^:]+)[:] *(|[^ ].*[^ ]|[^ ]) *"};

    QString GetStatusText(int code)
    {
        switch (code) {
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 505: return "HTTP Version not supported";
        default: return "Unknown";
        }
    }

    void returnHTTPError(int code)
    {
        // TODO: returnHTTPError

        QByteArray headers;
        headers.append(QString("HTTP/1.1 %1 %2\r\n")
                       .arg(QString::number(code), GetStatusText(code)));
        headers.append("Content-Length: 0\r\n");
        headers.append("\r\n");

        socket->write(headers);

        socket->disconnectFromHost();
        socket->close();
    }

    void onDataReceived()
    {
        try {
            // read header
            QByteArray data = socket->readAll();
            QStringList headerLines = QString(data).split("\r\n");
            if (!regex_request.exactMatch(headerLines[0])) {
                // malformed header!
                throw MHWebServerHTTPError(400);
            }

            QString method = regex_request.cap(1);
            QString uri = regex_request.cap(2);
            int httpMajorVer = regex_request.cap(3).toInt();
            int httpMinorVer = regex_request.cap(4).toInt();
            if ((httpMajorVer == 1 && httpMinorVer > 1) ||
                (httpMajorVer > 1)) {
                // unsupported version
                throw MHWebServerHTTPError(505);
            }
            //qDebug() << method << uri;

            if (method != "GET") {
                // not supported method
                throw MHWebServerHTTPError(501);
            }

            QMap<QString, QString> headers;
            for (int i = 1; i < headerLines.size(); i++) {
                if (!regex_header.exactMatch(headerLines[i])) {
                    // invalid header
                    continue;
                }
                headers.insert(regex_header.cap(1), regex_header.cap(2));
            }

            // websocket?
            if (headers.contains("Sec-WebSocket-Key")) {
                QString unhashed = headers["Sec-WebSocket-Key"];
                unhashed += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                QByteArray hash = QCryptographicHash::hash(unhashed.toUtf8(),
                                                           QCryptographicHash::Sha1);
                QString hashStr = hash.toBase64();
                std::unique_ptr<MHWebSocket> sock(new MHWebSocket(socket.get()));
                WebSocketHandler *handler = server.handleWebSocket(uri, headers, sock.get());
                if (!handler) {
                    sock.release(); // ownership was given
                    throw MHWebServerHTTPError(500);
                }
                sock.release();

                QString proto = handler->getProtocol();

                // now ready to start WebSocket.
                QByteArray reply;
                reply.append("HTTP/1.1 101 Switching Protocols\r\n");
                reply.append("Upgrade: websocket\r\n");
                reply.append("Connection: Upgrade\r\n");
                reply.append("Sec-WebSocket-Accept: ");
                reply.append(hashStr);
                reply.append("\r\n");
                reply.append("Sec-WebSocket-Protocol: ");
                reply.append(proto);
                reply.append("\r\n\r\n");
                socket->write(reply);
                socket->flush();
                //qDebug() << "headers:" << headers;
                //qDebug() << "replying:" << reply;

                handler->start();
                socket.release();
                return;
            }

            // normal GET
            Contents contents = server.handleGet(uri, headers);
            QByteArray reply;
            reply.append("HTTP/1.1 300 OK\r\n");
            reply.append("Content-Type: ");
            reply.append(contents.contentType);
            reply.append("\r\n");
            reply.append(QString("Content-Length: %1\r\n").arg(contents.data.size()));
            reply.append("\r\n");
            socket->write(reply);
            socket->write(contents.data);

            socket->disconnectFromHost();
            socket->close();

        } catch (const MHWebServerHTTPError& httpError) {
            returnHTTPError(httpError.status());
        }
    }

public:
    Connection(MHWebServer& server, QTcpSocket *socket):
    server(server),
    socket(socket)
    {
        connect(socket, &QTcpSocket::readyRead, [&]()
        {
            if (!this->socket) return;
            onDataReceived();
        });
        connect(socket, &QTcpSocket::disconnected, [&]()
        {
            if (!this->socket) return;
            this->socket->deleteLater();
            this->socket.release();
            delete this;
        });
    }
    ~Connection()
    {
        if (socket) socket.release()->deleteLater();
    }
};

MHWebServer::MHWebServer(QObject *parent) :
    QTcpServer(parent)
{
    connect(this, &MHWebServer::newConnection, [&]
    {
        auto *socket = this->nextPendingConnection();
        new Connection(*this, socket);
    });
}

MHWebServer::Contents MHWebServer::handleGet(const QString &, const QMap<QString, QString> &)
{
    throw MHWebServerHTTPError(404);
}

MHWebServer::WebSocketHandler *MHWebServer::handleWebSocket(const QString &, const QMap<QString, QString> &,
                                  MHWebSocket *)
{
    throw MHWebServerHTTPError(404);
}

MHWebServer::~MHWebServer()
{

}
