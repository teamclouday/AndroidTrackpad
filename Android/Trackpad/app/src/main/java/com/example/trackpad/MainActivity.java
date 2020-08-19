package com.example.trackpad;

import androidx.appcompat.app.AppCompatActivity;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends AppCompatActivity
{
    enum CONNECTION_TYPE
    {
        CONNECTION_TYPE_WIFI,
        CONNECTION_TYPE_BTH,
    };

    private CONNECTION_TYPE connection_type = CONNECTION_TYPE.CONNECTION_TYPE_WIFI;

    private final String logTag = "AndroidTrackpad";
    private final UUID CONNECTION_UUID = UUID.fromString("97c4eab8-234b-42d0-8c09-e9a5b1f1ba5b");

    // variables for Bluetooth
    public BluetoothAdapter myBthAdapter;
    public BluetoothDevice myBthDevice;
    public BluetoothSocket myBthSocket = null;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        setTitle(R.string.app_fullname);

        // set radio group listener
        RadioGroup radioGroup = findViewById(R.id.connection_options);
        radioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                switch(checkedId)
                {
                    case R.id.radioButtonBth:
                        connection_type = CONNECTION_TYPE.CONNECTION_TYPE_BTH;
                        break;
                    case R.id.radioButtonWifi:
                        connection_type = CONNECTION_TYPE.CONNECTION_TYPE_WIFI;
                        break;
                }
            }
        });
    }

    // onclick event for Connect button
    public void connectButtonClick(View view)
    {
        if (connection_type == CONNECTION_TYPE.CONNECTION_TYPE_WIFI)
        {
            // reference: https://developer.android.com/training/connect-devices-wirelessly/wifi-direct


        }
        else if(connection_type == CONNECTION_TYPE.CONNECTION_TYPE_BTH)
        {
            // reference: https://developer.android.com/guide/topics/connectivity/bluetooth#SettingUp

            // get default bluetooth adapter
            myBthAdapter = BluetoothAdapter.getDefaultAdapter();
            if (myBthAdapter == null)
            {
                refreshConnectInfo("Error: failed to get default Bluetooth adapter!");
                return;
            }
            // enable adapter first
            if (!myBthAdapter.isEnabled())
            {
                Intent enableBthIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                int REQUEST_ENABLE_BT = 1;
                startActivityForResult(enableBthIntent, REQUEST_ENABLE_BT);
            }
            // get paired devices
            Set<BluetoothDevice> pairedDevices = myBthAdapter.getBondedDevices();
            if (pairedDevices.size() > 0)
            {
                boolean found = false;
                for(BluetoothDevice device : pairedDevices)
                {
                    // check for paired PCs
                    if (device.getBluetoothClass().getMajorDeviceClass() == BluetoothClass.Device.Major.COMPUTER)
                    {
                        myBthDevice = device;
                        refreshConnectInfo("Found paired PC, MAC address = " + device.getAddress());
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    refreshConnectInfo("Error: cannot find a paired PC");
                    return;
                }
            }
            else
            {
                refreshConnectInfo("Error: cannot find paired device\nPlease pair your PC first");
                return;
            }
            // try to get connection socket
            BluetoothSocket tmp = null;
            try
            {
                tmp = myBthDevice.createRfcommSocketToServiceRecord(CONNECTION_UUID);
            }
            catch (IOException e)
            {
                refreshConnectInfo("Error: Bluetooth socket failed to create: " + e.getMessage());
            }
            myBthSocket = tmp;
            // connect as a client
            try
            {
                myBthSocket.connect();
            }
            catch (IOException connectExp)
            {
                refreshConnectInfo("Error: Bluetooth socket failed to connect: " + connectExp.getMessage());
                try
                {
                    myBthSocket.close();
                }
                catch (IOException closeExp)
                {
                    refreshConnectInfo("Error: Bluetooth socket failed to close: " + closeExp.getMessage());
                }
                myBthSocket = null;
                return;
            }
            refreshConnectInfo("Connection via Bluetooth socket succeed");
        }
        // disable click for radio buttons after connection
        findViewById(R.id.radioButtonBth).setClickable(false);
        findViewById(R.id.radioButtonWifi).setClickable(false);
        // send message
        Toast.makeText(getApplicationContext(), "You have been connected", 3).show();
    }

    // onclick event for Disconnect button
    public void disconnectButtonClick(View view)
    {
        if (connection_type == CONNECTION_TYPE.CONNECTION_TYPE_WIFI)
        {

        }
        else if (connection_type == CONNECTION_TYPE.CONNECTION_TYPE_BTH)
        {
            if (myBthSocket == null)
            {
                refreshConnectInfo("Warning: You have not connected yet");
            }
            else
            {
                try
                {
                    myBthSocket.close();
                }
                catch (IOException e)
                {
                    refreshConnectInfo("Error: Bluetooth socket failed to close: " + e.getMessage());
                }
                // send message
                Toast.makeText(getApplicationContext(), "You have been disconnected", 3).show();
            }
        }
        // enable click for radio buttons after disconnect
        findViewById(R.id.radioButtonBth).setClickable(true);
        findViewById(R.id.radioButtonWifi).setClickable(true);
    }

    // onclick event for Enter trackpad button
    public void trackpadButtonClick(View view)
    {
        Intent intent = new Intent(this, TrackpadActivity.class);
        startActivity(intent);
    }

    // onclick event for Help button
    public void helpButtonClick(View view)
    {
        Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://www.google.com"));
        startActivity(browserIntent);
    }

    // refresh connection information in text view
    private void refreshConnectInfo(String newMsg)
    {
        TextView infoText = findViewById(R.id.textViewConnectInfo);
        infoText.append(newMsg + "\n");
    }
}