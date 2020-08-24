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
		if (dragging)
		{
			mouseLeftClick();
			dragging = false;
		}
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
			mouseLeftClick();
			break;
		case DATA_TYPE::DATA_TYPE_CLICK_RIGHT:
			mouseRightClick();
			break;
		case DATA_TYPE::DATA_TYPE_DRAG:
			if (!dragging)
				dragStart();
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
		if ((DATA_TYPE)data.type != DATA_TYPE::DATA_TYPE_DRAG && dragging)
		{
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

void TrackpadManager::mouseLeftClick()
{
	INPUT ip[2] = { 0 };
	ip[0].type = INPUT_MOUSE;
	ip[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	ip[1].type = INPUT_MOUSE;
	ip[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(2, ip, sizeof(INPUT));
}

void TrackpadManager::mouseRightClick()
{
	INPUT ip[2] = { 0 };
	ip[0].type = INPUT_MOUSE;
	ip[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	ip[1].type = INPUT_MOUSE;
	ip[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	SendInput(2, ip, sizeof(INPUT));
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

// note: stop drag event will be unnecessary
// because a single left click event will be sent right after dragging
void TrackpadManager::dragStart()
{
	INPUT ip = { 0 };
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	SendInput(1, &ip, sizeof(INPUT));
}

void TrackpadManager::move(float deltaX, float deltaY)
{
	INPUT ip = { 0 };
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_MOVE;
	ip.mi.dx = deltaX * sensitivity;
	ip.mi.dy = deltaY * sensitivity;
	SendInput(1, &ip, sizeof(INPUT));
}