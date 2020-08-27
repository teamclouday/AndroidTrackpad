# Linux  

------

### Dependencies  
* SDL2 (Assume installed on system)  
* ImGui  
* OpenGL3 (Assume GLEW is installed on system)  
* bluez5  
* X11 (Make sure your linux backend is using X11 and something like libx11-dev is installed)  
* XTest (Make sure something like libxtst-dev is installed)  
* libnotify  
* PkgConfig (Tool for cmake use)  

### How to build  
First download ImGui package and prepare put it in ```external/imgui``` in this folder  
In the ```external/imgui``` folder, create ```src``` and ```include``` and put necessary files to them separately  
Next check that ```SDL2``` and ```GLEW``` are installed on this system  

Then type command (Release build):  
```bash
mkdir build && cd build  
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Or (Debug build):  
```bash
mkdir build && cd build  
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

The executables are stored in ```bin``` folder  

------

### Important Note  
The sdp registration method is using deprecated API, so make sure bluetoothd is running in compatible mode:  
```bash
sudo vim /etc/systemd/system/dbus-org.bluez.service
```
and change:  
```
ExecStart=/usr/lib/bluetooth/bluetoothd
```
to  
```
ExecStart=/usr/lib/bluetooth/bluetoothd --compat
```
Also, in order for bluetooth service to connect sdp, edit in same file:  
```
ExecStartPost=/bin/chmod 777 /var/run/sdp
```
to give permission to all users to use sdp (no idea if this is safe)  

Finally, run:  
```bash
sudo systemctl daemon-reload
sudo systemctl restart bluetooth
```

------

For more information, see README.md in Windows folder  