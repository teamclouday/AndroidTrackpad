#pragma once

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
public:
	bool initialized = false;
private:
};