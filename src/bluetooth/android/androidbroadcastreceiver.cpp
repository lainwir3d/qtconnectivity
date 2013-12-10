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
#include "android/androidbroadcastreceiver_p.h"
#include <qpa/qplatformnativeinterface.h>
#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>
#include <QtAndroidExtras/QAndroidJniEnvironment>

QT_BEGIN_NAMESPACE

class QPlatformNativeInterface;

static JNINativeMethod methods[] = {
    {"jniOnReceive",    "(ILandroid/content/Context;Landroid/content/Intent;)V",                    (void *)&Java_eu_licentia_necessitas_industrius_QtBroadcastReceiver_jniOnReceive},
};

AndroidBroadcastReceiver::AndroidBroadcastReceiver(QObject* parent)
    : QObject(parent), valid(false)
{
    //get QtActivity
    QPlatformNativeInterface* nativeInterface = QGuiApplication::platformNativeInterface();
    if (nativeInterface == NULL) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "QGuiApplication::platformNativeInterface(); returned NULL. Unable to initialize connectivity module.");
        return;
    }

    void* pointer = QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("QtActivity");
    if (pointer == NULL) {
        qWarning() << "nativeResourceForWidget(\"ApplicationContext\", 0)" << " - failed.";
        return;
    }

    jobject jActivityObject = reinterpret_cast<jobject>(pointer);
    activityObject = QAndroidJniObject(jActivityObject);

    broadcastReceiverObject = QAndroidJniObject("org/qtproject/qt5/android/bluetooth/QtBluetoothBroadcastReceiver");
    if (!broadcastReceiverObject.isValid())
        return;
    broadcastReceiverObject.setField<jint>("qtObject", reinterpret_cast<int>(this));

    QAndroidJniEnvironment env;
    jclass objectClass = env->GetObjectClass(broadcastReceiverObject.object<jobject>());
    jint res = env->RegisterNatives(objectClass, methods, 1);
    if (res < 0) {
        qWarning() << "Cannot register BroadcastReceiver::jniOnReceive";
        return;
    }

    intentFilterObject = QAndroidJniObject("android/content/IntentFilter");
    if (!intentFilterObject.isValid())
        return;

    valid = true;
}

AndroidBroadcastReceiver::~AndroidBroadcastReceiver()
{
    unregisterReceiver();
}

bool AndroidBroadcastReceiver::isValid() const
{
    return valid;
}

void AndroidBroadcastReceiver::unregisterReceiver()
{
    if (!valid)
        return;

    activityObject.callObjectMethod(
                "unregisterReceiver",
                "(Landroid/content/BroadcastReceiver;)V",
                broadcastReceiverObject.object<jobject>());
}

void AndroidBroadcastReceiver::addAction(const QString &action)
{
    if (!valid)
        return;

    QAndroidJniObject actionString = QAndroidJniObject::fromString(action);
    intentFilterObject.callMethod<void>("addAction", "(Ljava/lang/String;)V", actionString.object<jstring>());

    activityObject.callObjectMethod(
                "registerReceiver",
                "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;",
                broadcastReceiverObject.object<jobject>(),
                intentFilterObject.object<jobject>());
}

QT_END_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void JNICALL Java_eu_licentia_necessitas_industrius_QtBroadcastReceiver_jniOnReceive
  (JNIEnv *env, jobject javaObject, jint qtObject, jobject context, jobject intent) {
    Q_UNUSED(javaObject);
    reinterpret_cast<AndroidBroadcastReceiver*>(qtObject)->onReceive(env, context, intent);
}
#ifdef __cplusplus
}

#endif
