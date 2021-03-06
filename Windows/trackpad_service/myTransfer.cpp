#include "myTransfer.hpp"
#include "myUI.hpp"

#include <mutex>
#include <thread>
#include <chrono>

extern bool GLOB_CONNECTED;
extern std::mutex GLOB_LOCK;
extern UIManager* myUIManager;
extern std::mutex lock_UI;

TransferManager::TransferManager()
{

}

TransferManager::~TransferManager()
{
	if (myBthManager)
		delete myBthManager;
	if (myWifiManager)
		delete myWifiManager;
}

void TransferManager::processRequest()
{
	// if no request, sleep for 50 milliseconds
	if (!(start_requested || stop_requested))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		return;
	}
	// if type is bluetooth
	if (transfer_type == TRANSFER_TYPE_BLUETOOTH)
	{
		if (!myBthManager)
		{
			if (stop_requested)
			{
				// if not initialized and stop button clicked, ignore
				stop_requested = false;
				return;
			}
			myBthManager = new BthManager();
			if (!myBthManager->initialized)
			{
				start_requested = false;
				stop_requested = false;
				myBthManager = nullptr;
				return;
			}
		}
		if (start_requested)
		{
			// if connected, skip request
			if (GLOB_CONNECTED)
			{
				lock_UI.lock();
				if(myUIManager)
					myUIManager->pushMessage("Device has already been connected");
				lock_UI.unlock();
			}
			else
			{
				myBthManager->start();
			}
			start_requested = false;
		}
		if (stop_requested)
		{
			if (GLOB_CONNECTED)
			{
				myBthManager->connection_should_stop = true;
				myBthManager->stop();
				lock_UI.lock();
				if (myUIManager)
					myUIManager->pushMessage("Bluetooth service has stopped");
				lock_UI.unlock();
			}
			else
			{
				lock_UI.lock();
				if (myUIManager)
					myUIManager->pushMessage("You have not started service yet");
				lock_UI.unlock();
			}
			stop_requested = false;
		}
	}
	// if type is wifi
	else if (transfer_type == TRANSFER_TYPE_WIFI_P2P)
	{
		if (!myWifiManager)
		{
			if (stop_requested)
			{
				// if not initialized and stop button clicked, ignore
				stop_requested = false;
				return;
			}
			myWifiManager = new WifiManager();
			if (!myWifiManager->initialized)
			{
				start_requested = false;
				stop_requested = false;
				myWifiManager = nullptr;
				return;
			}
		}
		if (start_requested)
		{
			// if connected, skip request
			if (GLOB_CONNECTED)
			{
				lock_UI.lock();
				if (myUIManager)
					myUIManager->pushMessage("Device has already been connected");
				lock_UI.unlock();
			}
			else
			{
				myWifiManager->start();
			}
			start_requested = false;
		}
		if (stop_requested)
		{
			if (GLOB_CONNECTED)
			{
				myWifiManager->connection_should_stop = true;
				myWifiManager->stop();
				lock_UI.lock();
				if (myUIManager)
					myUIManager->pushMessage("Wifi P2P service has stopped");
				lock_UI.unlock();
			}
			else
			{
				lock_UI.lock();
				if (myUIManager)
					myUIManager->pushMessage("You have not started service yet");
				lock_UI.unlock();
			}
			stop_requested = false;
		}
	}
}