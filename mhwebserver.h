#ifndef MHWEBSERVER_H
#define MHWEBSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <list>
#include <memory>
#include <QException>
#include "mhwebsocket.h"

class MHWebServerHTTPError : public QException
{
    int _status;
public:
    explicit MHWebServerHTTPError(int status): _status(status) { }
    MHWebServerHTTPError(const MHWebServerHTTPError&) = default;
    int status() const { return _status; }
    void raise() const { throw *this; }
    MHWebServerHTTPError *clone() const { return new MHWebServerHTTPError(*this); }
};

class MHWebServer : public QTcpServer
{
    Q_OBJECT

    class Connection;

public:

    class WebSocketHandler
    {
    public:
        virtual ~WebSocketHandler() { }
        virtual QString getProtocol() = 0;
        virtual void start() = 0;
    };

    struct Contents
    {
        QString contentType;
        QByteArray data;
    };

protected:


    virtual Contents handleGet(const QString& uri, const QMap<QString, QString>& headers);
    virtual WebSocketHandler *handleWebSocket(const QString& uri, const QMap<QString, QString>& headers,
                                              MHWebSocket *socket);

public:



    explicit MHWebServer(QObject *parent = 0);
    ~MHWebServer();
signals:

public slots:

};

#endif // MHWEBSERVER_H
