/*
 * Copyright (c) 2024 MAQ
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef _GC_qdomyosWS_h
#define _GC_qdomyosWS_h 1

#include <QObject>

class QWebSocket;
class QTimer;

class qdSocket : public QObject
{
    Q_OBJECT

public:
    qdSocket(const QString &url, QObject *parent = nullptr);
    void sendResistance(double resistance, bool update_gc_slope = true);

    virtual ~qdSocket();

private slots:
    void onConnected();
    void onDisconnected();

    void timeout_update_slot();

signals:
    void setNotification(QString msg, int timeout);

private:
    QWebSocket *m_webSocket;
    QString m_url;
    double m_treadmill_slope;   // Last Slope sent to the treadmill
    double m_gc_slope;         // Current Slope in GC

    QTimer *m_timer_slope;
    bool    m_update_slope;
};


#endif // _GC_qdomyosWS_h
