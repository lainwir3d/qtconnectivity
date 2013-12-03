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

package org.qtproject.qt5.android.bluetooth;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

public class QtBluetoothBroadcastReceiver extends BroadcastReceiver
{
    /* Pointer to the Qt object that "owns" the Java object */
    int qtObject = 0;
    static Activity qtactivity = null;

    private static final int TURN_BT_ON = 3330;
    private static final int TURN_BT_DISCOVERABLE = 3331;

    public void onReceive(Context context, Intent intent)
    {
        jniOnReceive(qtObject, context, intent);
    }

    public native void jniOnReceive(int qtObject, Context context, Intent intent);

    /*static public void myregister()
    {
        try {
            QtBluetoothBroadcastReceiver receiver = new QtBluetoothBroadcastReceiver();
            Intent i = qtactivity.registerReceiver(receiver,
                    new IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED));
            System.out.println("1 " + i);
            i = qtactivity.registerReceiver(receiver,
                    new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED));
            System.out.println("3 " + i);
            i = qtactivity.registerReceiver(receiver,
                    new IntentFilter(BluetoothDevice.ACTION_FOUND));
            System.out.println("4 " + i);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }*/

    static public void setActivity(Activity activity, Object activityDelegate)
    {
        qtactivity = activity;
    }

    /*static public void test()
    {
        try {
            System.out.println("--------------------------------------");
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            qtactivity.startActivityForResult(enableBtIntent, 33);
        } catch (Exception ex) {
           ex.printStackTrace();
        }
    }*/

    static public void setDiscoverable()
    {
        Intent intent = new Intent(BluetoothAdapter.ACTION_REQUEST_DISCOVERABLE);
        intent.putExtra(BluetoothAdapter.EXTRA_DISCOVERABLE_DURATION, 300);
        try {
            qtactivity.startActivityForResult(intent, TURN_BT_ON);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    static public void setConnectable()
    {
        Intent intent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
        try {
            qtactivity.startActivityForResult(intent, TURN_BT_DISCOVERABLE);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

}
