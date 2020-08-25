package com.example.trackpad;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;


public class MainActivity extends AppCompatActivity
{
    // define connection type
    enum CONNECTION_TYPE
    {
        CONNECTION_TYPE_WIFI,
        CONNECTION_TYPE_BTH,
    }
    // define data type for communication
    enum DATA_TYPE
    {
        DATA_TYPE_CLICK_LEFT    (0),
        DATA_TYPE_CLICK_RIGHT   (1),
        DATA_TYPE_SCROLL_HORI   (2),
        DATA_TYPE_SCROLL_VERT   (3),
        DATA_TYPE_DRAG          (4),
        DATA_TYPE_MOVE          (5);

        private final int value;
        DATA_TYPE(int value)
        {
            this.value = value;
        }
        public int getValue()
        {
            return value;
        }
    }
    // define a single package structure
    public static class BufferSingle
    {
        public DATA_TYPE type;
        public float velX;
        public float velY;
        public BufferSingle(DATA_TYPE type, float velX, float velY)
        {
            this.type = type;
            this.velX = velX;
            this.velY = velY;
        }
    }
    // define a helper class for managing data packages
    public static class BufferData
    {
        public final int MAX_SIZE = 50;
        private ArrayList<BufferSingle> buff = new ArrayList<>();

        public synchronized void addData(BufferSingle data)
        {
            if(buff.size() >= MAX_SIZE) return;
            buff.add(data);
        }

        public synchronized BufferSingle getData()
        {
            if(buff.size() <= 0) return null;
            return buff.remove(0);
        }

        public synchronized int size()
        {
            return buff.size();
        }
    }
    // set default connection type to Wifi
    protected CONNECTION_TYPE connection_type = CONNECTION_TYPE.CONNECTION_TYPE_WIFI;
    // set log tag for logging
    protected final String logTag = "AndroidTrackpad";

    protected myBthManager managerBth = null;
    protected myWifiManager managerWifi = null;
    private Thread managerThread;
    private Thread trackpadThread;
    public static BufferData data_buffer = new BufferData();

    // a runnable that tries to transfer data if certain managers are alive (connected)
    class myManagerRunnable implements Runnable
    {
        private MainActivity activity;
        public myManagerRunnable(MainActivity activity)
        {
            super();
            this.activity = activity;
        }

