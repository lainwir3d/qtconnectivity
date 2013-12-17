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

#include "qbluetoothdevicediscoveryagent_p.h"
#include <QtCore/QLoggingCategory>
#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include "android/devicediscoverybroadcastreceiver_p.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_BT_ANDROID)

QBluetoothDeviceDiscoveryAgentPrivate::QBluetoothDeviceDiscoveryAgentPrivate(
                                                    const QBluetoothAddress &deviceAdapter)
    : inquiryType(QBluetoothDeviceDiscoveryAgent::GeneralUnlimitedInquiry),
      lastError(QBluetoothDeviceDiscoveryAgent::NoError), errorString(QStringLiteral()),
      receiver(0),
      m_adapterAddress(deviceAdapter),
      m_active(false),
      pendingCancel(false),
      pendingStart(false)
{
    adapter = QAndroidJniObject::callStaticObjectMethod("android/bluetooth/BluetoothAdapter",
                                                        "getDefaultAdapter",
                                                        "()Landroid/bluetooth/BluetoothAdapter;");
}

QBluetoothDeviceDiscoveryAgentPrivate::~QBluetoothDeviceDiscoveryAgentPrivate()
{
    if (m_active)
        stop();

    delete receiver;
}

bool QBluetoothDeviceDiscoveryAgentPrivate::isActive() const
{
    if (pendingStart)
        return true;
    if (pendingCancel)
        return false;
    return m_active;
}

void QBluetoothDeviceDiscoveryAgentPrivate::start()
{
    if (pendingCancel) {
        pendingStart = true;
        return;
    }

    Q_Q(QBluetoothDeviceDiscoveryAgent);

    if (!adapter.isValid()) {
        qCWarning(QT_BT_ANDROID) << "Device does not support Bluetooth";
        lastError = QBluetoothDeviceDiscoveryAgent::InputOutputError;
        errorString = QBluetoothDeviceDiscoveryAgent::tr("Device does not support Bluetooth.");
        emit q->error(lastError);
        return;
    }

    //TODO change to new error enum InvalidBluetoothAdapterError
    if (!m_adapterAddress.isNull() &&
            adapter.callObjectMethod<jstring>("getAddress").toString() != m_adapterAddress.toString()) {
        qCWarning(QT_BT_ANDROID) << "Incorrect local adapter passed.";
        lastError = QBluetoothDeviceDiscoveryAgent::InputOutputError;
        errorString = QBluetoothDeviceDiscoveryAgent::tr("Passed address is not a local device.");
        emit q->error(lastError);
        return;
    }

    const int state = adapter.callMethod<jint>("getState");
    if (state != 12 ) { //BluetoothAdapter.STATE_ON
        lastError = QBluetoothDeviceDiscoveryAgent::PoweredOffError;
        errorString = QBluetoothDeviceDiscoveryAgent::tr("Device is powered off.");
        emit q->error(lastError);
        return;
    }

    //install Java BroadcastReceiver
    if (!receiver) {
        receiver = new DeviceDiscoveryBroadcastReceiver();
        qRegisterMetaType<QBluetoothDeviceInfo>("QBluetoothDeviceInfo");
        QObject::connect(receiver, SIGNAL(deviceDiscovered(const QBluetoothDeviceInfo&)),
                         this, SLOT(processDiscoveredDevices(const QBluetoothDeviceInfo&)));
        QObject::connect(receiver, SIGNAL(finished()), this, SLOT(processDiscoveryFinished()));
    }

    discoveredDevices.clear();

    const bool success = adapter.callMethod<jboolean>("startDiscovery");
    if (!success) {
        lastError = QBluetoothDeviceDiscoveryAgent::InputOutputError;
        errorString = QBluetoothDeviceDiscoveryAgent::tr("Discovery cannot be started.");
        emit q->error(lastError);
        return;
    }

    m_active = true;

    qCDebug(QT_BT_ANDROID) << "QBluetoothDeviceDiscoveryAgentPrivate::start() - successfully executed.";
}

void QBluetoothDeviceDiscoveryAgentPrivate::stop()
{
    Q_Q(QBluetoothDeviceDiscoveryAgent);

    if (!m_active)
        return;

    pendingCancel = true;
    pendingStart = false;
    bool success = adapter.callMethod<jboolean>("cancelDiscovery");
    if (!success) {
        lastError = QBluetoothDeviceDiscoveryAgent::InputOutputError;
        errorString = QBluetoothDeviceDiscoveryAgent::tr("Discovery cannot be stopped.");
        emit q->error(lastError);
        return;
    }
}

void QBluetoothDeviceDiscoveryAgentPrivate::processDiscoveryFinished()
{
    if (!m_active) //We need to guard because Android sends two DISCOVERY_FINISHED when cancelling
        return;

    m_active = false;

    Q_Q(QBluetoothDeviceDiscoveryAgent);

    if (pendingCancel && !pendingStart) {
        emit q->canceled();
        pendingCancel = false;
    } else if (pendingStart) {
        pendingStart = pendingCancel = false;
        start();
    } else {
        emit q->finished();
    }
}

void QBluetoothDeviceDiscoveryAgentPrivate::processDiscoveredDevices(const QBluetoothDeviceInfo &info)
{
    Q_Q(QBluetoothDeviceDiscoveryAgent);

    discoveredDevices.append(info);
    emit q->deviceDiscovered(info);
}
QT_END_NAMESPACE
