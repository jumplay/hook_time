#include "stdafx.h"
#include <windows.h>
#include <stdint.h>
#pragma once

class clock_t
{
	LARGE_INTEGER frequ;
	LARGE_INTEGER start_point;
	FILETIME start_time;
	uint32_t scale;

public:
	clock_t() {}

	void init (FILETIME& file_time, uint32_t scale)
	{
		this->scale = scale;
		QueryPerformanceFrequency(&frequ);
		QueryPerformanceCounter(&start_point);
		start_time = file_time;
	}
	
	void get_cur_time(PSYSTEMTIME p_sys_time) const
	{
		LARGE_INTEGER current_point;
		QueryPerformanceCounter(&current_point);
		
		ULONGLONG time_acc = ((LONGLONG)(start_time.dwHighDateTime) << 32) + start_time.dwLowDateTime;
		time_acc += (current_point.QuadPart - start_point.QuadPart) * 10000000 / (frequ.QuadPart) * scale;
		FILETIME current_time;
		current_time.dwHighDateTime = (DWORD)(time_acc >> 32);
		current_time.dwLowDateTime = (DWORD)(time_acc & 0xFFFFFFFF);
		
		FileTimeToSystemTime(&current_time, p_sys_time);
	}
};