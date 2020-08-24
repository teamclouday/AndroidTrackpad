#pragma once

#include <cstring>
#include <deque>
#include <mutex>

// define the data seperator
#define DATA_VALIDATION_CODE 10086

class TrackpadManager
{
public:
	enum DATA_TYPE
	{
		DATA_TYPE_CLICK_LEFT	= 0,
		DATA_TYPE_CLICK_RIGHT	= 1,
		DATA_TYPE_SCROLL_HORI	= 2,
		DATA_TYPE_SCROLL_VERT	= 3,
		DATA_TYPE_DRAG			= 4,
		DATA_TYPE_MOVE			= 5,
	};

	struct DATA_PACK
	{
		// assume raw_data has 4*4 bytes
		// alignment is:
		// validation_code	4 bytes
		// type				4 bytes
		// velX				4 bytes
		// velY				4 bytes

		int type;
		float velX;
		float velY;

		// reference: https://stackoverflow.com/questions/1001307/detecting-endianness-programmatically-in-a-c-program
		static bool is_little_endian()
		{
			union
			{
				uint32_t i;
				char c[4];
			} u = { 0x01020304 };
			return u.c[0] == 0x04;
		}

		static int reverse_bytes_int(uint32_t num)
		{
			unsigned char bytes[4];
			bytes[0] = (num >> 24) & 0xFF;
			bytes[1] = (num >> 16) & 0xFF;
			bytes[2] = (num >> 8) & 0xFF;
			bytes[3] = (num) & 0xFF;
			int newNum;
			std::memcpy(&newNum, bytes, sizeof(int));
			return newNum;
		}

		static float reverse_bytes_float(uint32_t num)
		{
			unsigned char bytes[4];
			bytes[0] = (num >> 24) & 0xFF;
			bytes[1] = (num >> 16) & 0xFF;
			bytes[2] = (num >> 8) & 0xFF;
			bytes[3] = (num) & 0xFF;
			float newNum;
			std::memcpy(&newNum, bytes, sizeof(float));
			return newNum;
		}
	};

	TrackpadManager();
	~TrackpadManager();

	void process();
	void addData(DATA_PACK newData);
private:
	void mouseLeftClick(float flag);
	void mouseRightClick();
	void scrollHorizontal(float delta);
	void scrollVertical(float delta);
	void dragStart();
	void dragStop();
	void move(float deltaX, float deltaY);

public:
	float sensitivity = 1.0f;
	std::mutex lock;
private:
	const int DATA_PACK_MAX = 32;
	std::deque<DATA_PACK> buffer;
	bool dragging = false; // inidicator for dragging
};