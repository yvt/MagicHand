#ifndef MHSERVER_H
#define MHSERVER_H

#include "mhwebserver.h"
#include <QSet>
#include <QTimer>
#include <QPointF>

class MHServer;
class MHWSHandler: public QObject, public MHWebServer::WebSocketHandler
{
    Q_OBJECT
    MHServer *server;
    MHWebSocket *socket;
    QTimer *timer;
    QString passcode;
    QString sessionCode;

    QPointF lastMotion;
    qulonglong lastMotionTime;

    QPointF lastMotion2;
    qulonglong lastMotionTime2;


    bool accepted;

    void sessionAccepted();
    void sendPacket(const QJsonObject&);

public:
    MHWSHandler(MHServer *, const QMap<QString, QString> &headers, MHWebSocket *socket);
    ~MHWSHandler();

    QString getProtocol() Q_DECL_OVERRIDE;
    void start() Q_DECL_OVERRIDE;

private slots:
    void onDisconnected();
    void readReady(QByteArray);
    void passcodeAccepted(const QString&);
    void sessionAccepted(const QString&);
    void sessionTerminated(const QString&, const QString& reason);
    void sessionOverridden(const QString&, MHWSHandler *);
    void onTimer();
};

class MHServer : public MHWebServer
{
    Q_OBJECT

    class WSHandler;

    QSet<QString> acceptedSessions;

protected:
    Contents handleGet(const QString& uri, const QMap<QString, QString>& headers) Q_DECL_OVERRIDE;
    WebSocketHandler *handleWebSocket(const QString& uri, const QMap<QString, QString>& headers,
                                              MHWebSocket *socket) Q_DECL_OVERRIDE;

public:
    explicit MHServer(QObject *parent = 0);

    void acceptPasscode(const QString&);
    void acceptSessionCode(const QString&);
    bool isSessionAccepted(const QString&);
    void rejectSessionCode(const QString&, const QString& reason);
    void onSessionOverridden(const QString&, MHWSHandler *);

signals:

    void passcodeAccepted(const QString&);
    void sessionAccepted(const QString&);
    void sessionRejected(const QString&, const QString& reason);
    void sessionOverridden(const QString&, MHWSHandler *);

private slots:
};

#endif // MHSERVER_H
