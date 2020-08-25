# Android Trackpad  

Connect your Android phone to Windows or Linux system as a trackpad  

------

### How to use  
Install PC side based on your system, install apk on your Android phone  
For bluetooth mode, please pair phone with your PC in advance  
For Wifi mode, please start a hotspot on PC and connect your phone with it (Or if your router support port forwarding, this program may also work for PC and Android in same network. The port is ```10086```)  

#### Note
There are some issues with exiting the program (See Windows README.md for detail)  
All are solvable by disconnecting from Android side first  

------

### Features  
1. Connect via Wifi, or Bluetooth (Configuration done on both sides)  
2. Four basic actions:  
   a. click (left & right)  
   b. scroll (up-down & left-right)  
   c. drag  
   d. move  
3. Data pack design:  
   ```cpp
    enum DATA_TYPE
    {
        DATA_TYPE_CLICK_LEFT  = 0,
        DATA_TYPE_CLICK_RIGHT = 1,
        DATA_TYPE_SCROLL_HORI = 2,
        DATA_TYPE_SCROLL_VERT = 3,
        DATA_TYPE_DRAG        = 4,
        DATA_TYPE_MOVE        = 5,
    };

    struct DATA
    {
        enum DATA_TYPE type;
        float velX;
        float velY;
    };
   ```
   ```velX``` and ```velY``` are used as distance when type is a movement  
4. Trackpad sensitivity control  

------

