# Windows  

------

### Dependencies  
* SDL2  
* ImGui  
* OpenGL3 (GLEW)  
* WinSock2 (ws2_32.lib)  

### How to build  
Prepare ```external/glew```, ```external/ImGui```, and ```external/SDL2``` under this folder (download from website)  
with ```include``` and ```lib``` (or ```src``` in case of ImGui) folder in each  
libs for glew should be compiled static (both in x86 and x64)  
libs for SDL2 should have both x86 and x64, and put with dll into ```lib```  
Open solution file ```*.sln``` in visual studio and build  

------

### Features  

* 3 working threads  
  1. UI manager thread  
  2. Communication manager thread  
  3. Trackpad manager thread  
* Keyboard control on UI  
  ESC to exit the program  
* Wifi mode notice  
  In this mode, windows should launch Mobile hotspot for android device to connect  

------

### Known Issues  
* Program will not exit immediately if service is starting and UI exits  
  This is because the ```select``` function with a defined timeout is blocking the communication manager thread  
  Possible solutions can be found [here](https://stackoverflow.com/questions/3333361/how-to-cancel-waiting-in-select-on-windows)  
  I just don't bother to wait for 60s  
* If service is connected, and UI exits. The program will continue running until something is done on android side (either new movement or connection close)  
  Because ```recv``` function will pause the thread when no input is from the socket  
* The "Stop Service" button only works after something is done on android side (either new movement or connection close)  
  This is because the ```recv``` function will pause the thread when no input is from the socket  
  If your program does not stop, try to disconnect on android side  
* For special programs (such as VMWare Workstation), the trackpad won't work, because the workstation won't capture input sent from ```SendInput```  