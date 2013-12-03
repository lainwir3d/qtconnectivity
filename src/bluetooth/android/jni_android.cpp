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

#include "android/jnithreadhelper_p.h"
#include <qbluetoothlocaldevice_p.h>
#include <android/log.h>

Q_DECL_EXPORT JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    void* venv = NULL;
    JNIEnv* nativeEnvironment = NULL;

    if (vm->GetEnv(&venv, JNI_VERSION_1_4) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_FATAL,"Qt","GetEnv failed");
        return -1;
    }
    nativeEnvironment = reinterpret_cast<JNIEnv*>(venv);

    JNIThreadHelper::setJvm(vm);
    jclass qtApplicationC;

    qtApplicationC = nativeEnvironment->FindClass("android/app/Application");
    if (qtApplicationC == NULL)
    {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Native registration unable to find class android.app.Application");
        return JNI_FALSE;
    } else {
        qtApplicationC = (jclass)nativeEnvironment->NewGlobalRef(qtApplicationC);
    }
    //JNIThreadHelper::setAppContextClass(qtApplicationC);
    jclass jQtBroadcastReceiverClass = nativeEnvironment->FindClass("org/qtproject/qt5/android/bluetooth/QtBluetoothBroadcastReceiver");
    if (jQtBroadcastReceiverClass == NULL) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt","Cannot find org/qtproject/qt5/android/bluetooth/QtBluetoothBroadcastReceiver111");
        nativeEnvironment->ExceptionClear();
        nativeEnvironment->DeleteLocalRef(jQtBroadcastReceiverClass);
        return JNI_VERSION_1_4;
    }
    __android_log_print(ANDROID_LOG_FATAL,"Qt","Cannot find org/qtproject/qt5/android/bluetooth/QtBluetoothBroadcastReceiver2222");

    /*
    jclass qtNativeClass = nativeEnvironment->FindClass("org/kde/necessitas/industrius/QtNative");
    jmethodID activityID = nativeEnvironment->GetStaticMethodID(qtNativeClass, "activity", "()Landroid/app/Activity;");
    jobject mainContext = nativeEnvironment->CallStaticObjectMethod(qtNativeClass, activityID);
    */

    return JNI_VERSION_1_4;
}
