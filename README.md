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
        uint32_t x_0;
        uint32_t y_0;
        uint32_t x_1;
        uint32_t y_1;
    };
   ```
   ```x_0``` and ```y_0``` are used as position when data type is click. They are used as first position when data type is a movement. ```x_1``` and ```y_1``` are used as second postion when data type is move. They are used as scrolling period when data type is scroll.  
4. Trackpad sensitivity control  

------

