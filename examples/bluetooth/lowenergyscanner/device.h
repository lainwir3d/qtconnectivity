/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H

#include <qbluetoothglobal.h>
#include <qbluetoothlocaldevice.h>
#include <QObject>
#include <QVariant>
#include <QList>
#include <QBluetoothServiceDiscoveryAgent>
#include <QBluetoothDeviceDiscoveryAgent>
#include "deviceinfo.h"
#include "qlowenergyserviceinfo.h"
#include "serviceinfo.h"
#include "characteristicinfo.h"

QT_FORWARD_DECLARE_CLASS (QBluetoothDeviceInfo)
QT_FORWARD_DECLARE_CLASS (QLowEnergyServiceInfo)
QT_FORWARD_DECLARE_CLASS (QLowEnergyCharacteristicInfo)
QT_FORWARD_DECLARE_CLASS (QLowEnergyController)
QT_FORWARD_DECLARE_CLASS (QBluetoothServiceInfo)

class Device: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant devicesList READ getDevices NOTIFY devicesDone)
    Q_PROPERTY(QVariant servicesList READ getServices NOTIFY servicesDone)
    Q_PROPERTY(QVariant characteristicList READ getCharacteristics NOTIFY characteristicsDone)
    Q_PROPERTY(QString update READ getUpdate NOTIFY updateChanged)
    Q_PROPERTY(bool state READ state NOTIFY stateChanged)
public:
    Device();
    ~Device();
    QVariant getDevices();
    QVariant getServices();
    QVariant getCharacteristics();
    QString getUpdate();
    bool state();

public slots:
    void addDevice(const QBluetoothDeviceInfo&);
    void startDeviceDiscovery();
    void scanFinished();
    void deviceScanError(QBluetoothDeviceDiscoveryAgent::Error);
    void scanServices(QString address);
    void addLowEnergyService(const QLowEnergyServiceInfo&);
    void serviceScanDone();
    void serviceConnected(const QLowEnergyServiceInfo &service);
    void connectToService(const QString &uuid);
    void errorReceived(const QLowEnergyServiceInfo &service);
    void disconnectFromService();
    void serviceDisconnected(const QLowEnergyServiceInfo &service);
    void serviceScanError(QBluetoothServiceDiscoveryAgent::Error);

Q_SIGNALS:
    void devicesDone();
    void servicesDone();
    void characteristicsDone();
    void updateChanged();
    void stateChanged();

private:
    void setUpdate(QString message);
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    QBluetoothServiceDiscoveryAgent *serviceDiscoveryAgent;
    DeviceInfo currentDevice;
    QList<QObject*> devices;
    QList<QObject*> m_services;
    QList<QObject*> m_characteristics;
    QString m_message;
    bool connected;
    QLowEnergyController *info;
    bool m_deviceScanState;
};

#endif // DEVICE_H
