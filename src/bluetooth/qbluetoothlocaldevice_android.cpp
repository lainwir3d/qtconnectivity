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

#include "qbluetoothlocaldevice.h"
#include "qbluetoothaddress.h"
#include "qbluetoothlocaldevice_p.h"
#include "android/jnithreadhelper_p.h"
#include <android/log.h>

QT_BEGIN_NAMESPACE

jclass QBluetoothLocalDevicePrivate::btAdapterClass=NULL;
jobject QBluetoothLocalDevicePrivate::btAdapterObject=NULL;

void QBluetoothLocalDevicePrivate::initialize(JNIEnv *env)
{
    btAdapterClass = env->FindClass("android/bluetooth/BluetoothAdapter");
    if (btAdapterClass == NULL)
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Native registration unable to find class android/bluetooth/BluetoothAdapter");
    else
        btAdapterClass = (jclass)env->NewGlobalRef(btAdapterClass);

    jmethodID getDefaultAdapterID = env->GetStaticMethodID(btAdapterClass, "getDefaultAdapter", "()Landroid/bluetooth/BluetoothAdapter;");
    if (getDefaultAdapterID == NULL)
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Native registration unable to get method ID: getDefaultAdapter of android/bluetooth/BluetoothAdapter");

    btAdapterObject = env->CallStaticObjectMethod(btAdapterClass, getDefaultAdapterID);
    if (btAdapterObject == NULL)
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Device does not support Bluetooth");

    btAdapterObject = env->NewGlobalRef(btAdapterObject);
}

bool QBluetoothLocalDevicePrivate::startDiscovery()
{
    JNIThreadHelper env;

    jmethodID startDiscoveryID = env->GetMethodID(btAdapterClass, "startDiscovery", "()Z");
    jboolean ret = env->CallBooleanMethod(btAdapterObject, startDiscoveryID);

    return ret;
}
bool QBluetoothLocalDevicePrivate::cancelDiscovery()
{
    JNIThreadHelper env;

    jmethodID cancelDiscoveryID = env->GetMethodID(btAdapterClass, "cancelDiscovery", "()Z");
    jboolean ret = env->CallBooleanMethod(btAdapterObject, cancelDiscoveryID);

    return ret;
}
bool QBluetoothLocalDevicePrivate::isDiscovering()
{
    JNIThreadHelper env;
    jmethodID isDiscoveringID = env->GetMethodID(btAdapterClass, "isDiscovering", "()Z");
    jboolean ret = env->CallBooleanMethod(btAdapterObject, isDiscoveringID);
    return ret;
}


QBluetoothLocalDevice::QBluetoothLocalDevice(QObject *parent)
:   QObject(parent)
{
}

QBluetoothLocalDevice::QBluetoothLocalDevice(const QBluetoothAddress &address, QObject *parent)
: QObject(parent)
{
}

QString QBluetoothLocalDevice::name() const
{
    if (d_ptr->btAdapterObject != 0) {
        JNIThreadHelper env;

        static jmethodID getNameID = env->GetMethodID(d_ptr->btAdapterClass, "getName", "()Ljava/lang/String;");
        jstring peerName = (jstring) env->CallObjectMethod(d_ptr->btAdapterObject, getNameID);

        jboolean isCopy;
        const jchar* name = env->GetStringChars(peerName, &isCopy);
        QString qtName = QString::fromUtf16(name, env->GetStringLength(peerName));
        env->ReleaseStringChars(peerName, name);
        return qtName;
    } else {
        return QString();
    }
}

QBluetoothAddress QBluetoothLocalDevice::address() const
{
    if (d_ptr->btAdapterObject != 0) {
        JNIThreadHelper env;

        static jmethodID getAddressID = env->GetMethodID(d_ptr->btAdapterClass, "getAddress", "()Ljava/lang/String;");
        jstring peerName = (jstring) env->CallObjectMethod(d_ptr->btAdapterObject, getAddressID);

        jboolean isCopy;
        const jchar* addressStr = env->GetStringChars(peerName, &isCopy);
        QString strAddress = QString::fromUtf16(addressStr, env->GetStringLength(peerName));
        QBluetoothAddress address(strAddress);
        env->ReleaseStringChars(peerName, addressStr);
        return address;
    } else {
        return QBluetoothAddress();
    }
}

void QBluetoothLocalDevice::powerOn()
{
    JNIThreadHelper env;

    jmethodID enableID = env->GetMethodID(d_ptr->btAdapterClass, "enable", "()Z");
    jboolean ret = env->CallBooleanMethod(d_ptr->btAdapterObject, enableID);
    if (!ret)
        emit error(QBluetoothLocalDevice::UnknownError);
}

void QBluetoothLocalDevice::setHostMode(QBluetoothLocalDevice::HostMode mode)
{
    Q_UNUSED(mode);
    // TODO: That must be implemented over Actions framework
}

QBluetoothLocalDevice::HostMode QBluetoothLocalDevice::hostMode() const
{
    JNIThreadHelper env;

    jmethodID getScanModeID = env->GetMethodID(d_ptr->btAdapterClass, "getScanMode", "()I");
    jint scanMode = env->CallBooleanMethod(d_ptr->btAdapterObject, getScanModeID);
    switch (scanMode) {
        case 20:
            return HostPoweredOff;
        case 21:
            return HostConnectable;
        case 23:
            return HostDiscoverable;
        default: // HostDiscoverableLimitedInquiry is not supported
            return HostPoweredOff;
    }
}

QList<QBluetoothHostInfo> QBluetoothLocalDevice::allDevices()
{
    //TODO
    QList<QBluetoothHostInfo> localDevices;
    return localDevices;
}

void QBluetoothLocalDevice::requestPairing(const QBluetoothAddress &address, Pairing pairing)
{
    // Not supported on Android. Android does it automatically while connecting.
    emit error(PairingError);

    Q_UNUSED(address);
    Q_UNUSED(pairing);
}

QBluetoothLocalDevice::Pairing QBluetoothLocalDevice::pairingStatus(const QBluetoothAddress &address) const
{
    Q_UNUSED(address);
    return Unpaired;
}

void QBluetoothLocalDevice::pairingConfirmation(bool confirmation)
{
    Q_UNUSED(confirmation);
}

bool QBluetoothLocalDevicePrivate::isValid() const
{
    // TODO
    return true;
}

QT_END_NAMESPACE
