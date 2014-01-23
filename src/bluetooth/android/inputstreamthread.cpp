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

#include <QtAndroidExtras/QAndroidJniEnvironment>

#include "android/inputstreamthread_p.h"
#include "qbluetoothsocket_p.h"

QT_BEGIN_NAMESPACE


InputStreamThread::InputStreamThread(QBluetoothSocketPrivate *socket)
    : QThread(), m_stop(false), isError(false)
{
    m_socket_p = socket;
}

void InputStreamThread::run()
{
    qint32 byte;
    Q_UNUSED(byte)
    while (1) {
        QMutexLocker locker(&m_mutex);
        if (m_stop || isError)
            break;
        readFromInputStream();
    }

    if (m_socket_p->inputStream.isValid())
        m_socket_p->inputStream.callMethod<void>("close");
}

bool InputStreamThread::hasError() const
{
    QMutexLocker locker(&m_mutex);

    return isError;
}

bool InputStreamThread::bytesAvalable() const
{
    QMutexLocker locker(&m_mutex);
    return m_socket_p->buffer.size();
}

//This runs inside the mutex and thread.
void InputStreamThread::readFromInputStream()
{
    QAndroidJniEnvironment env;

    //TODO this is bad, we need proper blocking detection
    jint count = m_socket_p->inputStream.callMethod<jint>("available");
    if (count <= 0)
        return;

    int bufLen = 1000; // Seems to magical number that also low-end products can survive.
    jbyteArray nativeArray = env->NewByteArray(bufLen);
    qDebug() << "wwwwww";
    jint ret = m_socket_p->inputStream.callMethod<jint>("read", "([BII)I", nativeArray, 0, bufLen);

    qDebug() << "ddddd" << ret;
    if (env->ExceptionCheck() || ret < 0) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(nativeArray);
        isError = true;
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection);
        return;
    }

    if (ret == 0) {
        qDebug() << "Nothing to read";
        env->DeleteLocalRef(nativeArray);
        return;
    }

    char *writePtr = m_socket_p->buffer.reserve(bufLen);
    env->GetByteArrayRegion(nativeArray, 0, ret, reinterpret_cast<jbyte*>(writePtr));
    env->DeleteLocalRef(nativeArray);
    m_socket_p->buffer.chop(bufLen - ret);
    emit dataAvailable();
}

void InputStreamThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_stop = true;
}

qint64 InputStreamThread::readData(char *data, qint64 maxSize)
{
    QMutexLocker locker(&m_mutex);

    if (isError)
        return -1;

    if (!m_socket_p->buffer.isEmpty())
        return m_socket_p->buffer.read(data, maxSize);

    return 0;
}

QT_END_NAMESPACE
