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

#include "android/devicediscoverybroadcastreceiver_p.h"
#include "qbluetoothdeviceinfo.h"
#include "qbluetoothaddress.h"


DeviceDiscoveryBroadcastReceiver::DeviceDiscoveryBroadcastReceiver(QObject* parent): AndroidBroadcastReceiver(parent)
{
   addAction("android.bluetooth.device.action.FOUND");
   addAction("android.bluetooth.adapter.action.DISCOVERY_STARTED");
   addAction("android.bluetooth.adapter.action.DISCOVERY_FINISHED");
}

void DeviceDiscoveryBroadcastReceiver::onReceive(JNIEnv *env, jobject context, jobject intent)
{
    __android_log_print(ANDROID_LOG_DEBUG,"Qt", "DeviceDiscoveryBroadcastReceiver::onReceive() - event.");

    // Cannot be static unfortunately because Android moves memory around now.
    /*static */jclass intentClass = env->FindClass("android/content/Intent");

    jmethodID getParcelableExtraID = env->GetMethodID(intentClass, "getParcelableExtra", "(Ljava/lang/String;)Landroid/os/Parcelable;");

    jmethodID getActionID = env->GetMethodID(intentClass, "getAction", "()Ljava/lang/String;");
    jstring actionJString = (jstring)env->CallObjectMethod(intent, getActionID);
    jboolean isCopy;
    const jchar* actionChar = env->GetStringChars(actionJString, &isCopy);
    QString actionString = QString::fromUtf16(actionChar, env->GetStringLength(actionJString));
    env->ReleaseStringChars(actionJString, actionChar);

    if (actionString == "android.bluetooth.adapter.action.DISCOVERY_FINISHED" ) {
        emit finished();
    } else if (actionString == "android.bluetooth.adapter.action.DISCOVERY_STARTED" ) {

    } else if (actionString == "android.bluetooth.device.action.FOUND") {
        jstring DEVICE = env->NewStringUTF("android.bluetooth.device.extra.DEVICE");
        jobject bluetoothDeviceObject = env->CallObjectMethod(intent, getParcelableExtraID, DEVICE);

        /*static */jclass bluetoothDeviceClass = env->FindClass("android/bluetooth/BluetoothDevice");
        /*static */jclass bluetoothClassClass = env->FindClass("android/bluetooth/BluetoothClass");

        static jmethodID getNameID = env->GetMethodID(bluetoothDeviceClass, "getName", "()Ljava/lang/String;");
        static jmethodID getAddressID = env->GetMethodID(bluetoothDeviceClass, "getAddress", "()Ljava/lang/String;");
        static jmethodID getBluetoothClassID = env->GetMethodID(bluetoothDeviceClass, "getBluetoothClass", "()Landroid/bluetooth/BluetoothClass;");
        static jmethodID getDeviceClassID = env->GetMethodID(bluetoothClassClass, "getDeviceClass", "()I");

        if (bluetoothDeviceObject != NULL) {
            // Name
            jstring deviceName = (jstring) env->CallObjectMethod(bluetoothDeviceObject, getNameID);
            const jchar* name = env->GetStringChars(deviceName, &isCopy);
            QString qtName = QString::fromUtf16(name, env->GetStringLength(deviceName));
            env->ReleaseStringChars(deviceName, name);

            // Address
            jstring deviceAddress = (jstring) env->CallObjectMethod(bluetoothDeviceObject, getAddressID);
            const jchar* address = env->GetStringChars(deviceAddress, &isCopy);
            QString qtAddress = QString::fromUtf16(address, env->GetStringLength(deviceAddress));
            env->ReleaseStringChars(deviceAddress, address);

            // Device Classes
            jobject bluetoothClassObject = env->CallObjectMethod(bluetoothDeviceObject, getBluetoothClassID);
            jint classesInt = env->CallIntMethod(bluetoothClassObject, getDeviceClassID);

            QBluetoothDeviceInfo info(QBluetoothAddress(qtAddress), qtName, classesInt);
            emit deviceDiscovered(info);
        }
    }
}

//#include "moc_devicediscoverybroadcastreceiver.cpp"

