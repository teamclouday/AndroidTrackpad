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