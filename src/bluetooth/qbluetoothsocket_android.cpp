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
#include "qbluetoothlocaldevice_p.h"
#include <QTime>
#include <string.h>
#include <QtConcurrentRun>
#include "android/jnithreadhelper.h"


QT_BEGIN_NAMESPACE

static jclass btAdapterClass;
static jobject btAdapterObject;


/*
jobject defineClass() {

    jclass dexClassLoaderClass = m_env->FindClass("dalvik/system/DexClassLoader");
    jmethodID dexClassLoaderConstructorID = m_env->GetMethodID(dexClassLoaderClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V"); // no parameters

    jstring jarFile = m_env->NewStringUTF("/data/local/qt/lib/JNIRunnable.jar");
    jstring temp = m_env->NewStringUTF("/data/local/qt/temp");
    jstring className = m_env->NewStringUTF("JNIRunnable");

    jclass classLoaderClass = m_env->FindClass("java/lang/ClassLoader");
    jmethodID getSystemClassLoaderID = m_env->GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");

    jobject systemClassLoaderObject = m_env->CallStaticObjectMethod(classLoaderClass, getSystemClassLoaderID);

    jobject dexClassLoaderObject = m_env->NewObject(dexClassLoaderClass, dexClassLoaderConstructorID, jarFile,temp, temp, systemClassLoaderObject);

    jmethodID loadClassID = m_env->GetMethodID(dexClassLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    jobject jniRunnableClassObject = m_env->CallObjectMethod(dexClassLoaderObject, loadClassID, className);

    jclass classClass = m_env->FindClass("java/lang/Class");
    jmethodID newInstanceID = m_env->GetMethodID(classClass, "newInstance", "()Ljava/lang/Object;");
    QtConnectivityJni::javaRunnableObject = m_env->CallObjectMethod(jniRunnableClassObject, newInstanceID);
    QtConnectivityJni::javaRunnableObject = m_env->NewGlobalRef(QtConnectivityJni::javaRunnableObject);


    //jmethodID getFieldID = m_env->GetMethodID(classClass, "getDeclaredField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
    //jstring fieldName = m_env->NewStringUTF("qtObject");
    //jobject fieldReflectedObject = m_env->CallObjectMethod(jniRunnableClassObject, getFieldID, fieldName);
    //jfieldID qtObjectFieldID = m_env->FromReflectedField(fieldReflectedObject);
    //env->SetIntField(QtConnectivityJni::javaRunnableObject, qtObjectFieldID, reinterpret_cast<int>(this));


    QtConnectivityJni::javaRunnableClass = m_env->GetObjectClass(QtConnectivityJni::javaRunnableObject);
    m_env->RegisterNatives(QtConnectivityJni::javaRunnableClass, methods, 1);
    loadDefaultAdapter();
    // QtConnectivityJni::env->FindClass() doesn't work on the loaded class so we use reflection API instead and return Class<JNIBroadcastReceiver> object.
    return jniRunnableClassObject;
}
*/
QBluetoothSocketPrivate::QBluetoothSocketPrivate():
    remoteDeviceObject(0),
    socketObject(0),
    inputStream(0),
    outputStream(0)
{
    btAdapterClass = QBluetoothLocalDevicePrivate::btAdapterClass;
    btAdapterObject = QBluetoothLocalDevicePrivate::btAdapterObject;

    //W/Qt      ( 1111): QObject::connect: Cannot queue arguments of type 'QBluetoothSocket::SocketError'
    //W/Qt      ( 1111): (Make sure 'QBluetoothSocket::SocketError' is registered using qRegisterMetaType().)
    qRegisterMetaType<QBluetoothSocket::SocketError>("QBluetoothSocket::SocketError");
}

QBluetoothSocketPrivate::~QBluetoothSocketPrivate()
{
    JNIThreadHelper env;
    if (remoteDeviceObject != 0)
        env->DeleteGlobalRef(remoteDeviceObject);

    if (socketObject != 0)
        env->DeleteGlobalRef(socketObject);

    if (inputStream != 0)
        env->DeleteGlobalRef(inputStream);

    if (outputStream != 0)
        env->DeleteGlobalRef(outputStream);
}

bool QBluetoothSocketPrivate::ensureNativeSocket(QBluetoothServiceInfo::Protocol type)
{
    Q_UNUSED(type);
    return false;
}

void QBluetoothSocketPrivate::connectToService(const QBluetoothAddress &address, quint16 port, QIODevice::OpenMode openMode)
{
    QtConcurrent::run(this, &QBluetoothSocketPrivate::connectToServiceConc, address, port, openMode);
}

