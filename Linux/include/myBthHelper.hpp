#pragma once

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <string>
#include <thread>

// reference1: https://people.csail.mit.edu/albert/bluez-intro/x502.html
// reference2: https://people.csail.mit.edu/albert/bluez-intro/x604.html

#define BLUETOOTH_BUFFER_SIZE 512

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

// This class manage bluetooth communication
class BthManager
{
public:
	BthManager();
	~BthManager();
	void start();
	void stop();
private:
	void initialize();
	void process();
public:
	bool initialized = false;
	bool connection_should_stop = false; // set to true if stop button clicked, in order to stop process thread
private:
	const unsigned char MY_UUID[16] = {
		0x97, 0xc4, 0xea, 0xb8, 0x23, 0x4b, 0x42, 0xd0,
		0x8c, 0x09, 0xe9, 0xa5, 0xb1, 0xf1, 0xba, 0x5b,
	}; // connection unique ID
	const uint8_t MY_CHANNEL = 11;
	const char* MY_SERVICE_NAME = "Android Trackpad (Linux Service)"; // set service name
	const char* MY_SERVICE_DESC = "Linux side bluetooth service for Android Trackpad project"; // set service description
	const char* MY_SERVICE_PROV = "Android Trackpad"; // set service provider
	int myLocalSocket = INVALID_SOCKET;
	int myClientSocket = INVALID_SOCKET;
	struct sockaddr_rc myLocalSocketAddr = {};
	struct sockaddr_rc myClientSocketAddr = {};
	sdp_session_t* myServiceSession = nullptr;
	timeval myLocalSocketTimeout = { 30, 0 }; // 30s timeout
	char buffer[BLUETOOTH_BUFFER_SIZE]; // buffer for receiving data
	// set a seperate thread to receive data from connected device
	std::thread processThread;
};