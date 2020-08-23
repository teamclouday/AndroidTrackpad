#include "myWifiHelper.hpp"
#include "myUI.hpp"

#include <initguid.h>
#include <mutex>

extern UIManager* myUIManager;
extern std::mutex lock_UI;
extern bool GLOB_CONNECTED;
extern bool GLOB_PROGRAM_EXIT;
extern std::mutex GLOB_LOCK;

WifiManager::WifiManager()
{
	initialize();
}

WifiManager::~WifiManager()
{
	stop();
	closesocket(myLocalSocket);
	WSACleanup();
}

void WifiManager::initialize()
{
	// init Winsock version 2.2
	WSADATA data = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &data))
	{
		UIManager::showWindowsMessageError("Failed to initialize Winsock version 2.2");
		return;
	}
	// setup config
	ADDRINFO* results = nullptr;
	ADDRINFO hints = { 0 };
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	// get server address and port info
	if (getaddrinfo(NULL, myLocalPort, &hints, &results))
	{
		UIManager::showWindowsMessageError("Failed to get server address information");
		WSACleanup();
		return;
	}
	// create local socket
	myLocalSocket = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
	if (INVALID_SOCKET == myLocalSocket)
	{
		UIManager::showWindowsMessageError("Failed to open Wifi TCP socket");
		freeaddrinfo(results);
		WSACleanup();
		return;
	}
	// bind local socket
	if (SOCKET_ERROR == bind(myLocalSocket, results->ai_addr, (int)results->ai_addrlen))
	{
		UIManager::showWindowsMessageError("Failed to bind Wifi TCP socket");
		freeaddrinfo(results);
		closesocket(myLocalSocket);
		WSACleanup();
		return;
	}
	// show current PC ip address information
	lock_UI.lock();
	if (myUIManager)
	{
		char addrbuff[30];
		inet_ntop(AF_INET, &results->ai_addr, addrbuff, 30);
		myUIManager->pushMessage("Current device IP = " + std::string(addrbuff));
	}
	lock_UI.unlock();
	freeaddrinfo(results);
	initialized = true;
}

void WifiManager::start()
{
	if (!initialized) return;
	lock_UI.lock();
	if (myUIManager)
		myUIManager->pushMessage("Wifi service starting (30s timeout)\nPlease wait");
	lock_UI.unlock();
	// start listening
	if (SOCKET_ERROR == listen(myLocalSocket, SOMAXCONN))
	{
		lock_UI.lock();
		if (myUIManager)
			myUIManager->pushMessage("Error: Wifi failed to start listening");
		lock_UI.unlock();
		return;
	}
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(myLocalSocket, &fds);
	// set listen timeout
	select(myLocalSocket + 1, &fds, NULL, NULL, &myLocalSocketTimeout);
	if (FD_ISSET(myLocalSocket, &fds))
	{
		int addrlen = sizeof(myClientSocketAddr);
		// accept the first connect for connection test
		myClientSocket = accept(myLocalSocket, (SOCKADDR*)&myClientSocketAddr, &addrlen);
		if (myClientSocket == INVALID_SOCKET)
		{
			lock_UI.lock();
			if (myUIManager)
				myUIManager->pushMessage("Wifi cannot accept an incoming connection");
			lock_UI.unlock();
			return;
		}
		else
		{
			if (validate())
			{
				lock_UI.lock();
				if (myUIManager)
					myUIManager->pushMessage("Wifi cannot accept an unidentified device\nPlease try again");
				lock_UI.unlock();
				return;
			}
			else
			{
				// after validation of client, accept the real connection
				myClientSocket = accept(myLocalSocket, (SOCKADDR*)&myClientSocketAddr, &addrlen);
				GLOB_LOCK.lock();
				GLOB_CONNECTED = true;
				GLOB_LOCK.unlock();
				// get device basic information
				char message[100];
				char addrbuff[30];
				inet_ntop(AF_INET, &myClientSocketAddr, addrbuff, 30);
				sprintf_s(message, "Device address = %s", addrbuff);
				lock_UI.lock();
				if (myUIManager)
					myUIManager->pushMessage("Wifi P2P Connected\n" + std::string(message));
				lock_UI.unlock();
				// start the process thread to process received data
				processThread = std::thread(&WifiManager::process, this);
			}
		}
	}
	else
	{
		lock_UI.lock();
		if (myUIManager)
			myUIManager->pushMessage("Wifi cannot find an incoming connection after timeout of 30s\nPlease try again");
		lock_UI.unlock();
		return;
	}
}

void WifiManager::stop()
{
	// first try to wait for the process thread
	if (processThread.joinable())
		processThread.join();
	GLOB_LOCK.lock();
	GLOB_CONNECTED = false;
	GLOB_LOCK.unlock();
	if (myClientSocket != INVALID_SOCKET)
	{
		// close the client socket
		closesocket(myClientSocket);
		myClientSocket = INVALID_SOCKET;
	}
}

void WifiManager::process()
{
	int result = 0;
	do
	{
		if (GLOB_PROGRAM_EXIT) break;
		if (connection_should_stop)
		{
			connection_should_stop = false; // reset to default value and quit
			break;
		}
		result = recv(myClientSocket, buffer, WIFI_BUFFER_SIZE, 0);
		lock_UI.lock();
		if (result > 0)
		{
			char message[100];
			sprintf_s(message, "Received %d bytes from device", result);
			if (myUIManager)
				myUIManager->pushMessage(std::string(message));
		}
		else if (result == 0)
		{
			if (myUIManager)
				myUIManager->pushMessage("Device has disconnected");
		}
		else
		{
			if (myUIManager)
				myUIManager->pushMessage("An error occured during connection");
		}
		lock_UI.unlock();
	} while (result > 0);
	// after receiving data, should set global state
	GLOB_LOCK.lock();
	GLOB_CONNECTED = false;
	GLOB_LOCK.unlock();
	// and close client socket
	closesocket(myClientSocket);
	myClientSocket = INVALID_SOCKET;
}

bool WifiManager::validate()
{

	return true;
}