void QBluetoothSocketPrivate::connectToServiceConc(const QBluetoothAddress &address, quint16 port, QIODevice::OpenMode openMode)
{
    Q_UNUSED(openMode);
    //Q_UNUSED(address);
    Q_UNUSED(port);
    Q_Q(QBluetoothSocket);

    JNIThreadHelper env;

    jmethodID getRemoteDeviceID = env->GetMethodID(btAdapterClass, "getRemoteDevice", "(Ljava/lang/String;)Landroid/bluetooth/BluetoothDevice;");

    jstring jni_address = env->NewStringUTF(address.toString().toUtf8());

    remoteDeviceObject = env->CallObjectMethod(btAdapterObject, getRemoteDeviceID, jni_address);
    remoteDeviceObject = (jobject)env->NewGlobalRef(remoteDeviceObject);

    jclass bluetoothDeviceClass = env->FindClass("android/bluetooth/BluetoothDevice");

    jmethodID  createSocketID = env->GetMethodID(bluetoothDeviceClass,"createRfcommSocketToServiceRecord","(Ljava/util/UUID;)Landroid/bluetooth/BluetoothSocket;");

    jclass uuidClass = env->FindClass("java/util/UUID");
    jmethodID fromStringID = env->GetStaticMethodID(uuidClass, "fromString", "(Ljava/lang/String;)Ljava/util/UUID;");

    jstring uuid = env->NewStringUTF("00001101-0000-1000-8000-00805F9B34FB");
    jobject uuidObject = env->CallStaticObjectMethod(uuidClass, fromStringID, uuid);

    socketObject = env->CallObjectMethod(remoteDeviceObject, createSocketID, uuidObject);
    socketObject = (jobject)env->NewGlobalRef(socketObject);

    emit q->stateChanged(QBluetoothSocket::ConnectingState);

    jclass socketClass = env->FindClass("android/bluetooth/BluetoothSocket");
    jmethodID connectID = env->GetMethodID(socketClass, "connect", "()V");

    env->CallVoidMethod(socketObject, connectID);

    if (!env->ExceptionCheck()) {
        jmethodID getInputStreamID = env->GetMethodID(socketClass, "getInputStream", "()Ljava/io/InputStream;");
        jmethodID getOutputStreamID = env->GetMethodID(socketClass, "getOutputStream", "()Ljava/io/OutputStream;");

        inputStream = env->CallObjectMethod(socketObject, getInputStreamID);
        outputStream = env->CallObjectMethod(socketObject, getOutputStreamID);
        inputStream = (jobject)env->NewGlobalRef(inputStream);
        outputStream = (jobject)env->NewGlobalRef(outputStream);


        q->setSocketState(QBluetoothSocket::ConnectedState);
        inputThread = new InputStreamThread(this);
        inputThread->start();

        emit q->connected();
    } else {
        socketError = QBluetoothSocket::UnknownSocketError;
        emit q->error(socketError);
        env->ExceptionClear();
    }
}

void QBluetoothSocketPrivate::_q_writeNotify()
{
}

void QBluetoothSocketPrivate::_q_readNotify()
{
}

void QBluetoothSocketPrivate::abort()
{
    if (socketObject != 0) {
        JNIThreadHelper env;

        jclass socketClass = env->FindClass("android/bluetooth/BluetoothSocket");
        jmethodID closeID = env->GetMethodID(socketClass, "close", "()V");
        env->CallVoidMethod(socketObject, closeID);
    }
}

QString QBluetoothSocketPrivate::localName() const
{
    QBluetoothLocalDevice device;
    return device.name();
}

QBluetoothAddress QBluetoothSocketPrivate::localAddress() const
{
    QBluetoothLocalDevice device;
    return device.address();
}

quint16 QBluetoothSocketPrivate::localPort() const
{
    // Impossible to get channel number with current Android API (Levels 5 to 13)
    return 0;
}

QString QBluetoothSocketPrivate::peerName() const
{
    if (remoteDeviceObject != 0) {
        JNIThreadHelper env;

        /*static*/ jclass bluetoothDeviceClass = env->FindClass("android/bluetooth/BluetoothDevice");
        /*static*/ jmethodID getNameID = env->GetMethodID(bluetoothDeviceClass, "getName", "()Ljava/lang/String;");
        jstring peerName = (jstring) env->CallObjectMethod(remoteDeviceObject, getNameID);

        jboolean isCopy;
        const jchar* name = env->GetStringChars(peerName, &isCopy);
        QString qtName = QString::fromUtf16(name, env->GetStringLength(peerName));
        env->ReleaseStringChars(peerName, name);
        return qtName;
    } else {
        return QString();
    }
}

