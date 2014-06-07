#include "mhserver.h"
#include <QFile>
#include <random>
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>
#include "mhplatform.h"
#include <QDateTime>
#include <QPointF>
#include <QHostInfo>

MHWSHandler::MHWSHandler(MHServer *server, const QMap<QString, QString> &headers, MHWebSocket *socket):
server(server),
socket(socket),
accepted(false)
{
    connect(socket->tcpSocket(), SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(server, SIGNAL(passcodeAccepted(QString)),
            this, SLOT(passcodeAccepted(QString)));
    connect(server, SIGNAL(sessionAccepted(QString)),
            this, SLOT(sessionAccepted(QString)));
    connect(server, SIGNAL(sessionRejected(QString,QString)),
            this, SLOT(sessionTerminated(QString,QString)));
    connect(server, SIGNAL(sessionOverridden(QString,MHWSHandler*)),
            this, SLOT(sessionOverridden(QString,MHWSHandler*)));

    connect(socket, SIGNAL(messageReceived(QByteArray)),
            this, SLOT(readReady(QByteArray)));

    timer = new QTimer(this);
    timer->setInterval(15);
    timer->setSingleShot(false);
    timer->setTimerType(Qt::CoarseTimer);
    connect(timer, SIGNAL(timeout()),
            this, SLOT(onTimer()));
    lastMotionTime = 0;
    lastMotionTime2 = 0;

    // generate passcode
    std::random_device rd;
    {
        std::uniform_int_distribution<int> dist(0, 9);
        QByteArray b;
        for (int i = 0; i < 8; ++i)
            b.append(QString::number(dist(rd)));
        passcode = b;
    }

    // generate session code
    {
        std::uniform_int_distribution<int> dist(0, 255);
        QByteArray b;
        for (int i = 0; i < 32; ++i)
            b.append((char)dist(rd));
        sessionCode = b.toBase64();
    }

    qDebug() << "new connection " <<passcode;
}


void MHWSHandler::passcodeAccepted(const QString &s)
{
    if (s == passcode) {
        server->acceptSessionCode(sessionCode);
    }
}
void MHWSHandler::sessionAccepted(const QString& s)
{
    if (s == sessionCode) {
        sessionAccepted();
    } else {
        timer->stop();
    }
}

void MHWSHandler::onDisconnected()
{
    qDebug() << "disconnected " <<passcode;

    socket->deleteLater();
    this->deleteLater();
}

void MHWSHandler::sessionAccepted()
{
    if (accepted) {
        return;
    }
    qDebug() << "accepted " << passcode;

    QJsonObject obj;
    obj["type"] = "accepted";
    obj["session"] = sessionCode;
    sendPacket(obj);
    accepted = true;
    timer->start();
}

void MHWSHandler::sessionTerminated(const QString& s, const QString& reason)
{
    if (s != sessionCode) {
        return;
    }
    qDebug() << "terminating " <<passcode << ", reason: " << reason;

    QJsonObject obj;
    obj["type"] = "kicked";
    obj["reason"] = reason;
    sendPacket(obj);
    socket->tcpSocket()->disconnectFromHost();
    accepted = false;
    timer->stop();
}

void MHWSHandler::sessionOverridden(const QString &sessionCode, MHWSHandler *newHandler)
{
    if (this->sessionCode == sessionCode && this != newHandler) {
        sessionTerminated(sessionCode, "Session was overridden.");
    }
}

void MHWSHandler::readReady(QByteArray chunk)
{
    try {
        QJsonDocument doc = QJsonDocument::fromJson(chunk);
        if (doc.isNull()) {
            throw std::runtime_error("null");
        }
        if (!doc.isObject()) {
            throw std::runtime_error("not object");
        }

        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();
        if (type == "session" && !accepted) {
            // restore session
            sessionCode = obj["session"].toString();

            if (server->isSessionAccepted(sessionCode)) {
                server->acceptSessionCode(sessionCode);
                server->onSessionOverridden(sessionCode, this);
                sessionAccepted();
            }
            return;
        } else if (type == "ping") {
            QJsonObject obj2;
            obj2["type"] = "pong";
            obj2["data"] = obj["data"];
            sendPacket(obj2);
            return;
        } else if (type == "keep_alive") {
            return;
        } else if (accepted) {
            if (type == "mouse_motion") {
                QPointF pt;
                pt.setX(obj["x"].toDouble());
                pt.setY(obj["y"].toDouble());
                lastMotion2 = lastMotion;
                lastMotionTime2 = lastMotionTime;
                lastMotion = pt;
                lastMotionTime = QDateTime::currentMSecsSinceEpoch();
                //qDebug() << " <<< " << pt << timer->isActive();
                // somehow timer doesn't get activated...
                onTimer();
                if (!timer->isActive()) {
                    timer->start();
                }
                return;
            } else if (type == "mouse_button") {
                MHPlatform::MouseButton button;
                QString buttonName = obj["button"].toString();
                if (buttonName == "left") {
                    button = MHPlatform::MouseButton::Left;
                } else if (buttonName == "right") {
                    button = MHPlatform::MouseButton::Right;
                } else if (buttonName == "center") {
                    button = MHPlatform::MouseButton::Center;
                } else {
                    throw std::runtime_error("invalid button");
                }
                if (obj["down"].toBool()) {
                    MHPlatform::mouseButtonDown(button);
                } else {
                    MHPlatform::mouseButtonUp(button);
                }
                return;
            }
        }
        throw std::runtime_error("unhandled");
    } catch (const std::exception& ex) {
        qDebug() << "error handling JSON data" << chunk << ": " << ex.what();
    }

}

void MHWSHandler::onTimer()
{
    if (!accepted) {
        timer->stop();
        return;
    }

    qulonglong time = QDateTime::currentMSecsSinceEpoch();
    if (time > lastMotionTime + 600) {
        return;
    }

    QPointF pt = lastMotion;
    if (time < lastMotionTime2 + 600) {
        // linear interpolate
        double per = time - lastMotionTime;
        per /= (lastMotionTime - lastMotionTime2 + 1);
        per = std::min(per, 1.0);
        per = std::max(per, 0.0);
        pt = lastMotion2 + (lastMotion - lastMotion2) * per;
    }

    MHPlatform::mouseMotion(pt);
}

void MHWSHandler::sendPacket(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson();
    socket->send(data);
}

MHWSHandler::~MHWSHandler()
{
    qDebug() << "deleting " << passcode;
    MHPlatform::mouseButtonUp(MHPlatform::MouseButton::Left);
    MHPlatform::mouseButtonUp(MHPlatform::MouseButton::Right);
    MHPlatform::mouseButtonUp(MHPlatform::MouseButton::Center);
}

QString MHWSHandler::getProtocol()
{
    return "mhctl";
}

void MHWSHandler::start()
{
    // send passcode
    QJsonObject obj;
    obj["type"] = "start";
    obj["code"] = passcode;
    obj["screen_width"] = MHPlatform::screenSize().width();
    obj["screen_height"] = MHPlatform::screenSize().height();
    sendPacket(obj);
}



MHServer::MHServer(QObject *parent) :
    MHWebServer(parent)
{
}

namespace {
    MHServer::Contents makeContentsFromFile(const QString& path, const QString& contentType) {
        MHServer::Contents cont;
        QFile file(path);
        if (!file.open(QFile::ReadOnly)) {
            qDebug() << "Reading " << path << " failed: " << file.error();
            throw MHWebServerHTTPError(404);
        }
        cont.data = file.readAll().replace("%{HOSTNAME}", QHostInfo::localHostName().toUtf8());
        cont.contentType = contentType;
        return cont;
    }
}

MHServer::Contents MHServer::handleGet(const QString &uri, const QMap<QString, QString> &headers)
{
    if (uri == "/") {
        return makeContentsFromFile(":/mh/web/index.html", "text/html");
    } else {
        if (uri.toLower() == "/index.html")
            throw MHWebServerHTTPError(404);
        QString contentsType = "application/x-octet-stream";
        if (uri.endsWith(".css")) contentsType = "text/css";
        else if (uri.endsWith(".js")) contentsType = "text/javascript";
        else if (uri.endsWith(".html")) contentsType = "text/html";
        return makeContentsFromFile(":/mh/web" + uri, contentsType);
    }
    throw MHWebServerHTTPError(404);
}

MHServer::WebSocketHandler *MHServer::handleWebSocket
(const QString &uri, const QMap<QString, QString> &headers, MHWebSocket *socket)
{
    if (uri == "/mhctl") {
        return new MHWSHandler(this, headers, socket);
    } else {
        throw MHWebServerHTTPError(404);
    }
}

bool MHServer::isSessionAccepted(const QString &code)
{
    return acceptedSessions.contains(code);
}

void MHServer::acceptPasscode(const QString &code)
{
    emit passcodeAccepted(code);
}

void MHServer::acceptSessionCode(const QString &code)
{
    if (!acceptedSessions.empty()) {
        // accept only one session so far.
        QString existing = *acceptedSessions.begin();
        if (existing != code) {
            rejectSessionCode(existing, "Another user is controlling the computer.");
        }
    }
    acceptedSessions.insert(code);
    emit sessionAccepted(code);
}

void MHServer::onSessionOverridden(const QString & session, MHWSHandler *handler)
{
    emit sessionOverridden(session, handler);
}

void MHServer::rejectSessionCode(const QString &code, const QString &reason)
{
    qDebug() << "rejectSessionCode :" << code << reason;
    emit sessionRejected(code, reason);
}
