package com.example.trackpad;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.WifiManager;
import android.net.wifi.WpsInfo;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pDeviceList;
import android.net.wifi.p2p.WifiP2pManager;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.nio.ByteBuffer;
import java.util.Collection;

// reference: https://developer.android.com/guide/topics/connectivity/wifip2p

public class myWifiManager
{
    private Activity myActivity;
    private WifiP2pManager myManager;
    private WifiP2pManager.Channel myChannel;
    private WifiP2pDevice targetDevice = null;
    private Socket mySocket = null;
    private final int myServerPort = 10086;

    private final String logTag = "TrackpadWifi";

    public static MainActivity.BufferData buffer = new MainActivity.BufferData();

    public boolean initialized = false;
    public boolean connected = false;
    public String error_msg = "";

    protected boolean discoveryDone = false;
    protected boolean connected_test = false;
    protected final Object lock = new Object();

    private WifiP2pManager.PeerListListener myPeerListener = new WifiP2pManager.PeerListListener()
    {
        @Override
        public void onPeersAvailable(WifiP2pDeviceList peers) {
            Collection<WifiP2pDevice> devices = peers.getDeviceList();
            if(devices.size() == 0)
            {
                error_msg = "Error: No peer device can be found";
            }
            else
            {
                for(WifiP2pDevice device : devices)
                {
                    // https://ndeflib.readthedocs.io/en/stable/records/wifi.html#primary-device-type
                    // for all primary device types
                    Log.d(logTag, "onPeersAvailable: Name = " + device.deviceName + " Type = " + device.primaryDeviceType);
                    if(device.primaryDeviceType.contains("Computer"))
                    {
                        // test connection for each peer computer
                        if(testDeviceConnection(device))
                        {
                            targetDevice = device;
                            break;
                        }
                    }
                }
            }
            synchronized (lock)
            {
                discoveryDone = true;
                lock.notify();
            }
        }
    };

    public class WifiDirectBroadcastReceiver extends BroadcastReceiver
    {
        private WifiP2pManager manager;
        private WifiP2pManager.Channel channel;
        private Activity activity;

        public WifiDirectBroadcastReceiver(WifiP2pManager manager, WifiP2pManager.Channel channel, Activity activity)
        {
            super();
            this.manager = manager;
            this.channel = channel;
            this.activity = activity;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if(WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION.equals(action))
            {
                // get peer device list
                if(manager != null)
                {
                    manager.requestPeers(channel, myPeerListener);
                }
            }
            else if(WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION.equals(action))
            {
                int state = intent.getIntExtra(WifiP2pManager.EXTRA_WIFI_STATE, -1);
                if(state == WifiP2pManager.WIFI_P2P_STATE_ENABLED)
                {
                    Log.d(logTag, "onReceive: enabled");
                }
                else
                {
                    Log.d(logTag, "onReceive: disabled");
                }
            }
            else if(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION.equals(action))
            {

            }
            else if(WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION.equals(action))
            {

            }
        }
    }

    private WifiDirectBroadcastReceiver myReceiver;

    public myWifiManager(Activity current_activity)
    {
        myActivity = current_activity;
        initialize();
        // set intent filter for state changes
        IntentFilter filter = new IntentFilter(WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION);
        filter.addAction(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);
        filter.addAction(WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION);
        filter.addAction(WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION);
        myReceiver = new WifiDirectBroadcastReceiver(myManager, myChannel, myActivity);
        myActivity.registerReceiver(myReceiver, filter);
    }

    public void connect()
    {
        if(targetDevice == null) return;
        if(mySocket == null)
            mySocket = new Socket();
        WifiP2pConfig config = new WifiP2pConfig();
        config.deviceAddress = targetDevice.deviceAddress;
        myManager.connect(myChannel, config, new WifiP2pManager.ActionListener()
        {
            @Override
            public void onSuccess()
            {
                try
                {
                    mySocket.bind(null);
                }
                catch(IOException e)
                {
                    return;
                }
                try
                {
                    mySocket.connect((new InetSocketAddress(targetDevice.deviceAddress, myServerPort)), 10000); // timeout for 10 seconds
                }
                catch(IOException e)
                {
                    return;
                }
                connected = true;
            }

            @Override
            public void onFailure(int reason)
            {
                connected = false;
            }
        });
    }

    public synchronized void addData(MainActivity.BufferSingle data)
    {
        buffer.addData(data);
    }

