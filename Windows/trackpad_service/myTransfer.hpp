#pragma once

#include "myBthHelper.hpp"
#include "myWifiHelper.hpp"

#include "myUI.hpp"

#include <mutex>

// This class manage communication with android device
// via BthManager / WifiManager
class TransferManager
{
friend UIManager;
public:
	enum TransferType
	{
		TRANSFER_TYPE_BLUETOOTH = 0,
		TRANSFER_TYPE_WIFI_P2P  = 1,
	};
	TransferManager();
	~TransferManager();
private:

public:

protected:
	TransferType transfer_type = TRANSFER_TYPE_WIFI_P2P;
};