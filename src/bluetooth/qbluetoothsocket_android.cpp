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

#include "qbluetoothsocket.h"
#include "qbluetoothsocket_p.h"
#include "qbluetoothaddress.h"
#include <QtCore/QLoggingCategory>
#include <QtCore/QTime>
#include <QtConcurrent/QtConcurrentRun>
#include <QtAndroidExtras/QAndroidJniEnvironment>


QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_BT_ANDROID)

QBluetoothSocketPrivate::QBluetoothSocketPrivate()
  : socket(-1),
    socketType(QBluetoothServiceInfo::UnknownProtocol),
    state(QBluetoothSocket::UnconnectedState),
    socketError(QBluetoothSocket::NoSocketError),
    connecting(false),
    discoveryAgent(0),
    inputThread(0)
{
    adapter = QAndroidJniObject::callStaticObjectMethod("android/bluetooth/BluetoothAdapter",
                                                        "getDefaultAdapter",
                                                        "()Landroid/bluetooth/BluetoothAdapter;");
    qRegisterMetaType<QBluetoothSocket::SocketError>("QBluetoothSocket::SocketError");
    qRegisterMetaType<QBluetoothSocket::SocketState>("QBluetoothSocket::SocketState");
}

QBluetoothSocketPrivate::~QBluetoothSocketPrivate()
{
}

bool QBluetoothSocketPrivate::ensureNativeSocket(QBluetoothServiceInfo::Protocol type)
{
    socketType = type;
    if (socketType == QBluetoothServiceInfo::RfcommProtocol)
        return true;

    return false;
}

//TODO Convert uuid parameter to const reference (affects QNX too)
void QBluetoothSocketPrivate::connectToService(const QBluetoothAddress &address, QBluetoothUuid uuid, QIODevice::OpenMode openMode)
{
    Q_Q(QBluetoothSocket);

    q->setSocketState(QBluetoothSocket::ConnectingState);
    QtConcurrent::run(this, &QBluetoothSocketPrivate::connectToServiceConc, address, uuid, openMode);
}