    public boolean sendMessage()
    {
        if(!connected) return false;
        if(mySocket == null) return false;
        // try to get output stream
        OutputStream stream;
        try
        {
            stream = mySocket.getOutputStream();
        }
        catch(IOException e)
        {
            error_msg = "Error: failed to get output stream from wifi socket";
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
            error_msg = "Error: failed to write to output stream in wifi socket";
            return false;
        }
        return true;
    }

    public void disconnect()
    {
        if(mySocket == null)
        {
            connected = false;
            return;
        }
        try
        {
            mySocket.close();
        }
        catch (IOException e){}
        mySocket = null;
        connected = false;
    }

    public void destroy()
    {
        try
        {
            myActivity.unregisterReceiver(myReceiver);
        }
        catch (Exception e)
        {
            Log.d(logTag, "destroy: failed to unregister receiver: " + e.getMessage());
        }
    }

    public void initialize()
    {
        initialized = false;
        discoveryDone = false;
        // check for Wifi status
        WifiManager tmpManager = (WifiManager) myActivity.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if(tmpManager == null)
        {
            error_msg = "Error: Cannot get Wifi service";
            return;
        }
        if(!tmpManager.isWifiEnabled())
        {
            error_msg = "Error: Wifi is not turned on";
            return;
        }
        // try to get P2P manager and initialize
        myManager = (WifiP2pManager) myActivity.getSystemService(Context.WIFI_P2P_SERVICE);
        if(myManager == null)
        {
            error_msg = "Error: Cannot get Wifi P2P service";
            return;
        }
        myChannel = myManager.initialize(myActivity.getApplicationContext(), myActivity.getMainLooper(), null);
        if(myChannel == null)
        {
            error_msg = "Error: Cannot initialize Wifi P2P";
            return;
        }
        // discover peer devices
        myManager.discoverPeers(myChannel, new WifiP2pManager.ActionListener()
        {
            @Override
            public void onSuccess(){}

            @Override
            public void onFailure(int reason)
            {
                if(reason == WifiP2pManager.P2P_UNSUPPORTED)
                {
                    error_msg = "Error: Wifi P2P is not supported";
                }
                else
                {
                    error_msg = "Error: Failed to discover peer devices";
                }
            }
        });
        // initialize fail if error_msg is not empty
        if(!error_msg.isEmpty()) return;
        // try to wait for discovery done signal
        synchronized (lock)
        {
            long timeout = System.currentTimeMillis() + 30000; // set timeout of 30s
            while(!discoveryDone)
            {
                long lockWait = timeout - System.currentTimeMillis();
                if(lockWait <= 0) break;
                try
                {
                    lock.wait(lockWait);
                }
                catch (InterruptedException e)
                {
                    break;
                }
            }
        }
        if(!discoveryDone)
        {
            error_msg = "Error: 30s expired and no peer device is found\nPlease launch P2P service on target PC in same network and try again";
        }
        // stop discovery after discoveryDone
        myManager.stopPeerDiscovery(myChannel, new WifiP2pManager.ActionListener()
        {
            @Override
            public void onSuccess(){}

            @Override
            public void onFailure(int reason){}
        });
        // initialize fail if error_msg is not empty
        if(!error_msg.isEmpty()) return;
        initialized = true;
    }

    public String getDeviceInfo()
    {
        if(targetDevice == null) return "";
        return "IP = " + targetDevice.deviceAddress + "\nName = " + targetDevice.deviceName;
    }

    private boolean testDeviceConnection(WifiP2pDevice device)
    {
        connected_test = false;
        WifiP2pConfig config = new WifiP2pConfig();
        config.deviceAddress = device.deviceAddress;
        myManager.connect(myChannel, config, new WifiP2pManager.ActionListener()
        {
            @Override
            public void onSuccess()
            {
                connected_test = true;
                myManager.removeGroup(myChannel, new WifiP2pManager.ActionListener() {
                    @Override
                    public void onSuccess(){}
                    @Override
                    public void onFailure(int reason){}
                });
            }

            @Override
            public void onFailure(int reason)
            {
                connected_test = false;
            }
        });
        Log.d(logTag, "testDeviceConnection: Name = " + device.deviceName + " IP = " +
                device.deviceAddress + " Connection " + (connected_test ? "succeeded" : "failed"));
        return connected_test;
    }
}
