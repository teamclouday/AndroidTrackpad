#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <string>
#include <thread>

// reference: https://docs.microsoft.com/en-us/windows/win32/winsock/getting-started-with-winsock

#define WIFI_BUFFER_SIZE 512

// This class manage wifi p2p communication
class WifiManager
{
public:
	WifiManager();
	~WifiManager();
	void start();
	void stop();
private:
	void initialize();
	void process();
	bool validate();
public:
	bool initialized = false;
	bool connection_should_stop = false;
private:
	const int myLocalPort = 10086;
	std::string localIP = "";
	SOCKET myLocalSocket = INVALID_SOCKET;
	SOCKET myClientSocket = INVALID_SOCKET;
	SOCKADDR_IN myClientSocketAddr = { 0 };
	const timeval myLocalSocketTimeout = { 60, 0 }; // 60s timeout
	char buffer[WIFI_BUFFER_SIZE]; // buffer for receiving data
	const char* test_server = "8.8.8.8"; // test server ip for network connection
	const int validate_id = 10086; // an id for validating connecting device
	// set a seperate thread to receive data from connected device
	std::thread processThread;
};