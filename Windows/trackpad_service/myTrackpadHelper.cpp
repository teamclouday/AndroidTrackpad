#include "myTrackpadHelper.hpp"

#include <Windows.h>

TrackpadManager::TrackpadManager() : buffer()
{

}

TrackpadManager::~TrackpadManager()
{
	buffer.clear();
}

void TrackpadManager::process()
{
	if (buffer.size() <= 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	else
	{
		DATA_PACK data;
		lock.lock();
		data = buffer.front();
		buffer.pop_front();
		lock.unlock();
		switch ((DATA_TYPE)data.type)
		{
		case DATA_TYPE::DATA_TYPE_CLICK_LEFT:
			mouseLeftClick(data.velX);
			break;
		case DATA_TYPE::DATA_TYPE_CLICK_RIGHT:
			mouseRightClick();
			break;
		case DATA_TYPE::DATA_TYPE_DRAG:
			if (!dragging)
			{
				dragStart();
				dragging = true;
			}
			move(data.velX, data.velY);
			break;
		case DATA_TYPE::DATA_TYPE_MOVE:
			move(data.velX, data.velY);
			break;
		case DATA_TYPE::DATA_TYPE_SCROLL_HORI:
			scrollHorizontal(data.velX);
			break;
		case DATA_TYPE::DATA_TYPE_SCROLL_VERT:
			scrollVertical(data.velY);
			break;
		}
		if (dragging)
		{
			if(((DATA_TYPE)data.type != DATA_TYPE::DATA_TYPE_DRAG) && 
				((DATA_TYPE)data.type != DATA_TYPE::DATA_TYPE_MOVE))
				dragging = false;
		}
	}
}

void TrackpadManager::addData(DATA_PACK newData)
{
	lock.lock();
	buffer.push_back(newData);
	while (buffer.size() > DATA_PACK_MAX)
		buffer.pop_front();
	lock.unlock();
}

void TrackpadManager::mouseLeftClick(float flag)
{
	if (flag)
	{
		// if not 0, perform dragStop
		dragStop();
	}
	else
	{
		// if 0, perform normal left click
		INPUT ip[2] = { 0 };
		ip[0].type = INPUT_MOUSE;
		ip[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		ip[1].type = INPUT_MOUSE;
		ip[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(2, ip, sizeof(INPUT));
		// std::cout << "mouseLeftClick" << std::endl;
	}
}

void TrackpadManager::mouseRightClick()
{
	INPUT ip[2] = { 0 };
	ip[0].type = INPUT_MOUSE;
	ip[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	ip[1].type = INPUT_MOUSE;
	ip[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	SendInput(2, ip, sizeof(INPUT));
	// std::cout << "mouseRightClick" << std::endl;
}

void TrackpadManager::scrollHorizontal(float delta)
{
	int sign = (delta > 0) ? -1 : 1;
	INPUT ip = { 0 };
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_HWHEEL;
	ip.mi.mouseData = (DWORD)(sign * WHEEL_DELTA);
	SendInput(1, &ip, sizeof(INPUT));
}

void TrackpadManager::scrollVertical(float delta)
{
	int sign = (delta > 0) ? 1 : -1;
	INPUT ip = { 0 };
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_WHEEL;
	ip.mi.mouseData = (DWORD)(sign * WHEEL_DELTA);
	SendInput(1, &ip, sizeof(INPUT));
}

void TrackpadManager::dragStart()
{
	INPUT ip = { 0 };
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN;
	SendInput(2, &ip, sizeof(INPUT));
	// std::cout << "dragStart" << std::endl;
}

void TrackpadManager::dragStop()
{
	INPUT ip = { 0 };
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(1, &ip, sizeof(INPUT));
	// std::cout << "dragStop" << std::endl;
}

void TrackpadManager::move(float deltaX, float deltaY)
{
	INPUT ip = { 0 };
	ip.type = INPUT_MOUSE;
	if (dragging)
	{
		ip.mi.dwFlags = MOUSEEVENTF_MOVE;
		// std::cout << "drag" << std::endl;
	}
	else
	{
		ip.mi.dwFlags = MOUSEEVENTF_MOVE;
		// std::cout << "move" << std::endl;
	}
	ip.mi.dx = deltaX * sensitivity;
	ip.mi.dy = deltaY * sensitivity;
	SendInput(1, &ip, sizeof(INPUT));
}