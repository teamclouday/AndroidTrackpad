# Linux  

------

### Dependencies  
* SDL2 (Assume installed on system)  
* ImGui  
* OpenGL3 (Assume GLEW is installed on system)  
* Bluez5 (Or Bluez4)  
* X11 (Make sure your linux backend is using X11 and something like libx11-dev is installed)  
* XTest (Make sure something like libxtst-dev is installed)  
* libnotify  
* PkgConfig (Tool for cmake use)  

### How to build  
At the very beginning, make sure that your system:  
* Has installed ```SDL2```, and ```GLEW```  
* Has installed ```Bluez``` (or ```libbluetooth-dev```)  
* Has installed ```libxtst-dev```  
* Has installed ```libnotify```  
* Has installed ```pkg-config```
* Is currently using X11 as backend (Wayland not supported)  
* Support OpenGL 3.0 at minimum  

First download ImGui package and prepare put it in ```external/imgui``` in this folder  
In the ```external/imgui``` folder, create ```src``` and ```include``` and put necessary files to them separately  

The final tree of ```external/imgui``` should be:  
```

```

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
The sdp registration method is using deprecated API, so make sure bluetoothd is running in compatible mode (**Only do this if your Bluez is version 5**):  
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
Then for any version of Bluez, edit in same file:  
```
ExecStartPost=/bin/chmod 777 /var/run/sdp
```
to give permission to all users to use sdp (no idea if this is safe yet)  

Finally, run:  
```bash
sudo systemctl daemon-reload
sudo systemctl restart bluetooth
```

------

For more information, see README.md in Windows folder  