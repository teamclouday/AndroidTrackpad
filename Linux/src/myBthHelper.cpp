#include "myBthHelper.hpp"
#include "myUI.hpp"
#include "myTrackpadHelper.hpp"

#include <unistd.h>

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
	close(myLocalSocket);
	if(myServiceSession)
	{
		sdp_close(myServiceSession);
		myServiceSession = nullptr;
	}
}

void BthManager::initialize()
{
	// open a bluetooth socket using RFCOMM protocol
	myLocalSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (INVALID_SOCKET == myLocalSocket)
	{
		UIManager::showWindowsMessageError("Failed to open bluetooth socket");
		return;
	}
	// define BDADDR_ANY and BDADDR_LOCAL
	bdaddr_t bdaddr_any =  {{0, 0, 0, 0, 0, 0}};
	bdaddr_t bdaddr_local = {{0, 0, 0, 0xFF, 0xFF, 0xFF}};
	myLocalSocketAddr.rc_family = AF_BLUETOOTH;
	myLocalSocketAddr.rc_bdaddr = bdaddr_any;
	myLocalSocketAddr.rc_channel = MY_CHANNEL;
	// bind socket
	if (SOCKET_ERROR == bind(myLocalSocket, (struct sockaddr*)&myLocalSocketAddr, sizeof(struct sockaddr_rc)))
	{
		UIManager::showWindowsMessageError("Failed to bind bluetooth socket\nPlease check your bluetooth is turned on");
		close(myLocalSocket);
		return;
	}
	// try to get socket name
	socklen_t addrlen = sizeof(struct sockaddr_rc);
	if (SOCKET_ERROR == getsockname(myLocalSocket, (struct sockaddr*)&myLocalSocketAddr, &addrlen))
	{
		UIManager::showWindowsMessageError("Failed to get bluetooth socket name\nInitialization failed");
		close(myLocalSocket);
		return;
	}
	// prepare sdp registration
	uuid_t root_uuid, l2cap_uuid, rfcomm_uuid, svc_uuid;
    sdp_list_t *l2cap_list = 0, 
               *rfcomm_list = 0,
               *root_list = 0,
               *proto_list = 0, 
               *access_proto_list = 0;
    sdp_data_t *channel = 0;

    sdp_record_t *record = sdp_record_alloc();

    // set the general service ID
    sdp_uuid128_create(&svc_uuid, &MY_UUID);
    sdp_set_service_id(record, svc_uuid);

    // make the service record publicly browsable
    sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
    root_list = sdp_list_append(0, &root_uuid);
    sdp_set_browse_groups(record, root_list);

    // set l2cap information
    sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
    l2cap_list = sdp_list_append(0, &l2cap_uuid);
    proto_list = sdp_list_append(0, l2cap_list);

    // set rfcomm information
    sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
    channel = sdp_data_alloc(SDP_UINT8, &MY_CHANNEL);
    rfcomm_list = sdp_list_append( 0, &rfcomm_uuid );
    sdp_list_append( rfcomm_list, channel );
    sdp_list_append( proto_list, rfcomm_list );

    // attach protocol information to service record
    access_proto_list = sdp_list_append( 0, proto_list );
    sdp_set_access_protos( record, access_proto_list );

    // set the name, provider, and description
    sdp_set_info_attr(record, MY_SERVICE_NAME, MY_SERVICE_PROV, MY_SERVICE_DESC);

    // connect to the local SDP server, register the service record, and 
    // disconnect
    myServiceSession = sdp_connect(&bdaddr_any, &bdaddr_local, SDP_RETRY_IF_BUSY);
    if(!sdp_record_register(myServiceSession, record, 0))
	{
		UIManager::showWindowsMessageError("Failed to register local sdp record for bluetooth");
		close(myLocalSocket);
		sdp_data_free(channel);
    	sdp_list_free(l2cap_list, 0);
    	sdp_list_free(rfcomm_list, 0);
    	sdp_list_free(root_list, 0);
    	sdp_list_free(access_proto_list, 0);
		if(myServiceSession)
		{
			sdp_close(myServiceSession);
			myServiceSession = nullptr;
		}
		return;
	}

    // cleanup
    sdp_data_free(channel);
    sdp_list_free(l2cap_list, 0);
    sdp_list_free(rfcomm_list, 0);
    sdp_list_free(root_list, 0);
    sdp_list_free(access_proto_list, 0);
	
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
		socklen_t addrlen = sizeof(myClientSocketAddr);
		// accept the first connect for connection test
		myClientSocket = accept(myLocalSocket, (struct sockaddr*)&myClientSocketAddr, &addrlen);
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
			myClientSocket = accept(myLocalSocket, (struct sockaddr*)&myClientSocketAddr, &addrlen);
			GLOB_LOCK.lock();
			GLOB_CONNECTED = true;
			GLOB_LOCK.unlock();
			// get device basic information
			char buffer[100] = { 0 };
			ba2str(&myClientSocketAddr.rc_bdaddr, buffer);
			lock_UI.lock();
			if (myUIManager)
				myUIManager->pushMessage("Bluetooth Connected\nDevice address = " + std::string(buffer));
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
		close(myClientSocket);
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
		close(myClientSocket);
		myClientSocket = INVALID_SOCKET;
	}
	GLOB_LOCK.lock();
	GLOB_CONNECTED = false;
	GLOB_LOCK.unlock();
}