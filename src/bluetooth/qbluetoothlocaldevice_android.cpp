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

#include <android/log.h>
#include <QtAndroidExtras/QAndroidJniEnvironment>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/QBluetoothAddress>

#include "qbluetoothlocaldevice_p.h"
#include "android/localdevicebroadcastreceiver_p.h"

QT_BEGIN_NAMESPACE

QBluetoothLocalDevicePrivate::QBluetoothLocalDevicePrivate(
        QBluetoothLocalDevice *q, const QBluetoothAddress &address)
    : q_ptr(q), obj(0), inTransition(false)
{
    initialize(address);

    receiver = new LocalDeviceBroadcastReceiver(q_ptr);
    QObject::connect(receiver, SIGNAL(hostModeStateChanged(QBluetoothLocalDevice::HostMode)),
                     this, SLOT(processHostModeChange(QBluetoothLocalDevice::HostMode)));
}


QBluetoothLocalDevicePrivate::~QBluetoothLocalDevicePrivate()
{
    delete receiver;
    delete obj;
}

QAndroidJniObject *QBluetoothLocalDevicePrivate::adapter()
{
    return obj;
}

void QBluetoothLocalDevicePrivate::initialize(const QBluetoothAddress &address)
{
    QAndroidJniEnvironment env;

    jclass btAdapterClass = env->FindClass("android/bluetooth/BluetoothAdapter");
    if (btAdapterClass == NULL) {
        __android_log_print(ANDROID_LOG_FATAL,"QtBluetooth", "Native registration unable to find class android/bluetooth/BluetoothAdapter");
        return;
    }

    jmethodID getDefaultAdapterID = env->GetStaticMethodID(btAdapterClass, "getDefaultAdapter", "()Landroid/bluetooth/BluetoothAdapter;");
    if (getDefaultAdapterID == NULL) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Native registration unable to get method ID: getDefaultAdapter of android/bluetooth/BluetoothAdapter");
        return;
    }


    jobject btAdapterObject = env->CallStaticObjectMethod(btAdapterClass, getDefaultAdapterID);
    if (btAdapterObject == NULL) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Device does not support Bluetooth");
        env->DeleteLocalRef(btAdapterClass);
        return;
    }

    obj = new QAndroidJniObject(btAdapterObject);
    if (!obj->isValid()) {
        delete obj;
        obj = 0;
    } else {
        if (!address.isNull()) {
            const QString localAddress = obj->callObjectMethod("getAddress", "()Ljava/lang/String;").toString();
            if (localAddress != address.toString()) {
                //passed address not local one -> invalid
                delete obj;
                obj = 0;
            }
        }
    }

    env->DeleteLocalRef(btAdapterObject);
    env->DeleteLocalRef(btAdapterClass);
}

//TODO: remove/shift function
bool QBluetoothLocalDevicePrivate::startDiscovery()
{
//    JNIThreadHelper env;

//    jmethodID startDiscoveryID = env->GetMethodID(btAdapterClass, "startDiscovery", "()Z");
//    jboolean ret = env->CallBooleanMethod(btAdapterObject, startDiscoveryID);

//    return ret;

    //TODO
    return true;
}

//TODO: remove/shift function
bool QBluetoothLocalDevicePrivate::cancelDiscovery()
{
//    JNIThreadHelper env;

//    jmethodID cancelDiscoveryID = env->GetMethodID(btAdapterClass, "cancelDiscovery", "()Z");
//    jboolean ret = env->CallBooleanMethod(btAdapterObject, cancelDiscoveryID);

    //TODO
    return true;
}

//TODO: remove/shift function
bool QBluetoothLocalDevicePrivate::isDiscovering()
{
//    JNIThreadHelper env;
//    jmethodID isDiscoveringID = env->GetMethodID(btAdapterClass, "isDiscovering", "()Z");
//    jboolean ret = env->CallBooleanMethod(btAdapterObject, isDiscoveringID);
//    return ret;
    //TODO
    return true;
}

bool QBluetoothLocalDevicePrivate::isValid() const
{
    return obj ? true : false;
}

void QBluetoothLocalDevicePrivate::processHostModeChange(QBluetoothLocalDevice::HostMode newMode)
{
    if (!inTransition) {
        //if not in transition -> pass data on
        emit q_ptr->hostModeStateChanged(newMode);
        return;
    }

    if (isValid() && newMode == QBluetoothLocalDevice::HostPoweredOff) {
        bool success = (bool) obj->callMethod<jboolean>("enable", "()Z");
        if (!success)
            emit q_ptr->error(QBluetoothLocalDevice::UnknownError);
    }

    inTransition = false;
}

QBluetoothLocalDevice::QBluetoothLocalDevice(QObject *parent)
:   QObject(parent),
    d_ptr(new QBluetoothLocalDevicePrivate(this, QBluetoothAddress()))
{
}

QBluetoothLocalDevice::QBluetoothLocalDevice(const QBluetoothAddress &address, QObject *parent)
:   QObject(parent),
    d_ptr(new QBluetoothLocalDevicePrivate(this, address))
{
}

QString QBluetoothLocalDevice::name() const
{
    if (d_ptr->adapter())
        return d_ptr->adapter()->callObjectMethod("getName", "()Ljava/lang/String;").toString();

    return QString();
}

QBluetoothAddress QBluetoothLocalDevice::address() const
{
    QString result;
    if (d_ptr->adapter())
        result = d_ptr->adapter()->callObjectMethod("getAddress", "()Ljava/lang/String;").toString();

    QBluetoothAddress address(result);
    return address;
}

