#include "myBthHelper.hpp"
#include "myUI.hpp"
#include "myTrackpadHelper.hpp"

#include <initguid.h>
#include <mutex>

extern UIManager* myUIManager;
extern std::mutex lock_UI;
extern TrackpadManager* myTrackManager;
extern std::mutex lock_Track;

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
	myLocalSocketAddr.port = BT_PORT_ANY;
	// bind socket
	if (SOCKET_ERROR == bind(myLocalSocket, (SOCKADDR*)&myLocalSocketAddr, sizeof(SOCKADDR_BTH)))
	{
		UIManager::showWindowsMessageError("Failed to bind bluetooth socket\nPlease check your bluetooth is turned on");
		closesocket(myLocalSocket);
		WSACleanup();
		return;
	}
	// try to get socket name
	int addrlen = sizeof(SOCKADDR_BTH);
	if (SOCKET_ERROR == getsockname(myLocalSocket, (SOCKADDR*)&myLocalSocketAddr, &addrlen))
	{
		UIManager::showWindowsMessageError("Failed to get bluetooth socket name\nInitialization failed");
		closesocket(myLocalSocket);
		WSACleanup();
		return;
	}
	// prepare GUID
	GUID guid;
	HRESULT convert_result = CLSIDFromString(MY_UUID, (LPCLSID)&guid);
	// set query set
	CSADDR_INFO cSAddrInfo = { 0 };
	cSAddrInfo.LocalAddr.lpSockaddr = (LPSOCKADDR)&myLocalSocketAddr;
	cSAddrInfo.LocalAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
	cSAddrInfo.RemoteAddr.lpSockaddr = (LPSOCKADDR)&myLocalSocketAddr;
	cSAddrInfo.RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
	cSAddrInfo.iSocketType = SOCK_STREAM;
	cSAddrInfo.iProtocol = BTHPROTO_RFCOMM;
	std::wstring serviceName(MY_SERVICE_NAME);
	std::wstring serviceComment(MY_SERVICE_COMMENT);
	WSAQUERYSET querySet = { 0 };
	querySet.dwSize = sizeof(WSAQUERYSET);
	querySet.lpServiceClassId = (LPGUID)&guid;
	querySet.lpszServiceInstanceName = &serviceName[0];
	querySet.lpszComment = &serviceComment[0];
	querySet.dwNameSpace = NS_BTH;
	querySet.dwNumberOfCsAddrs = 1;
	querySet.lpcsaBuffer = &cSAddrInfo;
	// advertise service so that android devices can detect this server
	if (SOCKET_ERROR == WSASetService(&querySet, RNRSERVICE_REGISTER, 0))
	{
		UIManager::showWindowsMessageError("Failed to register bluetooth server\nInitialization failed");
		closesocket(myLocalSocket);
		WSACleanup();
		return;
	}
	initialized = true;
}

void BthManager::start()
{
	if (!initialized) return;
	lock_UI.lock();
	if (myUIManager)
		myUIManager->pushMessage("Bluetooth service starting (30s timeout)\nPlease wait");
	lock_UI.unlock();
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
	// set listen timeout (this is for device tmp search for connection test)
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
				myUIManager->pushMessage("Bluetooth cannot accept an incoming connection");
			lock_UI.unlock();
			return;
		}
		else
		{
			// if accept success, wait and accept the real connection
			myClientSocket = accept(myLocalSocket, (SOCKADDR*)&myClientSocketAddr, &addrlen);
			GLOB_LOCK.lock();
			GLOB_CONNECTED = true;
			GLOB_LOCK.unlock();
			// get device basic information
			char message[100];
			sprintf_s(message, "Device address = %02x:%02x:%02x:%02x:%02x:%02x",
				static_cast<unsigned char>((myClientSocketAddr.btAddr >> 40) & 0xFF),
				static_cast<unsigned char>((myClientSocketAddr.btAddr >> 32) & 0xFF),
				static_cast<unsigned char>((myClientSocketAddr.btAddr >> 24) & 0xFF), 
				static_cast<unsigned char>((myClientSocketAddr.btAddr >> 16) & 0xFF), 
				static_cast<unsigned char>((myClientSocketAddr.btAddr >> 8) & 0xFF), 
				static_cast<unsigned char>(myClientSocketAddr.btAddr & 0xFF));
			lock_UI.lock();
			if (myUIManager)
				myUIManager->pushMessage("Bluetooth Connected\n" + std::string(message));
			lock_UI.unlock();
			// start the process thread to process received data
			if (processThread.joinable())
				processThread.join();
			processThread = std::thread(&BthManager::process, this);
		}
	}
	else
	{
		lock_UI.lock();
		if (myUIManager)
			myUIManager->pushMessage("Bluetooth cannot find an incoming connection after timeout of 30s\nPlease try again");
		lock_UI.unlock();
		return;
	}
}

void BthManager::stop()
{
	GLOB_LOCK.lock();
	GLOB_CONNECTED = false;
	GLOB_LOCK.unlock();
	lock_UI.lock();
	if (myUIManager)
		myUIManager->pushMessage("Stopping bluetooth service\nIf wait a long time, try to disconnect from Android phone first");
	lock_UI.unlock();
	// wait for the process thread
	if(processThread.joinable())
		processThread.join();
	// close the client socket
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
		if (connection_should_stop)
		{
			connection_should_stop = false; // reset to default value and quit
			break;
		}
		result = recv(myClientSocket, buffer, BLUETOOTH_BUFFER_SIZE, 0);
		if (result > sizeof(uint32_t))
		{
			bool isLittleEndian = TrackpadManager::DATA_PACK::is_little_endian();
			char* ptr = buffer;
			uint32_t tmp = 0;
			int tmp_int = 0;
			int tmp_float = 0.0f;
			// first find the validation code position
			std::memcpy(&tmp, buffer, sizeof(uint32_t));
			tmp_int = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_int(tmp) : (int)tmp;
			while ((tmp_int != DATA_VALIDATION_CODE) && (result > sizeof(uint32_t)))
			{
				std::memcpy(&tmp, ptr, sizeof(uint32_t));
				tmp_int = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_int(tmp) : (int)tmp;
				ptr += sizeof(uint32_t); // find next 4 bytes
				result -= sizeof(uint32_t);
			}
			while (result >= (4 * sizeof(uint32_t)) && !GLOB_PROGRAM_EXIT)
			{
				TrackpadManager::DATA_PACK newData = { 0 };
				std::memcpy(&tmp, ptr, sizeof(uint32_t));
				tmp_int = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_int(tmp) : (int)tmp;
				if (tmp_int == DATA_VALIDATION_CODE)
				{
					std::memcpy(&tmp, ptr + sizeof(uint32_t), sizeof(uint32_t));
					tmp_int = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_int(tmp) : (int)tmp;
					newData.type = tmp_int;
					std::memcpy(&tmp, ptr + 2*sizeof(uint32_t), sizeof(uint32_t));
					tmp_float = isLittleEndian ? TrackpadManager::DATA_PACK::reverse_bytes_float(tmp) : (float)tmp;
					newData.velX = tmp_float;
					std::memcpy(&tmp, ptr + 3*sizeof(uint32_t), sizeof(uint32_t));
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
	}
	while (!GLOB_PROGRAM_EXIT);
	// close the client socket
	if (myClientSocket != INVALID_SOCKET)
	{
		closesocket(myClientSocket);
		myClientSocket = INVALID_SOCKET;
	}
	GLOB_LOCK.lock();
	GLOB_CONNECTED = false;
	GLOB_LOCK.unlock();
}