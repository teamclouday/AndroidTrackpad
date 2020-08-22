#pragma once

#include "myBthHelper.hpp"
#include "myWifiHelper.hpp"

// This class manage communication with android device
// via BthManager / WifiManager
class TransferManager
{
public:
	enum TransferType
	{
		TRANSFER_TYPE_BLUETOOTH = 0,
		TRANSFER_TYPE_WIFI_P2P  = 1,
	};
	TransferManager();
	~TransferManager();
	void processRequest();
private:

public:
	// set default values
	bool start_requested = false;
	bool stop_requested = false;
	TransferType transfer_type = TRANSFER_TYPE_WIFI_P2P;
private:
	// too subset managers
	BthManager* myBthManager = nullptr;
	WifiManager* myWifiManager = nullptr;
};