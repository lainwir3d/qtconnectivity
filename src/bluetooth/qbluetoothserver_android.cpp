/***************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QLoggingCategory>
#include "qbluetoothserver.h"
#include "qbluetoothserver_p.h"
#include "qbluetoothsocket.h"
#include "qbluetoothsocket_p.h"
#include "qbluetoothlocaldevice.h"
#include "android/serveracceptancethread_p.h"

#include <QCoreApplication>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_BT_ANDROID)

QHash<QBluetoothServerPrivate*, int> __fakeServerPorts;

QBluetoothServerPrivate::QBluetoothServerPrivate(QBluetoothServiceInfo::Protocol sType)
    : socket(0),maxPendingConnections(1), securityFlags(QBluetooth::NoSecurity), serverType(sType),
      m_lastError(QBluetoothServer::NoError)
{
        thread = new ServerAcceptanceThread();
}

QBluetoothServerPrivate::~QBluetoothServerPrivate()
{
    Q_Q(QBluetoothServer);
    if (isListening())
        q->close();

    __fakeServerPorts.remove(this);

    if (thread->isRunning()) {
        thread->stop();
        thread->wait();
    }
    thread->deleteLater();
    thread = 0;
}

bool QBluetoothServerPrivate::initiateActiveListening(
        const QBluetoothUuid& uuid, const QString &serviceName)
{
    Q_UNUSED(uuid);
    Q_UNUSED(serviceName);
    qCDebug(QT_BT_ANDROID) << "initiate active listening" << uuid.toString() << serviceName;

    if (uuid.isNull() || serviceName.isEmpty())
        return false;

    //no change of SP profile details -> do nothing
    if (uuid == m_uuid && serviceName == m_serviceName)
        return true;

    m_uuid = uuid;
    m_serviceName = serviceName;
    thread->setServiceDetails(m_uuid, m_serviceName);

    Q_ASSERT(!thread->isRunning());
    thread->start();
    Q_ASSERT(thread->isRunning());

    return true;
}

bool QBluetoothServerPrivate::deactivateActiveListening()
{
    thread->stop();
    thread->wait();

    return true;
}

bool QBluetoothServerPrivate::isListening() const
{
    return thread->isRunning();
}

void QBluetoothServer::close()
{
    Q_D(QBluetoothServer);

    d->thread->stop();
    d->thread->wait();

    if (d->thread)
        d->thread->disconnect();
    __fakeServerPorts.remove(d);
}

bool QBluetoothServer::listen(const QBluetoothAddress &localAdapter, quint16 port)
{
    const QList<QBluetoothHostInfo> localDevices = QBluetoothLocalDevice::allDevices();
    if (!localDevices.count())
        return false; //no Bluetooth device

    if (!localAdapter.isNull()) {
        bool found = false;
        foreach (const QBluetoothHostInfo &hostInfo, localDevices) {
            if (hostInfo.address() == localAdapter) {
                found = true;
                break;
            }
        }

        if (!found) {
            qWarning(QT_BT_ANDROID) << localAdapter.toString() << "is not a valid local Bt adapter";
            return false;
        }
    }

    Q_D(QBluetoothServer);
    if (serverType() != QBluetoothServiceInfo::RfcommProtocol) {
        d->m_lastError = UnsupportedProtocolError;
        emit error(d->m_lastError);
        return false;
    }

    if (d->isListening())
        return false;

    //check Bluetooth is available and online
    QAndroidJniObject btAdapter = QAndroidJniObject::callStaticObjectMethod(
                                        "android/bluetooth/BluetoothAdapter",
                                        "getDefaultAdapter",
                                        "()Landroid/bluetooth/BluetoothAdapter;");
    if (!btAdapter.isValid()) {
        qCWarning(QT_BT_ANDROID) << "Device does not support Bluetooth";
        d->m_lastError = QBluetoothServer::UnknownError;
        emit error(d->m_lastError);
        return false;
    }

    const int state = btAdapter.callMethod<jint>("getState");
    if (state != 12 ) { //BluetoothAdapter.STATE_ON
        d->m_lastError = QBluetoothServer::PoweredOffError;
        emit error(d->m_lastError);
        qCWarning(QT_BT_ANDROID) << "Bluetooth device is powered off";
        return false;
    }

    //We can not register an actual Rfcomm port, because the platform does not allow it
    //but we need a way to associate a server with a service
    if (port == 0) { //Try to assign a non taken port id
        for (int i=1; ; i++){
            if (__fakeServerPorts.key(i) == 0) {
                port = i;
                break;
            }
        }
    }

    if (__fakeServerPorts.key(port) == 0) {
        __fakeServerPorts[d] = port;

        qCDebug(QT_BT_ANDROID) << "Port" << port << "registered";
    } else {
        qCWarning(QT_BT_ANDROID) << "server with port" << port << "already registered or port invalid";
        d->m_lastError = ServiceAlreadyRegisteredError;
        emit error(d->m_lastError);
        return false;
    }

    connect(d->thread, SIGNAL(newConnection()), this, SIGNAL(newConnection()));
    return true;
}

void QBluetoothServer::setMaxPendingConnections(int numConnections)
{
    //Q_D(QBluetoothServer);
    //TODO
    Q_UNUSED(numConnections);
    //d->maxPendingConnections = numConnections; //Currently not used
}

QBluetoothAddress QBluetoothServer::serverAddress() const
{
    //Android only supports one local adapter
    QList<QBluetoothHostInfo> hosts = QBluetoothLocalDevice::allDevices();
    Q_ASSERT(hosts.count() <= 1);

    if (hosts.isEmpty())
        return QBluetoothAddress();
    else
        return hosts.at(0).address();
}

quint16 QBluetoothServer::serverPort() const
{
    //We return the fake port
    Q_D(const QBluetoothServer);
    return __fakeServerPorts.value((QBluetoothServerPrivate*)d, 0);
}

bool QBluetoothServer::hasPendingConnections() const
{
    Q_D(const QBluetoothServer);

    return d->thread->hasPendingConnections();
}

QBluetoothSocket *QBluetoothServer::nextPendingConnection()
{
    Q_D(const QBluetoothServer);

    QAndroidJniObject socket = d->thread->nextPendingConnection();
    if (!socket.isValid())
        return 0;


    QBluetoothSocket *newSocket = new QBluetoothSocket();
    bool success = newSocket->d_ptr->setSocketDescriptor(socket, d->serverType);
    if (!success) {
        delete newSocket;
        newSocket = 0;
    }

    return newSocket;
}

void QBluetoothServer::setSecurityFlags(QBluetooth::SecurityFlags security)
{
    //TODO
    Q_D(QBluetoothServer);
    d->securityFlags = security; //not used
}

QBluetooth::SecurityFlags QBluetoothServer::securityFlags() const
{
    //TODO
    Q_D(const QBluetoothServer);
    return d->securityFlags; //not used
}

QT_END_NAMESPACE

