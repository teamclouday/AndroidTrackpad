#pragma once

#include <WinSock2.h>
#include <ws2bth.h>

#include <string>
#include <thread>

// reference1: https://docs.microsoft.com/en-us/windows/win32/bluetooth/bluetooth-programming-with-windows-sockets
// reference2: https://www.winsocketdotnetworkprogramming.com/winsock2programming/winsock2advancedotherprotocol4p.html

#define BLUETOOTH_BUFFER_SIZE 512

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
	// set server ID
	LPCWSTR MY_UUID = TEXT("{97c4eab8-234b-42d0-8c09-e9a5b1f1ba5b}"); // connection unique ID
	LPCWSTR MY_SERVICE_NAME = TEXT("Android Trackpad (Windows Service)"); // set service name
	LPCWSTR MY_SERVICE_COMMENT = TEXT("Windows side bluetooth service for Android Trackpad project"); // set service comment
	SOCKET myLocalSocket = INVALID_SOCKET;
	SOCKET myClientSocket = INVALID_SOCKET;
	SOCKADDR_BTH myLocalSocketAddr = {0};
	SOCKADDR_BTH myClientSocketAddr = {0};
	const timeval myLocalSocketTimeout = { 30, 0 }; // 30s timeout
	char buffer[BLUETOOTH_BUFFER_SIZE]; // buffer for receiving data
	// set a seperate thread to receive data from connected device
	std::thread processThread;
};