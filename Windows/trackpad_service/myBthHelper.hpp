#pragma once

#include <WinSock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>

#include <string>

// reference1: https://docs.microsoft.com/en-us/windows/win32/bluetooth/bluetooth-programming-with-windows-sockets
// reference2: https://www.winsocketdotnetworkprogramming.com/winsock2programming/winsock2advancedotherprotocol4p.html

#define BLUETOOTH_BUFFER_SIZE 512

// This class manage bluetooth communication
class BthManager
{
	struct PackageInfo
	{
		uint32_t lengthReceived;
	};

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
private:
	const wchar_t* UUID = L"97c4eab8-234b-42d0-8c09-e9a5b1f1ba5b"; // connection unique ID
	SOCKET myLocalSocket = INVALID_SOCKET;
	SOCKET myClientSocket = INVALID_SOCKET;
	SOCKADDR_BTH myLocalSocketAddr;
	SOCKADDR_BTH myClientSocketAddr;
	const timeval myLocalSocketTimeout = { 60, 0 }; // 60s timeout
	char buffer[BLUETOOTH_BUFFER_SIZE]; // buffer for receiving data
};