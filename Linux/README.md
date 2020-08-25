# Linux  

------

### Dependencies  
* SDL2 (Assume installed on system)  
* ImGui  
* OpenGL3 (Assume GLEW is installed on system)  
* Bluetooth dev lib (Assume something like libbluetooth-dev or bluez is installed)  
* X11 (Make sure your linux backend is using X11 and something like libx11-dev is installed)  

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

------

For more information, see README.md in Windows folder  