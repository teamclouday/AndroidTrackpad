package com.example.trackpad;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Set;
import java.util.UUID;

// reference: https://developer.android.com/guide/topics/connectivity/bluetooth#SettingUp

public class myBthManager
{
    private Activity myActivity;
    private BluetoothAdapter myAdapter;
    private BluetoothDevice targetDevice = null;
    private BluetoothSocket mySocket = null;

    private final int REQUEST_ENABLE_BIT = 1;
    private final UUID CONNECTION_UUID = UUID.fromString("97c4eab8-234b-42d0-8c09-e9a5b1f1ba5b");
    private final String logTag = "TrackpadBth";

    public static MainActivity.BufferData buffer = new MainActivity.BufferData();

    public boolean initialized = false;
    public boolean connected = false;
    public String error_msg = "";

    // set broadcast receiver
    private final BroadcastReceiver myReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if(BluetoothAdapter.ACTION_STATE_CHANGED.equals(action))
            {
                // disconnect if adapter is turning off
                int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
                if (state == BluetoothAdapter.STATE_TURNING_OFF)
                {
                    disconnect();
                    myActivity.findViewById(R.id.radioButtonBth).setClickable(true);

                }
            }
            else if(BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(action))
            {
                // disconnect if ACL is disconnected
                disconnect();
            }
            else if(BluetoothDevice.ACTION_ACL_DISCONNECT_REQUESTED.equals(action))
            {
                // disconnect if ACL is requesting disconnected
                disconnect();
            }
        }
    };

    public myBthManager(Activity current_activity)
    {
        myActivity = current_activity;
        // initialize bluetooth adapter
        myAdapter = BluetoothAdapter.getDefaultAdapter();
        if (myAdapter == null)
        {
            error_msg = "Error: Device does not support bluetooth";
            return;
        }
        initialize();
        // register filter
        IntentFilter filter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECT_REQUESTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        myActivity.registerReceiver(myReceiver, filter);
    }

    public void connect()
    {
        BluetoothSocket tmp;
        try
        {
            tmp = targetDevice.createRfcommSocketToServiceRecord(CONNECTION_UUID);
        }
        catch (IOException e)
        {
            Log.d(logTag, "testDeviceConnection: paired PC MAC address = " + targetDevice.getAddress() + " Name = " + targetDevice.getName() + " cannot create socket with UUID");
            return;
        }
        mySocket = tmp;
        try
        {
            mySocket.connect();
        }
        catch (IOException connectExp)
        {
            Log.d(logTag, "testDeviceConnection: paired PC MAC address = " + targetDevice.getAddress() + " Name = " + targetDevice.getName() + "socket failed to connect");
            try
            {
                mySocket.close();
            }
            catch (IOException closeExp)
            {
                Log.d(logTag, "testDeviceConnection: paired PC MAC address = " + targetDevice.getAddress() + " Name = " + targetDevice.getName() + "socket failed to close");
            }
            mySocket = null;
            return;
        }
        connected = true;
    }

    public synchronized void addData(MainActivity.BufferSingle data)
    {
        buffer.addData(data);
    }

    public boolean sendMessage()
    {
        if(!connected) return false;
        // try to get output stream
        OutputStream stream;
        try
        {
            stream = mySocket.getOutputStream();
        }
        catch(IOException e)
        {
            error_msg = "Error: failed to get output stream from bluetooth socket";
            return false;
        }
        // prepare data to send
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        MainActivity.BufferSingle data = null;
        do
        {
            data = buffer.getData();
            if(data == null) break;
            // init a byte buffer
            ByteBuffer bb = ByteBuffer.allocate(4);
            // set message start indicator
            bb.putInt(10086);
            outputStream.write(bb.array(), 0, 4);
            bb.clear();
            // set type as int
            bb.putInt(data.type.getValue());
            outputStream.write(bb.array(), 0, 4);
            bb.clear();
            // set posX
            bb.putInt(data.posX);
            outputStream.write(bb.array(), 0, 4);
            bb.clear();
            // set posY
            bb.putInt(data.posY);
            outputStream.write(bb.array(), 0, 4);
            bb.clear();
            // set time
            bb.putInt(data.time);
            outputStream.write(bb.array(), 0, 4);
        }while(true);
        try
        {
            stream.write(outputStream.toByteArray());
        }
        catch(IOException e)
        {
            error_msg = "Error: failed to write to output stream in bluetooth socket";
            return false;
        }
        return true;
    }

    public void disconnect()
    {
        if (mySocket == null)
        {
            connected = false;
            return;
        }
        try
        {
            mySocket.close();
        }
        catch (IOException e)
        {
            Log.d(logTag, "testDeviceConnection: paired PC MAC address = " + targetDevice.getAddress() + " Name = " + targetDevice.getName() + "socket failed to close");
        }
        mySocket = null;
        connected = false;
    }

    public String getDeviceInfo()
    {
        if (targetDevice == null) return "";
        return "MAC = " + targetDevice.getAddress() + "\nName = " + targetDevice.getName();
    }

    public void initialize()
    {
        if(myAdapter == null) return;
        initialized = false;
        // enable bluetooth
        if(!myAdapter.isEnabled())
        {
            Intent enableBthIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            myActivity.startActivityForResult(enableBthIntent, REQUEST_ENABLE_BIT);
        }
        // refresh bonded PCs
        refreshTargetDevice();
        if(targetDevice == null)
        {
            error_msg = "Error: Cannot find/connect a PC\nPlease pair a PC and start Bluetooth service on PC first";
            return;
        }
        initialized = true;
    }

    public void destroy()
    {
        if(myAdapter == null) return;
        disconnect();
        try
        {
            myActivity.unregisterReceiver(myReceiver);
        }
        catch(Exception e)
        {
            Log.d(logTag, "destroy: failed to unregister receiver: " + e.getMessage());
        }
    }

    private void refreshTargetDevice()
    {
        if(!myAdapter.isEnabled()) return;
        targetDevice = null;
        Set<BluetoothDevice> pairedDevices = myAdapter.getBondedDevices();
        if(pairedDevices.size() > 0)
        {
            for(BluetoothDevice device : pairedDevices)
            {
                if (device.getBluetoothClass().getMajorDeviceClass() == BluetoothClass.Device.Major.COMPUTER)
                {
                    if (testDeviceConnection(device))
                    {
                        targetDevice = device;
                        break;
                    }
                }
            }
        }
    }

    private boolean testDeviceConnection(BluetoothDevice device)
    {
        BluetoothSocket tmp;
        try
        {
            tmp = device.createRfcommSocketToServiceRecord(CONNECTION_UUID);
        }
        catch (IOException e)
        {
            Log.d(logTag, "testDeviceConnection: paired PC MAC address = " + device.getAddress() + " Name = " + device.getName() + " cannot create socket with UUID");
            return false;
        }
        try
        {
            tmp.connect();
        }
        catch (IOException connectExp)
        {
            Log.d(logTag, "testDeviceConnection: paired PC MAC address = " + device.getAddress() + " Name = " + device.getName() + " socket failed to connect");
            try
            {
                tmp.close();
            }
            catch (IOException closeExp)
            {
                Log.d(logTag, "testDeviceConnection: paired PC MAC address = " + device.getAddress() + " Name = " + device.getName() + " socket failed to close");
            }
            return false;
        }
        try
        {
            tmp.close();
        }
        catch (IOException e)
        {
            Log.d(logTag, "testDeviceConnection: paired PC MAC address = " + device.getAddress() + " Name = " + device.getName() + " socket failed to close");
        }
        return true;
    }
}
