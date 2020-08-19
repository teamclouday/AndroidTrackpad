# Android Trackpad  

Connect your Android phone to Windows or Linux system as a trackpad  

------

### Features  
1. Connect via Wifi, or Bluetooth (Configuration done on both sides)  
2. Three basic actions:  
   a. click (left & right)  
   b. scroll (up-down & left-right)  
   c. move  
3. Data pack design:  
   ```cpp
    enum DATA_TYPE
    {
        DATA_TYPE_CLICK_LEFT  = 0;
        DATA_TYPE_CLICK_RIGHT = 1;
        DATA_TYPE_SCROLL_HORI = 2;
        DATA_TYPE_SCROLL_VERT = 3;
        DATA_TYPE_MOVE        = 4;
    };

    struct DATA
    {
        enum DATA_TYPE type;
        uint32_t posX;
        uint32_t posY;
        uint32_t time;
    };
   ```
   ```posX``` and ```posY``` are used as positions in CLICK. They are used as directions in SCROLL and MOVE.  
   ```time``` is used as speed in MOVE, and period in SCROLL. It is ignored in CLICK.  
4. Trackpad sensitivity control  

------