void QBluetoothLocalDevice::powerOn()
{
    if (hostMode() != HostPoweredOff)
        return;

    if (d_ptr->adapter()) {
        bool ret = (bool) d_ptr->adapter()->callMethod<jboolean>("enable", "()Z");
        if (!ret)
            emit error(QBluetoothLocalDevice::UnknownError);
    }
}

void QBluetoothLocalDevice::setHostMode(QBluetoothLocalDevice::HostMode requestedMode)
{
    QBluetoothLocalDevice::HostMode mode = requestedMode;
    if (requestedMode == HostDiscoverableLimitedInquiry)
        mode = HostDiscoverable;

    if (mode == hostMode())
        return;

    if (mode == QBluetoothLocalDevice::HostPoweredOff) {
        bool success = false;
        if (d_ptr->adapter())
            success = (bool) d_ptr->adapter()->callMethod<jboolean>("disable", "()Z");

        if (!success)
            emit error(QBluetoothLocalDevice::UnknownError);
    } else if (mode == QBluetoothLocalDevice::HostConnectable) {
        if (hostMode() == QBluetoothLocalDevice::HostDiscoverable) {
            //cannot directly go from Discoverable to Connectable
            //we need to go to disabled mode and enable once disabling came through

            setHostMode(QBluetoothLocalDevice::HostPoweredOff);
            d_ptr->inTransition = true;
        } else {
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/qt5/android/bluetooth/QtBluetoothBroadcastReceiver", "setConnectable");
        }
    } else if (mode == QBluetoothLocalDevice::HostDiscoverable ||
               mode == QBluetoothLocalDevice::HostDiscoverableLimitedInquiry) {
        QAndroidJniObject::callStaticMethod<void>("org/qtproject/qt5/android/bluetooth/QtBluetoothBroadcastReceiver", "setDiscoverable");
    }
}

QBluetoothLocalDevice::HostMode QBluetoothLocalDevice::hostMode() const
{
    if (d_ptr->adapter()) {
        jint scanMode = d_ptr->adapter()->callMethod<jint>("getScanMode");

        switch (scanMode) {
            case 20: //BluetoothAdapter.SCAN_MODE_NONE
                return HostPoweredOff;
            case 21: //BluetoothAdapter.SCAN_MODE_CONNECTABLE
                return HostConnectable;
            case 23: //BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE
                return HostDiscoverable;
            default:
                break;
        }
    }

    return HostPoweredOff;
}

QList<QBluetoothHostInfo> QBluetoothLocalDevice::allDevices()
{
    //Android only supports max of one device (so far)
    QList<QBluetoothHostInfo> localDevices;

    QAndroidJniEnvironment env;
    jclass btAdapterClass = env->FindClass("android/bluetooth/BluetoothAdapter");
    if (btAdapterClass == NULL) {
        __android_log_print(ANDROID_LOG_FATAL,"QtBluetooth", "Native registration unable to find class android/bluetooth/BluetoothAdapter");
        return localDevices;
    }

    jmethodID getDefaultAdapterID = env->GetStaticMethodID(btAdapterClass, "getDefaultAdapter", "()Landroid/bluetooth/BluetoothAdapter;");
    if (getDefaultAdapterID == NULL) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Native registration unable to get method ID: getDefaultAdapter of android/bluetooth/BluetoothAdapter");
        env->DeleteLocalRef(btAdapterClass);
        return localDevices;
    }


    jobject btAdapterObject = env->CallStaticObjectMethod(btAdapterClass, getDefaultAdapterID);
    if (btAdapterObject == NULL) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Device does not support Bluetooth");
        env->DeleteLocalRef(btAdapterClass);
        return localDevices;
    }

    QAndroidJniObject o(btAdapterObject);
    if (o.isValid()) {
        QBluetoothHostInfo info;
        info.setName(o.callObjectMethod("getName", "()Ljava/lang/String;").toString());
        info.setAddress(QBluetoothAddress(o.callObjectMethod("getAddress", "()Ljava/lang/String;").toString()));
        localDevices.append(info);
    }

    env->DeleteLocalRef(btAdapterObject);
    env->DeleteLocalRef(btAdapterClass);

    return localDevices;
}

void QBluetoothLocalDevice::requestPairing(const QBluetoothAddress &address, Pairing pairing)
{
    //TODO
    emit error(PairingError);

    Q_UNUSED(address);
    Q_UNUSED(pairing);
}

QBluetoothLocalDevice::Pairing QBluetoothLocalDevice::pairingStatus(const QBluetoothAddress &address) const
{
    if (address.isNull() || !d_ptr->adapter())
        return Unpaired;

    QAndroidJniObject inputString = QAndroidJniObject::fromString(address.toString());
    QAndroidJniObject remoteDevice =
            d_ptr->adapter()->callObjectMethod("getRemoteDevice",
                                               "(Ljava/lang/String;)Landroid/bluetooth/BluetoothDevice;",
                                               inputString.object<jstring>());
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return Unpaired;
    }

    jint bondState = remoteDevice.callMethod<jint>("getBondState");
    switch (bondState) {
    case 12: //BluetoothDevice.BOND_BONDED
        return Paired;
    default:
        break;
    }

    return Unpaired;
}

void QBluetoothLocalDevice::pairingConfirmation(bool confirmation)
{
    //TODO
    Q_UNUSED(confirmation);
}

QList<QBluetoothAddress> QBluetoothLocalDevice::connectedDevices() const
{
    return QList<QBluetoothAddress>();
}

QT_END_NAMESPACE
