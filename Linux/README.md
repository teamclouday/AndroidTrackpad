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
This program requires root privilege (for Bluetooth service registration in sdp)  

```bash
sudo ./TrackpadService
```

------

For more information, see README.md in Windows folder  