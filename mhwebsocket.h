#ifndef MHWEBSOCKET_H
#define MHWEBSOCKET_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class MHWebSocket : public QObject
{
    Q_OBJECT
    QTcpSocket *socket;
    QByteArray buffer;
    QByteArray msgBuffer;
    QTimer *pingTimer;
    int watchdogCounter;
public:
    explicit MHWebSocket(QTcpSocket *socket, QObject *parent = 0);
    ~MHWebSocket();

    QTcpSocket *tcpSocket() const { return socket; }

    void send(QByteArray);
    void sendPing();

signals:
    void messageReceived(QByteArray);

private slots:
    void onReadyRead();
    void pingTimerTimedOut();
};

#endif // MHWEBSOCKET_H
