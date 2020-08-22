#include "main.hpp"

UIManager* myUIManager;
std::mutex lock_UI;
TransferManager* myTranManager;
std::mutex lock_Transfer;
bool GLOB_PROGRAM_EXIT;
std::mutex GLOB_LOCK;

int main(int, char**)
{
	GLOB_PROGRAM_EXIT = false;

	std::thread thread_UI(task_UI);
	std::thread thread_Transfer(task_Transfer);
	std::thread thread_Trackpad(task_Trackpad);

	while (!GLOB_PROGRAM_EXIT)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	thread_Trackpad.join();
	thread_Transfer.join();
	thread_UI.join();

	return 0;
}

void task_UI()
{
	myUIManager = new UIManager();
	if(myUIManager->initialized)
		myUIManager->loop();
	else
	{
		GLOB_LOCK.lock();
		GLOB_PROGRAM_EXIT = true;
		GLOB_LOCK.unlock();
	}
}

void task_Transfer()
{
	myTranManager = new TransferManager();
}

void task_Trackpad()
{

}