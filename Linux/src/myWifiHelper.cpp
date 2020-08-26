#include "myWifiHelper.hpp"
#include "myUI.hpp"
#include "myTrackpadHelper.hpp"

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <mutex>
#include <iostream>

extern UIManager* myUIManager;
extern std::mutex lock_UI;
extern TrackpadManager* myTrackManager;
extern std::mutex lock_Track;

extern bool GLOB_CONNECTED;
extern bool GLOB_PROGRAM_EXIT;
extern std::mutex GLOB_LOCK;

WifiManager::WifiManager()
{
	initialized = true;
}

WifiManager::~WifiManager()
{
	stop();
	close(myLocalSocket);
}

bool WifiManager::initialize()
{
	if(myLocalSocket != INVALID_SOCKET)
		close(myLocalSocket);
	// get adapter information
	struct sockaddr_in testAddr;
	int testSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == testSocket)
	{
		lock_UI.lock();
		if (myUIManager)
			myUIManager->pushMessage("Failed to open Wifi socket for test");
		lock_UI.unlock();
		return false;
	}
	memset(&testAddr, 0, sizeof(testAddr));
	testAddr.sin_family = AF_INET;
	inet_pton(AF_INET, test_server, &testAddr.sin_addr);
	testAddr.sin_port = htons(53);
	if (SOCKET_ERROR == connect(testSocket, (struct sockaddr*)&testAddr, sizeof(testAddr)))
	{
		lock_UI.lock();
		if (myUIManager)
			myUIManager->pushMessage(std::string("Failed to connect to test dns server: ") + test_server);
		lock_UI.unlock();
		close(testSocket);
		return false;
	}
	struct sockaddr_in result;
	socklen_t len = sizeof(result);
	if (SOCKET_ERROR == getsockname(testSocket, (struct sockaddr*)&result, &len))
	{
		lock_UI.lock();
		if (myUIManager)
			myUIManager->pushMessage("Failed to get Wifi test socket information");
		lock_UI.unlock();
		close(testSocket);
		return false;
	}
	close(testSocket);
	// save current PC ip address information
	char addrbuff[30];
	inet_ntop(AF_INET, &result.sin_addr, addrbuff, 30);
	localIP = std::string(addrbuff);
	// create local socket
	struct sockaddr_in hint = {};
	hint.sin_family = AF_INET;
	hint.sin_port = htons(myLocalPort);
	hint.sin_addr = result.sin_addr;
	len = sizeof(hint);
	myLocalSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == myLocalSocket)
	{
		lock_UI.lock();
		if (myUIManager)
			myUIManager->pushMessage("Failed to open Wifi TCP socket");
		lock_UI.unlock();
		return false;
	}
	// bind local socket
	if (SOCKET_ERROR == bind(myLocalSocket, (struct sockaddr*)&hint, len))
	{
		lock_UI.lock();
		if (myUIManager)
			myUIManager->pushMessage("Failed to bind Wifi TCP socket");
		lock_UI.unlock();
		close(myLocalSocket);
		return false;
	}
	
	return true;
}

void WifiManager::start()
{
	if (!initialize()) return;
	lock_UI.lock();
	if (myUIManager)
		myUIManager->pushMessage("Wifi service starting (60s timeout)\nPlease wait\nYour Local IP = " + localIP);
	lock_UI.unlock();
	// start listening
	if (SOCKET_ERROR == listen(myLocalSocket, 1))
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
		socklen_t addrlen = sizeof(myClientSocketAddr);
		// accept the first connect for connection test
		myClientSocket = accept(myLocalSocket, (struct sockaddr*)&myClientSocketAddr, &addrlen);
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
			if (!validate())
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
				myClientSocket = accept(myLocalSocket, (struct sockaddr*)&myClientSocketAddr, &addrlen);
				GLOB_LOCK.lock();
				GLOB_CONNECTED = true;
				GLOB_LOCK.unlock();
				// get device basic information
				char message[100];
				char addrbuff[30];
				inet_ntop(AF_INET, &myClientSocketAddr, addrbuff, 30);
				sprintf(message, "Device address = %s", addrbuff);
				lock_UI.lock();
				if (myUIManager)
					myUIManager->pushMessage("Wifi P2P Connected\n" + std::string(message));
				lock_UI.unlock();
				// start the process thread to process received data
				if (processThread.joinable())
					processThread.join();
				processThread = std::thread(&WifiManager::process, this);
			}
		}
	}
	else
	{
		lock_UI.lock();
		if (myUIManager)
			myUIManager->pushMessage("Wifi cannot find an incoming connection after timeout of 60s\nPlease try again");
		lock_UI.unlock();
		return;
	}
}