void QBluetoothSocketPrivate::connectToServiceConc(const QBluetoothAddress &address,
                                                   const QBluetoothUuid &uuid, QIODevice::OpenMode openMode)
{
    Q_Q(QBluetoothSocket);
    Q_UNUSED(openMode);

    if (!adapter.isValid()) {
        qCWarning(QT_BT_ANDROID) << "Device does not support Bluetooth";
        errorString = QBluetoothSocket::tr("Device does not support Bluetooth");
        q->setSocketError(QBluetoothSocket::NetworkError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    const int state = adapter.callMethod<jint>("getState");
    if (state != 12 ) { //BluetoothAdapter.STATE_ON
        qCWarning(QT_BT_ANDROID) << "Bt device offline";
        errorString = QBluetoothSocket::tr("Device is powered off.");
        q->setSocketError(QBluetoothSocket::NetworkError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    QAndroidJniEnvironment env;
    QAndroidJniObject inputString = QAndroidJniObject::fromString(address.toString());
    remoteDevice = adapter.callObjectMethod("getRemoteDevice",
                                            "(Ljava/lang/String;)Landroid/bluetooth/BluetoothDevice;",
                                            inputString.object<jstring>());
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        errorString = QBluetoothSocket::tr("Cannot access address %1").arg(address.toString());
        q->setSocketError(QBluetoothSocket::HostNotFoundError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    //cut leading { and trailing } {xxx-xxx}
    QString tempUuid = uuid.toString();
    tempUuid.chop(1); //remove trailing '}'
    tempUuid.remove(0, 1); //remove first '{'

    inputString = QAndroidJniObject::fromString(tempUuid);
    QAndroidJniObject uuidObject = QAndroidJniObject::callStaticObjectMethod("java/util/UUID", "fromString",
                                                                       "(Ljava/lang/String;)Ljava/util/UUID;",
                                                                       inputString.object<jstring>());
    socketObject = remoteDevice.callObjectMethod("createRfcommSocketToServiceRecord",
    //socketObject = remoteDevice.callObjectMethod("createInsecureRfcommSocketToServiceRecord",
                                                 "(Ljava/util/UUID;)Landroid/bluetooth/BluetoothSocket;",
                                                 uuidObject.object<jobject>());

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        socketObject = remoteDevice = QAndroidJniObject();
        errorString = QBluetoothSocket::tr("Cannot connect to %1 on %2",
                                           "%1 = uuid, %2 = Bt address").arg(uuid.toString()).arg(address.toString());
        q->setSocketError(QBluetoothSocket::ServiceNotFoundError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    socketObject.callMethod<void>("connect");
    if (env->ExceptionCheck() || socketObject.callMethod<jboolean>("isConnected") == JNI_FALSE) {
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        socketObject = remoteDevice = QAndroidJniObject();
        errorString = QBluetoothSocket::tr("Connection to service failed");
        q->setSocketError(QBluetoothSocket::ServiceNotFoundError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    inputStream = socketObject.callObjectMethod("getInputStream", "()Ljava/io/InputStream;");
    outputStream = socketObject.callObjectMethod("getOutputStream", "()Ljava/io/OutputStream;");

    if (env->ExceptionCheck() || !inputStream.isValid() || !outputStream.isValid()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        //close socket again
        socketObject.callMethod<void>("close");
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        socketObject = inputStream = outputStream = remoteDevice = QAndroidJniObject();


        errorString = QBluetoothSocket::tr("Obtaining streams for service failed");
        q->setSocketError(QBluetoothSocket::NetworkError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    if (inputThread) {
        inputThread->stop();
        inputThread->wait();
        delete inputThread;
    }
    inputThread = new InputStreamThread(this);
    QObject::connect(inputThread, SIGNAL(dataAvailable()), q, SIGNAL(readyRead()), Qt::QueuedConnection);
    QObject::connect(inputThread, SIGNAL(error()),
                     this, SLOT(inputThreadError()), Qt::QueuedConnection);
    inputThread->start();

    q->setSocketState(QBluetoothSocket::ConnectedState);
    emit q->connected();
}

void QBluetoothSocketPrivate::_q_writeNotify()
{
}

void QBluetoothSocketPrivate::_q_readNotify()
{
}

void QBluetoothSocketPrivate::abort()
{
    if (state == QBluetoothSocket::UnconnectedState)
        return;

    if (socketObject.isValid()) {
        QAndroidJniEnvironment env;

        if (inputThread) {
            inputThread->stop();
            inputThread->wait();
            delete inputThread;
            inputThread = 0;
        }

        socketObject.callMethod<void>("close");
        if (env->ExceptionCheck()) {
            qCWarning(QT_BT_ANDROID) << "Error during closure of socket";
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        inputStream = outputStream = socketObject = remoteDevice = QAndroidJniObject();

        Q_Q(QBluetoothSocket);
        emit q->disconnected();
    }
}

QString QBluetoothSocketPrivate::localName() const
{
    if (adapter.isValid())
        return adapter.callObjectMethod<jstring>("getName").toString();

    return QString();
}

QBluetoothAddress QBluetoothSocketPrivate::localAddress() const
{
    QString result;
    if (adapter.isValid())
        result = adapter.callObjectMethod("getAddress", "()Ljava/lang/String;").toString();

    return QBluetoothAddress(result);
}

quint16 QBluetoothSocketPrivate::localPort() const
{
    // Impossible to get channel number with current Android API (Levels 5 to 19)
    return 0;
}

QString QBluetoothSocketPrivate::peerName() const
{
    if (!remoteDevice.isValid())
        return QString();

    return remoteDevice.callObjectMethod("getName", "()Ljava/lang/String;").toString();
}

QBluetoothAddress QBluetoothSocketPrivate::peerAddress() const
{
    if (!remoteDevice.isValid())
        return QBluetoothAddress();

    const QString address = remoteDevice.callObjectMethod("getAddress",
                                                          "()Ljava/lang/String;").toString();

    return QBluetoothAddress(address);
}

quint16 QBluetoothSocketPrivate::peerPort() const
{
    // Impossible to get channel number with current Android API (Levels 5 to 13)
    return 0;
}

qint64 QBluetoothSocketPrivate::writeData(const char *data, qint64 maxSize)
{
    //TODO implement buffered behavior (so far only unbuffered)
    //TODO check that readData and writeData return -1 on error (on all platforms)
    Q_Q(QBluetoothSocket);
    if (state != QBluetoothSocket::ConnectedState || !outputStream.isValid()) {
        qCWarning(QT_BT_ANDROID) << "Socket::writeData: " << (int)state << outputStream.isValid() ;
        errorString = QBluetoothSocket::tr("Cannot write while not connected");
        q->setSocketError(QBluetoothSocket::OperationError);
        return -1;
    }

    QAndroidJniEnvironment env;
    jbyteArray nativeData = env->NewByteArray((qint32)maxSize);
    env->SetByteArrayRegion(nativeData, 0, (qint32)maxSize, reinterpret_cast<const jbyte*>(data));
    outputStream.callMethod<void>("write", "([BII)V", nativeData, 0, (qint32)maxSize);
    env->DeleteLocalRef(nativeData);

    if (env->ExceptionCheck()) {
        qCWarning(QT_BT_ANDROID) << "Error while writing";
        env->ExceptionDescribe();
        env->ExceptionClear();
        errorString = QBluetoothSocket::tr("Error during write on socket.");
        q->setSocketError(QBluetoothSocket::NetworkError);
        return -1;
    }

    return maxSize;
}

qint64 QBluetoothSocketPrivate::readData(char *data, qint64 maxSize)
{
    Q_Q(QBluetoothSocket);
    if (state != QBluetoothSocket::ConnectedState || !inputThread) {
        qCWarning(QT_BT_ANDROID) << "Socket::writeData: " << (int)state << outputStream.isValid() ;
        errorString = QBluetoothSocket::tr("Cannot write while not connected");
        q->setSocketError(QBluetoothSocket::OperationError);
        return -1;
    }

    return inputThread->readData(data, maxSize);
}

void QBluetoothSocketPrivate::inputThreadError()
{
    Q_Q(QBluetoothSocket);

    //any error from InputThread is a NetworkError
    errorString = QBluetoothSocket::tr("Network error during read");
    q->setSocketError(QBluetoothSocket::NetworkError);
}

void QBluetoothSocketPrivate::close()
{
    /* This function is called by QBluetoothSocket::close and softer version
       QBluetoothSocket::disconnectFromService() which difference I do not quite fully understand.
       Anyways we end up in Android "close" function call.
       */
    abort();
}

bool QBluetoothSocketPrivate::setSocketDescriptor(int socketDescriptor, QBluetoothServiceInfo::Protocol socketType,
                         QBluetoothSocket::SocketState socketState, QBluetoothSocket::OpenMode openMode)
{
    Q_UNUSED(socketDescriptor);
    Q_UNUSED(socketType)
    Q_UNUSED(socketState);
    Q_UNUSED(openMode);
    qCWarning(QT_BT_ANDROID) << "No socket descriptor support on Android.";
    return false;
}

int QBluetoothSocketPrivate::socketDescriptor() const
{
    return 0;
}

qint64 QBluetoothSocketPrivate::bytesAvailable() const
{
    //We cannot access buffer directly as it is part of different thread
    if (inputThread)
        return inputThread->bytesAvalable();

    return 0;
}

QT_END_NAMESPACE
