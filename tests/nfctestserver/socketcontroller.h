/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNfc module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SOCKETCONTROLLER_H
#define SOCKETCONTROLLER_H

#include <QtCore/QObject>

#include <qnearfieldmanager.h>
#include <qllcpsocket_p.h>

QT_USE_NAMESPACE

class SocketController : public QObject
{
    Q_OBJECT

public:
    enum ConnectionType {
        StreamConnection,
        DatagramConnection,
        BoundSocket,
        ConnectionlessSocket
    };

    SocketController(ConnectionType type, QObject *parent = 0);
    ~SocketController();

public slots:
    void connected();
    void disconnected();
    void error(QLlcpSocket::SocketError socketError);
    void stateChanged(QLlcpSocket::SocketState socketState);
    void readyRead();
    void targetDetected(QNearFieldTarget *target);
    void targetLost(QNearFieldTarget *target);

protected:
    void timerEvent(QTimerEvent *event);

private:
    QNearFieldManager *m_manager;
    QLlcpSocket *m_socket;
    ConnectionType m_connectionType;
    QString m_service;
    quint8 m_port;
    int m_timerId;
};

#endif // SOCKETCONTROLLER_H