void WifiManager::stop()
{
	GLOB_LOCK.lock();
	GLOB_CONNECTED = false;
	GLOB_LOCK.unlock();
	lock_UI.lock();
	if (myUIManager)
		myUIManager->pushMessage("Stopping Wifi service\nIf wait a long time, try to disconnect from Android phone first");
	lock_UI.unlock();
	// wait for the process thread
	if (processThread.joinable())
		processThread.join();
	// close the client socket
	if (myClientSocket != INVALID_SOCKET)
	{
		close(myClientSocket);
		myClientSocket = INVALID_SOCKET;
	}
}

void WifiManager::process()
{
	int result = 0;
	do
	{
		if (connection_should_stop)
		{
			connection_should_stop = false; // reset to default value and quit
			break;
		}
		result = recv(myClientSocket, buffer, WIFI_BUFFER_SIZE, 0);
		if (result > static_cast<int>(sizeof(uint32_t)))
		{
			bool isLittleEndian = TrackpadManager::DATA_PACK::is_little_endian();
			char* ptr = buffer;
			uint32_t tmp = 0;
			int tmp_int = 0;
			int tmp_float = 0.0f;
			// first find the validation code position
			std::memcpy(&tmp, buffer, sizeof(uint32_t));
			tmp_int = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_int(tmp) : (int)tmp;
			while ((tmp_int != DATA_VALIDATION_CODE) && (result > static_cast<int>(sizeof(uint32_t))))
			{
				std::memcpy(&tmp, ptr, sizeof(uint32_t));
				tmp_int = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_int(tmp) : (int)tmp;
				ptr += sizeof(uint32_t); // find next 4 bytes
				result -= sizeof(uint32_t);
			}
			while (result >= (4 * static_cast<int>(sizeof(uint32_t))) && !GLOB_PROGRAM_EXIT)
			{
				TrackpadManager::DATA_PACK newData = {};
				std::memcpy(&tmp, ptr, sizeof(uint32_t));
				tmp_int = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_int(tmp) : (int)tmp;
				if (tmp_int == DATA_VALIDATION_CODE)
				{
					std::memcpy(&tmp, ptr + sizeof(uint32_t), sizeof(uint32_t));
					tmp_int = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_int(tmp) : (int)tmp;
					newData.type = tmp_int;
					std::memcpy(&tmp, ptr + 2 * sizeof(uint32_t), sizeof(uint32_t));
					tmp_float = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_float(tmp) : (float)tmp;
					newData.velX = tmp_float;
					std::memcpy(&tmp, ptr + 3 * sizeof(uint32_t), sizeof(uint32_t));
					tmp_float = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_float(tmp) : (float)tmp;
					newData.velY = tmp_float;
					lock_Track.lock();
					if (myTrackManager)
						myTrackManager->addData(newData);
					lock_Track.unlock();
					ptr += 4 * sizeof(uint32_t);
					result -= 4 * sizeof(uint32_t);
				}
				else
				{
					ptr += sizeof(uint32_t);
					result -= sizeof(uint32_t);
				}
			}
		}
		else if (result == 0)
		{
			lock_UI.lock();
			if (myUIManager)
				myUIManager->pushMessage("Device has disconnected");
			lock_UI.unlock();
			break;
		}
		else if(result < 0)
		{
			lock_UI.lock();
			if (myUIManager)
				myUIManager->pushMessage("An error occured during connection");
			lock_UI.unlock();
			break;
		}
	} while (!GLOB_PROGRAM_EXIT);
	// close the client socket
	if (myClientSocket != INVALID_SOCKET)
	{
		close(myClientSocket);
		myClientSocket = INVALID_SOCKET;
	}
	GLOB_LOCK.lock();
	GLOB_CONNECTED = false;
	GLOB_LOCK.unlock();
}

bool WifiManager::validate()
{
	if (myClientSocket == INVALID_SOCKET) return false;
	char bytes[4];
	int res = recv(myClientSocket, bytes, sizeof(int), 0);
	if (res <= 0) return false;
	uint32_t validation;
	std::memcpy(&validation, bytes, sizeof(uint32_t));
	int check = (TrackpadManager::DATA_PACK::is_little_endian()) ? TrackpadManager::DATA_PACK::reverse_bytes_int(validation) : (int)validation;
	return (check == DATA_VALIDATION_CODE);
}