package com.example.trackpad;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.InputType;
import android.util.Log;
import android.util.Patterns;
import android.widget.EditText;
import android.widget.Toast;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;

// reference: https://developer.android.com/guide/topics/connectivity/wifip2p

public class myWifiManager
{
    private Activity myActivity;
    private Socket mySocket = null;
    private final int myServerPort = 10086;
    private final int validationID = 10086;
    public String myServerIPAddr = "";

    private final String logTag = "TrackpadWifi";

    public static MainActivity.BufferData buffer = new MainActivity.BufferData();

    public boolean initialized = false;
    public boolean decided = false;
    public boolean connected = false;
    public String error_msg = "";

    protected final Object lock = new Object();

    public myWifiManager(Activity current_activity)
    {
        myActivity = current_activity;
        initialize();
    }

    public void connect()
    {
        if(!sendValidateMessage()) return;
        if(mySocket == null)
            mySocket = new Socket();
        try
        {
            mySocket.bind(null);
        }
        catch(IOException e)
        {
            Log.d(logTag, "connect: socket bind failed " + e.getMessage());
            return;
        }
        try
        {
            mySocket.connect((new InetSocketAddress(myServerIPAddr, myServerPort)), 10000); // timeout for 10 seconds
        }
        catch(IOException e)
        {
            Log.d(logTag, "connect: socket connect failed " + e.getMessage());
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
        catch (IOException e)
        {
            Log.d(logTag, "disconnect: socket close failed " + e.getMessage());
        }
        mySocket = null;
        connected = false;
    }

    public void destroy()
    {
        disconnect();
    }

    public void initialize()
    {
        initialized = false;
        // create alert dialog to ask user input target IP address
        myActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(myActivity);
                builder.setTitle("Enter IP address of target PC\n(Leave empty to skip)");
                final EditText ipinput = new EditText(myActivity);
                ipinput.setInputType(InputType.TYPE_CLASS_TEXT);
                builder.setView(ipinput);
                // Set up the buttons
                builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        if(checkIpAddressString(ipinput.getText().toString()))
                        {
                            if(!ipinput.getText().toString().isEmpty())
                                myServerIPAddr = ipinput.getText().toString();
                            initialized = true;
                        }
                        else
                        {
                            Toast.makeText(myActivity, "Invalid IP Address Please try again", Toast.LENGTH_SHORT).show();
                            error_msg = "Invalid IP address entered";
                        }
                        decided = true;
                    }
                });
                builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        dialog.cancel();
                        error_msg = "Target IP address not entered";
                        decided = true;
                    }
                });
                builder.show();
            }
        });

    }

    private boolean sendValidateMessage()
    {
        // open tmp socket
        Socket tmp = new Socket();
        try
        {
            tmp.bind(null);
        }
        catch(IOException e)
        {
            Log.d(logTag, "sendValidateMessage: socket bind failed " + e.getMessage());
            return false;
        }
        try
        {
            tmp.connect((new InetSocketAddress(myServerIPAddr, myServerPort)), 10000); // timeout for 10 seconds
        }
        catch(IOException e)
        {
            Log.d(logTag, "sendValidateMessage: socket connect failed " + e.getMessage());
            return false;
        }
        OutputStream stream;
        try
        {
            stream = tmp.getOutputStream();
        }
        catch(IOException e)
        {
            Log.d(logTag, "sendValidateMessage: socket getOutputStream failed " + e.getMessage());
            return false;
        }
        // prepare data to send
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        ByteBuffer bb = ByteBuffer.allocate(4);
        bb.putInt(validationID);
        outputStream.write(bb.array(), 0, 4);
        // write through socket
        try
        {
            stream.write(outputStream.toByteArray());
        }
        catch(IOException e)
        {
            Log.d(logTag, "sendValidateMessage: socket output stream write failed " + e.getMessage());
            return false;
        }
        // close socket
        try
        {
            tmp.close();
        }
        catch(IOException e)
        {
            return false;
        }
        return true;
    }

    public String getDeviceInfo()
    {
        return "IP = " + myServerIPAddr;
    }

    public boolean checkIpAddressString(String input)
    {
        // either empty or normal IP is valid
        return input.isEmpty() || Patterns.IP_ADDRESS.matcher(input).matches();
    }
}
