#include "myTrackpadHelper.hpp"
#include "myUI.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <thread>
#include <iostream>

TrackpadManager::TrackpadManager() : buffer()
{

}

TrackpadManager::~TrackpadManager()
{
	buffer.clear();
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
		Display *display = XOpenDisplay(NULL);
    	if(display == NULL)
    	{
    	    fprintf(stderr, "Errore nell'apertura del Display !!!\n");
    	    exit(EXIT_FAILURE);
    	}
    	XEvent event;
		memset(&event, 0, sizeof(XEvent));
    	event.type = ButtonPress;
    	event.xbutton.button = Button1;
    	event.xbutton.same_screen = True;
    	XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
		event.xbutton.subwindow = event.xbutton.window;
    	while(event.xbutton.subwindow)
    	{
    	    event.xbutton.window = event.xbutton.subwindow;
    	    XQueryPointer(display, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    	}
		if(!XSendEvent(display, PointerWindow, True, 0xfff, &event))
			std::cerr << "Error" << std::endl;
		XFlush(display);
		event.type = ButtonRelease;
    	event.xbutton.state = 0x100;
		if(!XSendEvent(display, PointerWindow, True, 0xfff, &event))
			std::cerr << "Error" << std::endl;
		XFlush(display);
		XCloseDisplay(display);
		// std::cout << "mouseLeftClick" << std::endl;
	}
}

void TrackpadManager::mouseRightClick()
{
	// INPUT ip[2] = { 0 };
	// ip[0].type = INPUT_MOUSE;
	// ip[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	// ip[1].type = INPUT_MOUSE;
	// ip[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	// SendInput(2, ip, sizeof(INPUT));
}

void TrackpadManager::scrollHorizontal(float delta)
{
	// int sign = (delta > 0) ? -1 : 1;
	// INPUT ip = { 0 };
	// ip.type = INPUT_MOUSE;
	// ip.mi.dwFlags = MOUSEEVENTF_HWHEEL;
	// ip.mi.mouseData = sign * static_cast<DWORD>(WHEEL_DELTA * sensitivity);
	// SendInput(1, &ip, sizeof(INPUT));
}

void TrackpadManager::scrollVertical(float delta)
{
	// int sign = (delta > 0) ? 1 : -1;
	// INPUT ip = { 0 };
	// ip.type = INPUT_MOUSE;
	// ip.mi.dwFlags = MOUSEEVENTF_WHEEL;
	// ip.mi.mouseData = sign * static_cast<DWORD>(WHEEL_DELTA * sensitivity);
	// SendInput(1, &ip, sizeof(INPUT));
}

void TrackpadManager::dragStart()
{
	// INPUT ip = { 0 };
	// ip.type = INPUT_MOUSE;
	// ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	// SendInput(1, &ip, sizeof(INPUT));
	// dragging = true;
	// std::cout << "dragStart" << std::endl;
}

void TrackpadManager::dragStop()
{
	// INPUT ip = { 0 };
	// ip.type = INPUT_MOUSE;
	// ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	// SendInput(1, &ip, sizeof(INPUT));
	// dragging = false;
	// std::cout << "dragStop" << std::endl;
}

void TrackpadManager::move(float deltaX, float deltaY)
{
	// INPUT ip = { 0 };
	// ip.type = INPUT_MOUSE;
	// if (dragging)
	// {
	// 	ip.mi.dwFlags = MOUSEEVENTF_MOVE;
	// 	// std::cout << "drag" << std::endl;
	// }
	// else
	// {
	// 	ip.mi.dwFlags = MOUSEEVENTF_MOVE;
	// 	// std::cout << "move" << std::endl;
	// }
	// ip.mi.dx = deltaX * sensitivity;
	// ip.mi.dy = deltaY * sensitivity;
	// SendInput(1, &ip, sizeof(INPUT));
}