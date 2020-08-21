package com.example.trackpad;

import androidx.appcompat.app.AppCompatActivity;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;


public class MainActivity extends AppCompatActivity
{
    enum CONNECTION_TYPE
    {
        CONNECTION_TYPE_WIFI,
        CONNECTION_TYPE_BTH,
    };

    protected CONNECTION_TYPE connection_type = CONNECTION_TYPE.CONNECTION_TYPE_WIFI;

    protected final String logTag = "AndroidTrackpad";

    protected myBthManager managerBth = null;
    protected myWifiManager managerWifi = null;

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
    // run in background to avoid UI stuck

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
                    currentActivity.managerWifi.initialize();
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
        new connectButtonClickTask().execute(this);
    }

    // onclick event for Disconnect button
    // run in background to avoid UI stuck

    private final class disconnectButtonClickTask extends AsyncTask<MainActivity, Void, Void>
    {
        @Override
        protected Void doInBackground(MainActivity... mainActivities) {
            MainActivity currentActivity = mainActivities[0];
            if (currentActivity.connection_type == CONNECTION_TYPE.CONNECTION_TYPE_WIFI)
            {
                currentActivity.managerWifi.disconnect();
            }
            else if (currentActivity.connection_type == CONNECTION_TYPE.CONNECTION_TYPE_BTH)
            {
                currentActivity.managerBth.disconnect();
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            super.onPostExecute(aVoid);
            toggleConnectionState(false);
        }
    }

    public void disconnectButtonClick(View view)
    {
        new disconnectButtonClickTask().execute(this);
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
    protected void refreshConnectInfo(String newMsg)
    {
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
        }
        else
        {
            // enable click for radio buttons after disconnect
            findViewById(R.id.radioButtonBth).setEnabled(true);
            findViewById(R.id.radioButtonWifi).setEnabled(true);
        }
    }
}