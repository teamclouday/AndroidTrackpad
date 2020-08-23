# Android  

------

### Communications  

* Bluetooth  
  In bluetooth mode, the application will search in paired devices for target PC  
  Assumed that a bluetooth server service has been running on target PC  
  It will try to connect to the server, and prepare for communication  
* Wifi (P2P)  
  In P2P mode, the application will start a discovery of peer devices  
  It will filter only PC devices, and tries to connect  
  Assumed that a P2P service has been running on target PC  
  Will try to exchange a unique ID before actual communication  
  In this mode, android device is assumed to have connected to target deivce's Mobile hotspot

------

### Gestures  

* One finger tap = Left click  
* Two finger tap = Right click  
* One finger move = Move cursor  
* Two finger move = Scroll (Up, Down, Left, Right)  
* Multiple finger move = Not supported (will use first identified finger as one finger move)  