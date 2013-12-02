/****************************************************************************
**
** Copyright (C) 2013 Lauri Laanmets (Proekspert AS) <lauri.laanmets@eesti.ee>
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qbluetoothdevicediscoveryagent.h"
#include "qbluetoothdevicediscoveryagent_p.h"
#include "qbluetoothaddress.h"
#include "qbluetoothuuid.h"
#include "qbluetoothdeviceinfo.h"
#include "qbluetoothlocaldevice_p.h"
#include "android/jnithreadhelper_p.h"


#define QTM_DEVICEDISCOVERY_DEBUG

QT_BEGIN_NAMESPACE

QBluetoothDeviceDiscoveryAgentPrivate::QBluetoothDeviceDiscoveryAgentPrivate(const QBluetoothAddress &deviceAdapter)
{
    bReceiver = NULL;
}

QBluetoothDeviceDiscoveryAgentPrivate::~QBluetoothDeviceDiscoveryAgentPrivate()
{
    delete bReceiver;
}

bool QBluetoothDeviceDiscoveryAgentPrivate::isActive() const
{
    return QBluetoothLocalDevicePrivate::isDiscovering();
}

void QBluetoothDeviceDiscoveryAgentPrivate::start()
{
    Q_Q(QBluetoothDeviceDiscoveryAgent);

    if (bReceiver == NULL) {
        bReceiver = new DeviceDiscoveryBroadcastReceiver();
        qRegisterMetaType<QBluetoothDeviceInfo>("QBluetoothDeviceInfo");
        QObject::connect(bReceiver, SIGNAL(deviceDiscovered(const QBluetoothDeviceInfo&)), q, SIGNAL(deviceDiscovered(const QBluetoothDeviceInfo&)));
        QObject::connect(bReceiver, SIGNAL(finished()), q, SIGNAL(finished()));
    }

    if (!QBluetoothLocalDevicePrivate::startDiscovery())
        emit q->error(QBluetoothDeviceDiscoveryAgent::UnknownError);

    __android_log_print(ANDROID_LOG_DEBUG,"Qt", "QBluetoothDeviceDiscoveryAgentPrivate::start() - successfully executed.");
}

void QBluetoothDeviceDiscoveryAgentPrivate::stop()
{
    Q_Q(QBluetoothDeviceDiscoveryAgent);
    if (QBluetoothLocalDevicePrivate::cancelDiscovery())
        emit q->canceled();
    else
        emit q->error(QBluetoothDeviceDiscoveryAgent::PoweredOffError);
}

QT_END_NAMESPACE
