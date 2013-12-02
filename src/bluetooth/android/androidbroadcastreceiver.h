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

#ifndef JNIBROADCASTRECEIVER_H
#define JNIBROADCASTRECEIVER_H
#include <jni.h>
#include <QtCore>
#include <android/log.h>
#include "jnithreadhelper.h"

/* Header for class eu_licentia_necessitas_industrius_QtBroadcastReceiver */
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     eu_licentia_necessitas_industrius_QtBroadcastReceiver
 * Method:    jniOnReceive
 * Signature: (ILandroid/content/Context;Landroid/content/Intent;)V
 */
JNIEXPORT void JNICALL Java_eu_licentia_necessitas_industrius_QtBroadcastReceiver_jniOnReceive
  (JNIEnv *, jobject, jint, jobject, jobject);

#ifdef __cplusplus
}
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class AndroidBroadcastReceiver: public QObject
{
    Q_OBJECT
public:
    AndroidBroadcastReceiver(QObject* parent = 0);
    virtual ~AndroidBroadcastReceiver();

    void addAction(QString filter);
    static void initialize(JNIThreadHelper& environment, jclass appClass, jobject mainActivity);
    static void loadJavaClass(JNIEnv*);

private:
    friend void Java_eu_licentia_necessitas_industrius_QtBroadcastReceiver_jniOnReceive(JNIEnv *, jobject, jint, jobject, jobject);
    virtual void onReceive(JNIEnv *env, jobject context, jobject intent)=0;

    void unregisterReceiver();
    static void defineJavaClass(JNIThreadHelper&, jclass);

    jobject jQtBroadcastReceiverObject;
    jobject jIntentFilterObject;

    static jclass jIntentFilterClass;
    static jclass jQtBroadcastReceiverClass;
    static jobject jActivityObject;
};

QT_END_NAMESPACE

QT_END_HEADER


#endif // JNIBROADCASTRECEIVER_H
