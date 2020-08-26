#include "myTrackpadHelper.hpp"
#include "myUI.hpp"

#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

#include <thread>
#include <iostream>

TrackpadManager::TrackpadManager() : buffer()
{
	root_display = XOpenDisplay(NULL);
	if(!root_display)
	{
		UIManager::showLinuxMessageError("Failed to get X11 root display");
		return;
	}
	initialized = true;
}

TrackpadManager::~TrackpadManager()
{
	buffer.clear();
	if(root_display)
		XCloseDisplay(root_display);
}

void TrackpadManager::process()
{
	std::this_thread::sleep_for(std::chrono::nanoseconds(100));
	if( buffer.size() > 0 )
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
		XTestFakeButtonEvent(root_display, Button1, True, 0);
		XFlush(root_display);
		std::this_thread::sleep_for(std::chrono::nanoseconds(10));
		XTestFakeButtonEvent(root_display, Button1, False, 0);
		XFlush(root_display);
	}
}

void TrackpadManager::mouseRightClick()
{
	XTestFakeButtonEvent(root_display, Button3, True, 0);
	XFlush(root_display);
	std::this_thread::sleep_for(std::chrono::nanoseconds(10));
	XTestFakeButtonEvent(root_display, Button3, False, 0);
	XFlush(root_display);
}

void TrackpadManager::scrollHorizontal(float delta)
{
	int button = (delta > 0) ? Button6 : Button7;
	XTestFakeButtonEvent(root_display, button, True, 0);
	XFlush(root_display);
	std::this_thread::sleep_for(std::chrono::nanoseconds(10));
	XTestFakeButtonEvent(root_display, button, False, 0);
	XFlush(root_display);
}

void TrackpadManager::scrollVertical(float delta)
{
	int button = (delta > 0) ? Button4 : Button5;
	XTestFakeButtonEvent(root_display, button, True, 0);
	XFlush(root_display);
	std::this_thread::sleep_for(std::chrono::nanoseconds(10));
	XTestFakeButtonEvent(root_display, button, False, 0);
	XFlush(root_display);
}

void TrackpadManager::dragStart()
{
	XTestFakeButtonEvent(root_display, Button1, True, 0);
	XFlush(root_display);
}

void TrackpadManager::dragStop()
{
	XTestFakeButtonEvent(root_display, Button1, False, 0);
	XFlush(root_display);
}

void TrackpadManager::move(float deltaX, float deltaY)
{
	deltaX *= sensitivity;
	deltaY *= sensitivity;
	Window root = DefaultRootWindow(root_display);
	XWarpPointer(root_display, root, None, 0, 0, 0, 0, static_cast<int>(deltaX), static_cast<int>(deltaY));
	XFlush(root_display);
}