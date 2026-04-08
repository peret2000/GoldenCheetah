#include "qdomyosWS.h"

#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSocketNotifier>
#include <QWebSocket>
#include <QTimer>


qdSocket::qdSocket(const QString &url, QObject *parent)
    :   QObject(parent),
        m_url(url),
        m_treadmill_slope(0.0),
        m_gc_slope(0.0),
        m_update_slope(false)
{
    m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    QObject::connect(m_webSocket, &QWebSocket::connected, this, &qdSocket::onConnected);

    QObject::connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, [this](QAbstractSocket::SocketError error) {
        QString str = "QDomyos Websocket: WebSocket error: " + QString::number(error) + ". " + m_webSocket->errorString();
        qCritical() << str;
        emit setNotification(str, 2);
    });

    QObject::connect(m_webSocket, &QWebSocket::disconnected, this, &qdSocket::onDisconnected);

    m_timer_slope = new QTimer(this);
    m_timer_slope->setSingleShot(true);
    QObject::connect(m_timer_slope, SIGNAL(timeout()), this, SLOT(timeout_update_slot()));

    // Utiliza QTimer::singleShot para retrasar la apertura del socket a que termine la cola de eventos
    QTimer::singleShot(0, this, [this]() {
        m_webSocket->open(QUrl(m_url));
    });

}

void qdSocket::onConnected()
{
	qDebug() << "QDomyos Websocket: Conectado...";
    m_update_slope = true;
}

void qdSocket::onDisconnected()
{
	qDebug() << "QDomyos Websocket: Desconectado...";
    m_timer_slope->stop();
    m_update_slope = false;
}

void qdSocket::timeout_update_slot()
{
    m_update_slope = true;
    if (m_gc_slope != m_treadmill_slope) {
        m_timer_slope->stop();
        sendResistance(m_gc_slope, false);
    }
}

void qdSocket::sendResistance(double resistance, bool update_gc_slope) {

    if (update_gc_slope || m_update_slope)
        resistance = QString::number(resistance, 'f', 1).toDouble();
    if (update_gc_slope)
        m_gc_slope = resistance;

    if (m_update_slope && m_treadmill_slope != resistance) {
        m_treadmill_slope = resistance;

        QJsonObject json;
        json["msg"] = "setresistance";
        QJsonObject content;
        content["value"] = m_treadmill_slope;
        json["content"] = content;

        QJsonDocument doc(json);
        QString jsonString = doc.toJson(QJsonDocument::Compact);

        m_webSocket->sendTextMessage(jsonString);
        qDebug() << "QDomyos Websocket: Sent message:" << jsonString;
        //emit TrainSidebar::setNotification("Changing Treadmill Slope to " + QString::number(resistance), 1);
        emit setNotification("Changing Treadmill Slope to " + QString::number(resistance), 1);

        m_update_slope = false;
        m_timer_slope->start(5000);
    }

}



qdSocket::~qdSocket() {
    if (m_webSocket) {
        m_webSocket->close();
        delete m_webSocket;
	}
    if (m_timer_slope) {
        delete m_timer_slope;
    }
};
