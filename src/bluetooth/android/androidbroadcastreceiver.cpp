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
#include "android/jnithreadhelper_p.h"
//#include <QtGui/QPlatformNativeInterface>
#include <qpa/qplatformnativeinterface.h>
#include <QtGui/QGuiApplication>

class QPlatformNativeInterface;

static JNINativeMethod methods[] = {
    {"jniOnReceive",    "(ILandroid/content/Context;Landroid/content/Intent;)V",                    (void *)&Java_eu_licentia_necessitas_industrius_QtBroadcastReceiver_jniOnReceive},
};

jobject AndroidBroadcastReceiver::jActivityObject = NULL;
jclass AndroidBroadcastReceiver::jQtBroadcastReceiverClass = NULL;
jclass AndroidBroadcastReceiver::jIntentFilterClass = NULL;

void AndroidBroadcastReceiver::initialize(JNIThreadHelper& env, jclass appClass, jobject mainActivity)
{
    jActivityObject = mainActivity;

    jIntentFilterClass = env->FindClass("android/content/IntentFilter");

    if (jIntentFilterClass == NULL) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt","Cannot find android.content.IntentFilter");
    } else {
        jIntentFilterClass = (jclass)env->NewGlobalRef(jIntentFilterClass);
    }
    env->RegisterNatives(jQtBroadcastReceiverClass, methods, 1);
}

void AndroidBroadcastReceiver::loadJavaClass(JNIEnv *env)
{
    jQtBroadcastReceiverClass = env->FindClass("org/qtproject/qt5/android/QtBroadcastReceiver");
    if (jQtBroadcastReceiverClass == NULL) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt","Cannot find org/qtproject/qt5/android/QtBroadcastReceiver");
        env->ExceptionClear();
    } else {
        jQtBroadcastReceiverClass = (jclass)env->NewGlobalRef(jQtBroadcastReceiverClass);
    }
}

AndroidBroadcastReceiver::AndroidBroadcastReceiver(QObject* parent): QObject(parent)
{
    if (jQtBroadcastReceiverClass != NULL) {
        JNIThreadHelper env;

        if (jActivityObject==NULL) {
            QPlatformNativeInterface* nativeInterface = QGuiApplication::platformNativeInterface();
            if (nativeInterface == NULL) {
                __android_log_print(ANDROID_LOG_FATAL,"Qt", "QGuiApplication::platformNativeInterface(); returned NULL. Unable to initialize connectivity module.");
                goto error;
            }
            void* pointer = QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("QtActivity");
            if (pointer == NULL) {
                qDebug() << "nativeResourceForWidget(\"ApplicationContext\", 0)" << " - failed.";
                __android_log_print(ANDROID_LOG_FATAL,"Qt", "nativeResourceForWidget(\"ApplicationContext\", 0) returned NULL\nProbably QtGui lib is too old.");
                goto error;
            } else {
                jActivityObject = reinterpret_cast<jobject>(pointer);
                //Lauri - questionalbe if this is already a global ref
                jActivityObject = (jclass)env->NewGlobalRef(jActivityObject);

                initialize(env, JNIThreadHelper::appContextClass(), jActivityObject);
            }
        }

        jmethodID jQtBroadcastReceiverConstructorID = env->GetMethodID(jQtBroadcastReceiverClass, "<init>", "()V"); // no parameters
        jQtBroadcastReceiverObject = env->NewObject(jQtBroadcastReceiverClass, jQtBroadcastReceiverConstructorID);
        jQtBroadcastReceiverObject = env->NewGlobalRef(jQtBroadcastReceiverObject);

        jfieldID jQtObjectFieldID = env->GetFieldID(jQtBroadcastReceiverClass, "qtObject", "I");
        env->SetIntField(jQtBroadcastReceiverObject, jQtObjectFieldID, reinterpret_cast<int>(this));

        jmethodID intentFilterConstructorID = env->GetMethodID(jIntentFilterClass, "<init>", "()V"); // no parameters
        jIntentFilterObject = env->NewObject(jIntentFilterClass, intentFilterConstructorID);
        jIntentFilterObject = env->NewGlobalRef(jIntentFilterObject);
    }
    error:;
}

AndroidBroadcastReceiver::~AndroidBroadcastReceiver()
{
    unregisterReceiver();

    JNIThreadHelper env;
    env->DeleteGlobalRef(jQtBroadcastReceiverObject);
    env->DeleteGlobalRef(jIntentFilterObject);
}

void AndroidBroadcastReceiver::unregisterReceiver()
{
    if (jActivityObject==NULL) return;

    JNIThreadHelper env;
    jclass jContextClass = env->FindClass("android/app/Application");
    jmethodID unregisterReceiverMethodID = env->GetMethodID(jContextClass, "unregisterReceiver", "(Landroid/content/BroadcastReceiver;)V");
    env->CallObjectMethod(jActivityObject, unregisterReceiverMethodID, jQtBroadcastReceiverObject);
}

void AndroidBroadcastReceiver::addAction(QString action)
{
    if (jActivityObject==NULL) return;

    JNIThreadHelper env;
    static jmethodID jAddActionID = env->GetMethodID(jIntentFilterClass, "addAction", "(Ljava/lang/String;)V");
    jstring filterStringUTF = env->NewStringUTF(action.toUtf8()); // Lauri - toAscii replaces by utf8
    env->CallVoidMethod(jIntentFilterObject, jAddActionID, filterStringUTF);
    env->DeleteLocalRef(filterStringUTF);

    jclass jContextClass = env->FindClass("android/app/Application");
    jmethodID registerReceiverMethodID = env->GetMethodID(jContextClass, "registerReceiver", "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;");
    env->CallObjectMethod(jActivityObject, registerReceiverMethodID, jQtBroadcastReceiverObject, jIntentFilterObject);
}

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

//#include "moc_androidbroadcastreceiver.cpp"