QBluetoothAddress QBluetoothSocketPrivate::peerAddress() const
{
    if (remoteDeviceObject != 0) {
        JNIThreadHelper env;

        /*static*/ jclass bluetoothDeviceClass = env->FindClass("android/bluetooth/BluetoothDevice");
        /*static*/ jmethodID getNameID = env->GetMethodID(bluetoothDeviceClass, "getAddress", "()Ljava/lang/String;");
        jstring peerName = (jstring) env->CallObjectMethod(remoteDeviceObject, getNameID);

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

quint16 QBluetoothSocketPrivate::peerPort() const
{
    // Impossible to get channel number with current Android API (Levels 5 to 13)
    return 0;
}

qint64 QBluetoothSocketPrivate::writeData(const char *data, qint64 maxSize)
{
    if (state != QBluetoothSocket::ConnectedState) return 0;

    JNIThreadHelper env;

    qint32 maxSizeInt = maxSize;
    /*static*/ jclass outputStreamClass = env->FindClass("java/io/OutputStream");
    /*static*/ jmethodID writeDataID = env->GetMethodID(outputStreamClass, "write", "([BII)V");

    jbyteArray dataArray = env->NewByteArray(maxSizeInt);
    env->SetByteArrayRegion(dataArray, 0, maxSizeInt, reinterpret_cast<const jbyte*>(data));
    env->CallVoidMethod(outputStream, writeDataID, dataArray, 0, maxSizeInt);

    if (env->ExceptionCheck() == JNI_TRUE)
        env->ExceptionClear();

    return maxSizeInt;
}

qint64 QBluetoothSocketPrivate::readData(char *data, qint64 maxSize)
{
    if (!buffer.isEmpty()) {
        int i = buffer.read(data, maxSize);
        return i;
    }
    return 0;
}

qint32 QBluetoothSocketPrivate::read()
{
    Q_Q(QBluetoothSocket);
    JNIThreadHelper env;

    /*static*/ jclass inputStreamClass = env->FindClass("java/io/InputStream");
    /*static*/ jmethodID readID = env->GetMethodID(inputStreamClass, "read", "([BII)I");
    int bufLen = 1000; // Seems to magical number that also low-end products can survive.
    jint ret=0;
    jbyteArray dataArray = env->NewByteArray(bufLen);
    ret = env->CallIntMethod(inputStream, readID, dataArray, 0, bufLen);
    if (ret > 0 && env->ExceptionCheck() == JNI_FALSE) {
        char *writePointer = buffer.reserve(bufLen);
        env->GetByteArrayRegion(dataArray, 0, ret, reinterpret_cast<jbyte*>(writePointer));
        env->DeleteLocalRef(dataArray);
        buffer.chop(bufLen - (ret < 0 ? 0 : ret));
        emit q->readyRead();
    } else if (ret < 0 || env->ExceptionCheck() == JNI_TRUE) {
        emit q->error(QBluetoothSocket::UnknownSocketError);
        q->disconnectFromService();
        q->setSocketState(QBluetoothSocket::UnconnectedState);
    } else {
        qWarning() << "java/io/InputStream::read([BII)I returned zero bytes.";
    }
    if (env->ExceptionCheck() == JNI_TRUE)
        env->ExceptionClear();

    return ret;
}

void QBluetoothSocketPrivate::close()
{
    /* This function is called by QBluetoothSocket::close and softer version
       QBluetoothSocket::disconnectFromService() which difference I do not quite fully understand.
       Anyways we end up in Android "close" function call.
       */
    abort();
}
/*
void QBluetoothSocketPrivate::jniCallback(){
    //ATTACH_THREAD
    qDebug() << "new QBluetoothSocketPrivate";
    QtConnectivityJni::btAdapterClass = QtConnectivityJni::env->FindClass("android/bluetooth/BluetoothAdapter");
    if (QtConnectivityJni::btAdapterClass == NULL)
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Native registration unable to find class android/bluetooth/BluetoothAdapter");

    jmethodID getDefaultAdapterID = QtConnectivityJni::env->GetStaticMethodID(QtConnectivityJni::btAdapterClass, "getDefaultAdapter", "()Landroid/bluetooth/BluetoothAdapter;");
    if (getDefaultAdapterID == NULL)
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Native registration unable to get method ID");

    qDebug() << "Hakkame nyyd looma";
    QtConnectivityJni::btAdapterObject = QtConnectivityJni::env->CallStaticObjectMethod(QtConnectivityJni::btAdapterClass, getDefaultAdapterID);
    if (QtConnectivityJni::btAdapterObject == NULL)i {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Device does not support Bluetooth");
        //return JNI_FALSE;
    } else {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "BluetoothAdapter returned.");
    }
    QtConnectivityJni::btAdapterObject = QtConnectivityJni::env->NewGlobalRef(QtConnectivityJni::btAdapterObject);
    //QtConnectivityJni::m_javaVM->DetachCurrentThread();
}*/

bool QBluetoothSocketPrivate::setSocketDescriptor(int socketDescriptor, QBluetoothServiceInfo::Protocol socketType,
                         QBluetoothSocket::SocketState socketState, QBluetoothSocket::OpenMode openMode)
{
    Q_UNUSED(socketDescriptor);
    Q_UNUSED(socketType)
    Q_UNUSED(socketState);
    Q_UNUSED(openMode);
    return false;
}

int QBluetoothSocketPrivate::socketDescriptor() const
{
    return 0;
}

qint64 QBluetoothSocketPrivate::bytesAvailable() const
{
    return buffer.size();
}

QT_END_NAMESPACE
