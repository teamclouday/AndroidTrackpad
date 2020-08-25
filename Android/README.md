# Android  

------

### Communications  

* Bluetooth  
  In bluetooth mode, the application will search in paired devices for target PC  
  Assumed that a bluetooth server service has been running on target PC  
  It will try to connect to the server, and prepare for communication  
* Wifi (Direct)  
  In Wifi mode, the application will ask for input of PC's IP address (which is shown on PC side)  
  Assumed that a TCP server is running on PC. It will try to connect the target PC directly  
  Before real connection, android side will send a pre-coded validation code to PC, so that PC knows it is the right device  

------

### Gestures  

* One finger tap = Left click  
* Two finger tap = Right click  
* One finger move = Move cursor  
* Two finger move = Scroll (Up, Down, Left, Right)  
* Multiple finger move = Not supported (will use first identified finger as one finger move)  
* Double tap then move = Drag  