#include "myBthHelper.hpp"
#include "myUI.hpp"

#include <initguid.h>
#include <mutex>

extern UIManager* myUIManager;
extern std::mutex lock_UI;
extern bool GLOB_CONNECTED;
extern bool GLOB_PROGRAM_EXIT;
extern std::mutex GLOB_LOCK;

BthManager::BthManager()
{
	initialize();
}

BthManager::~BthManager()
{
	stop();
	closesocket(myLocalSocket);
	WSACleanup();
}

void BthManager::initialize()
{
	// init Winsock version 2.2
	WSADATA data = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &data))
	{
		UIManager::showWindowsMessageError("Failed to initialize Winsock version 2.2");
		return;
	}
	// open a bluetooth socket using RFCOMM protocol
	myLocalSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
	if (INVALID_SOCKET == myLocalSocket)
	{
		UIManager::showWindowsMessageError("Failed to open bluetooth socket");
		WSACleanup();
		return;
	}
	myLocalSocketAddr.addressFamily = AF_BTH;
	myLocalSocketAddr.port = 0;
	// setup UUID
	GUID guid;
	HRESULT convert_result = IIDFromString(UUID, (LPCLSID)&guid);
	myLocalSocketAddr.serviceClassId = guid;
	// bind socket
	if (SOCKET_ERROR == bind(myLocalSocket, (struct sockaddr*)&myLocalSocketAddr, sizeof(SOCKADDR_BTH)))
	{
		UIManager::showWindowsMessageError("Failed to bind bluetooth socket");
		closesocket(myLocalSocket);
		WSACleanup();
		return;
	}
	initialized = true;
}

void BthManager::start()
{
	if (!initialized) return;
	// start listening
	if (SOCKET_ERROR == listen(myLocalSocket, 1))
	{
		lock_UI.lock();
		if(myUIManager)
			myUIManager->pushMessage("Error: bluetooth failed to start listening");
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
		myClientSocket = accept(myLocalSocket, (SOCKADDR*)&myClientSocketAddr, &addrlen);
		if (myClientSocket == INVALID_SOCKET)
		{
			lock_UI.lock();
			if (myUIManager)
				myUIManager->pushMessage("Bluetooth cannot accept an incoming connection");
			lock_UI.unlock();
			return;
		}
		else
		{
			GLOB_LOCK.lock();
			GLOB_CONNECTED = true;
			GLOB_LOCK.unlock();

			char message[100];
			sprintf_s(message, "Device address = %04x%08x", GET_NAP(myClientSocketAddr.btAddr), GET_SAP(myClientSocketAddr.btAddr));
			lock_UI.lock();
			if (myUIManager)
				myUIManager->pushMessage("Bluetooth Connected\n" + std::string(message));
			lock_UI.unlock();

			process();
		}
	}
	else
	{
		lock_UI.lock();
		if (myUIManager)
			myUIManager->pushMessage("Bluetooth cannot find an incoming connection after timeout of 60s\nPlease try again");
		lock_UI.unlock();
		return;
	}
}

void BthManager::stop()
{
	GLOB_LOCK.lock();
	GLOB_CONNECTED = false;
	GLOB_LOCK.unlock();
	if (myClientSocket != INVALID_SOCKET)
	{
		closesocket(myClientSocket);
		myClientSocket = INVALID_SOCKET;
	}
}

void BthManager::process()
{
	int result = 0;
	do
	{
		if (GLOB_PROGRAM_EXIT) break;
		result = recv(myClientSocket, buffer, BLUETOOTH_BUFFER_SIZE, 0);
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
	}
	while (result > 0);
	stop();
}