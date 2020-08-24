# Android Trackpad  

Connect your Android phone to Windows or Linux system as a trackpad  

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
   ```velX``` and ```velY``` are used as velocity when type is a movement  
4. Trackpad sensitivity control  

------