        @Override
        public void run()
        {
            // while loop until this thread is interrupted (app exit)
            while(!Thread.interrupted())
            {
                if(activity.connection_type == CONNECTION_TYPE.CONNECTION_TYPE_BTH &&
                activity.managerBth != null)
                {
                    if(activity.managerBth.initialized && activity.managerBth.connected)
                    {
                        if(!activity.managerBth.sendMessage())
                        {
                            final String error_msg = activity.managerBth.error_msg;
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    refreshConnectInfo(error_msg);
                                    refreshConnectInfo("You will be disconnected");
                                }
                            });
                            new disconnectButtonClickTask().execute(activity);
                            sleep();
                        }
                    }
                    else
                    {
                        sleep();
                    }
                }
                else if(activity.connection_type == CONNECTION_TYPE.CONNECTION_TYPE_WIFI &&
                activity.managerWifi != null)
                {
                    if(activity.managerWifi.initialized && activity.managerWifi.connected)
                    {
                        if(!activity.managerWifi.sendMessage())
                        {
                            final String error_msg = activity.managerWifi.error_msg;
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    refreshConnectInfo(error_msg);
                                    refreshConnectInfo("You will be disconnected");
                                }
                            });
                            new disconnectButtonClickTask().execute(activity);
                            sleep();
                        }
                    }
                    else
                    {
                        sleep();
                    }
                }
                else
                {
                    sleep();
                }
            }
        }
        // sleep for 50 milliseconds to avoid heavy CPU usage
        private void sleep()
        {
            try
            {
                Thread.sleep(50);
            }
            catch(InterruptedException e){}
        }
    }
    // a runnable that manage the trackpad service
    class myTrackpadRunnable implements Runnable
    {
        private MainActivity activity;

        public myTrackpadRunnable(MainActivity activity)
        {
            this.activity = activity;
        }

        @Override
        public void run()
        {
            while(!Thread.interrupted())
            {
                if(activity.connection_type == CONNECTION_TYPE.CONNECTION_TYPE_BTH)
                {
                    if(activity.managerBth != null && activity.managerBth.connected)
                    {
                        while(data_buffer.size() > 0)
                        {
                            activity.managerBth.addData(data_buffer.getData());
                        }
                    }
                    else
                    {
                        sleep();
                    }
                }
                else
                {
                    if(activity.managerWifi != null && activity.managerWifi.connected)
                    {
                        while(data_buffer.size() > 0)
                        {
                            activity.managerWifi.addData(data_buffer.getData());
                        }
                    }
                    else
                    {
                        sleep();
                    }
                }
            }
        }

        private void sleep()
        {
            try
            {
                Thread.sleep(50);
            }
            catch(InterruptedException e){}
        }
    }

    public static synchronized void addData(BufferSingle data)
    {
        data_buffer.addData(data);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // set title of the activity
        setTitle(R.string.app_fullname);
        // set radio group listener for connection type change
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
        // start the manager thread
        managerThread = new Thread(new myManagerRunnable(this));
        managerThread.start();
        // start the trackpad thread
        trackpadThread = new Thread(new myTrackpadRunnable(this));
        trackpadThread.start();
    }

    // onclick event for Connect button
    // run in background to avoid UI stuck
    // this async task will create a certain manager class object if required
    // and trigger connect method
    private final class connectButtonClickTask extends AsyncTask<MainActivity, Void, Void>
    {
        private boolean taskfail = false;
        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            findViewById(R.id.btn_connect).setClickable(false);
        }

        @Override
        protected Void doInBackground(MainActivity... mainActivities) {
            MainActivity currentActivity = mainActivities[0];
            if (currentActivity.connection_type == CONNECTION_TYPE.CONNECTION_TYPE_WIFI)
            {
                if(currentActivity.managerBth != null)
                {
                    currentActivity.managerBth.destroy();
                    currentActivity.managerBth = null;
                }
                if(currentActivity.managerWifi == null)
                {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(getApplicationContext(), "initializing wifi, please wait", Toast.LENGTH_SHORT).show();
                        }
                    });
                    currentActivity.managerWifi = new myWifiManager(currentActivity);
                }
                else
                {
                    currentActivity.managerWifi.initialize();
                }
                while(!currentActivity.managerWifi.decided)
                {
                    try
                    {
                        Thread.sleep(10);
                    }
                    catch(InterruptedException e)
                    {
                        break;
                    }
                }
                currentActivity.managerWifi.decided = false;

                if(!currentActivity.managerWifi.initialized)
                {
                    final String error_msg = currentActivity.managerWifi.error_msg;
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            refreshConnectInfo("Error: failed to initialize wifi helper\n" + error_msg);
                        }
                    });
                    taskfail = true;
                    return null;
                }
                else
                {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            refreshConnectInfo("Trying to connect to " + managerWifi.myServerIPAddr);
                        }
                    });
                    currentActivity.managerWifi.connect();
                    if(!currentActivity.managerWifi.connected)
                    {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                refreshConnectInfo("Error: failed to connect a PC");
                            }
                        });
                        taskfail = true;
                        return null;
                    }
                    else {
                        final String deviceInfo = managerWifi.getDeviceInfo();
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                refreshConnectInfo("Wifi connected\nDevice Info:\n" + deviceInfo);
                            }
                        });
                    }
                }

            }
            else if(currentActivity.connection_type == CONNECTION_TYPE.CONNECTION_TYPE_BTH)
            {
                if(currentActivity.managerWifi != null)
                {
                    currentActivity.managerWifi.destroy();
                    currentActivity.managerWifi = null;
                }
                if(currentActivity.managerBth == null)
                {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(getApplicationContext(), "initializing bluetooth, please wait", Toast.LENGTH_SHORT).show();
                        }
                    });
                    currentActivity.managerBth = new myBthManager(currentActivity);
                }
                else
                    currentActivity.managerBth.initialize();
                if(!currentActivity.managerBth.initialized)
                {
                    final String error_msg = currentActivity.managerBth.error_msg;
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            refreshConnectInfo("Error: failed to initialize bluetooth helper\n" + error_msg);
                        }
                    });
                    taskfail = true;
                    return null;
                }
                else {
                    currentActivity.managerBth.connect();
                    if(!currentActivity.managerBth.connected)
                    {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                refreshConnectInfo("Error: failed to connect a PC");
                            }
                        });
                        taskfail = true;
                        return null;
                    }
                    else {
                        final String deviceInfo = managerBth.getDeviceInfo();
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                refreshConnectInfo("Bluetooth connected\nDevice Info:\n" + deviceInfo);
                            }
                        });
                    }
                }
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            super.onPostExecute(aVoid);
            if(!taskfail)
            {
                toggleConnectionState(true);
                // send message
                Toast.makeText(getApplicationContext(), "You have been connected", Toast.LENGTH_SHORT).show();
            }
            findViewById(R.id.btn_connect).setClickable(true);
        }
    }
    public void connectButtonClick(View view)
    {
        // run the async task
        new connectButtonClickTask().execute(this);
    }

    // onclick event for Disconnect button
    // run in background to avoid UI stuck
    // this async task will trigger disconnect method
    private final class disconnectButtonClickTask extends AsyncTask<MainActivity, Void, Void>
    {
        @Override
        protected Void doInBackground(MainActivity... mainActivities) {
            MainActivity currentActivity = mainActivities[0];
            if (currentActivity.connection_type == CONNECTION_TYPE.CONNECTION_TYPE_WIFI)
            {
                if(currentActivity.managerWifi != null && currentActivity.managerWifi.initialized)
                    currentActivity.managerWifi.disconnect();
            }
            else if (currentActivity.connection_type == CONNECTION_TYPE.CONNECTION_TYPE_BTH)
            {
                if(currentActivity.managerBth != null && currentActivity.managerBth.initialized)
                    currentActivity.managerBth.disconnect();
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            super.onPostExecute(aVoid);
            toggleConnectionState(false);
            // send message
            Toast.makeText(getApplicationContext(), "You have been disconnected", Toast.LENGTH_SHORT).show();
        }
    }
    public void disconnectButtonClick(View view)
    {
        // run the async task
        new disconnectButtonClickTask().execute(this);
    }

    // onclick event for Enter trackpad button
    public void trackpadButtonClick(View view)
    {
        // launch the trackpad activity
        Intent intent = new Intent(this, TrackpadActivity.class);
        startActivity(intent);
    }

    // onclick event for Help button
    public void helpButtonClick(View view)
    {
        // help button will open the github page of this project
        Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/teamclouday/AndroidTrackpad/tree/master/Android"));
        startActivity(browserIntent);
    }

    // refresh connection information in text view
    protected void refreshConnectInfo(String newMsg)
    {
        // add new message to connection information text view
        TextView infoText = findViewById(R.id.textViewConnectInfo);
        infoText.append(newMsg + "\n");
    }

    // toggle connection state
    protected void toggleConnectionState(boolean connected)
    {
        if(connected)
        {
            // disable click for radio buttons after connection
            findViewById(R.id.radioButtonBth).setEnabled(false);
            findViewById(R.id.radioButtonWifi).setEnabled(false);
            findViewById(R.id.btn_connect).setEnabled(false);
        }
        else
        {
            // enable click for radio buttons after disconnect
            findViewById(R.id.radioButtonBth).setEnabled(true);
            findViewById(R.id.radioButtonWifi).setEnabled(true);
            findViewById(R.id.btn_connect).setEnabled(true);
        }
    }
}