#include "mhwebsocket.h"

MHWebSocket::MHWebSocket(QTcpSocket *socket, QObject *parent) :
    QObject(parent),
    socket(socket)
{
    connect(socket, SIGNAL(readyRead()),
            this, SLOT(onReadyRead()));
    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    pingTimer = new QTimer();
    pingTimer->setInterval(1000);
    pingTimer->setTimerType(Qt::VeryCoarseTimer);
    pingTimer->setSingleShot(false);
    pingTimer->start();
    connect(pingTimer, SIGNAL(timeout()),
            this, SLOT(pingTimerTimedOut()));
    watchdogCounter = 5;
}

MHWebSocket::~MHWebSocket() {
    socket->deleteLater();
}

void MHWebSocket::pingTimerTimedOut() {
    sendPing();
    watchdogCounter--;
    if (watchdogCounter < 0) {
        socket->disconnectFromHost();
        pingTimer->stop();
    }
}

void MHWebSocket::onReadyRead()
{
    buffer.append(socket->readAll());
    watchdogCounter = 5; // reset
    if (buffer.size() < 2) {
        // not enough
        return;
    }
    int pos = 0;
    bool fin = buffer[0] & 128;
    bool rsv1 = buffer[0] & 64;
    bool rsv2 = buffer[0] & 32;
    bool rsv3 = buffer[0] & 16;
    int opcode = (buffer[0]) & 15;
    if (rsv1 || rsv2 || rsv3) {
        // unsupported!
        socket->reset();
    }
    bool mask = buffer[1] & 128;
    qulonglong len = (buffer[1]) & 127;
    pos = 2;
    if (len == 126) {
        if (buffer.size() < 4) {
            // not enough
            return;
        }
        len = (unsigned char)buffer[2];
        len = (unsigned char)buffer[3] | (len << 8);
        pos = 4;
    } else if (len == 127) {
        if (buffer.size() < 10) {
            // not enough
            return;
        }
        len = (unsigned char)buffer[2];
        len = (unsigned char)buffer[3] | (len << 8);
        len = (unsigned char)buffer[4] | (len << 8);
        len = (unsigned char)buffer[5] | (len << 8);
        len = (unsigned char)buffer[6] | (len << 8);
        len = (unsigned char)buffer[7] | (len << 8);
        len = (unsigned char)buffer[8] | (len << 8);
        len = (unsigned char)buffer[9] | (len << 8);
        pos = 10;
    }

    if (len > 1024 * 1024) {
        // too long...
        socket->reset();
        return;
    }

    char maskKey[4];
    if (mask) {
        // mask key
        if (buffer.size() < pos + 4) {
            // not enough
            return;
        }
        maskKey[0] = buffer[pos++];
        maskKey[1] = buffer[pos++];
        maskKey[2] = buffer[pos++];
        maskKey[3] = buffer[pos++];
    } else {
        // client-to-server data must be masked!
        socket->reset();
        return;
    }

    if (buffer.size() < pos + len) {
        // not enough
        return;
    }

    int maskIndex = 0;
    for(int i = 0; i < len; i++) {
        buffer[i + pos] = buffer[i + pos] ^ maskKey[maskIndex];
        maskIndex = (maskIndex + 1) & 3;
    }
    msgBuffer.append(buffer.data() + pos, len);
    pos += len;
    buffer.remove(0, pos);

    if (fin) {
        if (opcode == 0x8) {
            // connection close
            socket->disconnectFromHost();
        } else if (opcode == 0x1 || opcode == 0x2) {
            QByteArray b;
            b.swap(msgBuffer);
            emit messageReceived(b);
        }
    }
}

void MHWebSocket::send(QByteArray payload)
{
    QByteArray packet;
    packet.append((char)0x81); // FIN, text frame

    int shortLen;
    qulonglong fullLen = payload.size();
    if (fullLen < 126) {
        shortLen = (int)fullLen;
        packet.append((char)shortLen);
    } else if (fullLen < 65536) {
        shortLen = 126;
        packet.append((char)shortLen);
        uint16_t middleLen = (uint16_t)fullLen;
        packet.append(reinterpret_cast<char*>(&middleLen), 2);
    } else {
        shortLen = 127;
        packet.append((char)shortLen);
        packet.append(reinterpret_cast<char*>(&fullLen), 8);
    }

    // (no masking-key)

    packet.append(payload);
    socket->write(packet);
    socket->flush();
}

void MHWebSocket::sendPing()
{
    QByteArray packet;
    packet.append((char)0x89); // FIN, ping

    packet.append((char)0);
    // (no masking-key)

    socket->write(packet);
    socket->flush();
}

