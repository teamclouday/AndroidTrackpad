#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

#include <string>
#include <thread>

// reference: http://www.linuxhowtos.org/C_C++/socket.htm

#define WIFI_BUFFER_SIZE 512

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

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
	int myLocalSocket = INVALID_SOCKET;
	int myClientSocket = INVALID_SOCKET;
	struct sockaddr_in myClientSocketAddr = {};
	timeval myLocalSocketTimeout = { 60, 0 }; // 60s timeout
	char buffer[WIFI_BUFFER_SIZE]; // buffer for receiving data
	const char* test_server = "8.8.8.8"; // test server ip for network connection
	const int validate_id = 10086; // an id for validating connecting device
	// set a seperate thread to receive data from connected device
	std::thread processThread;
};