#include "stdafx.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdint.h>
#include "utils.h"
#include "print_log.h"
#pragma comment(lib, "Ws2_32.lib")

void print_time(const SYSTEMTIME& sys_time)
{
	printf("%u/%u/%u %u:%u:%u.%u\n",
		sys_time.wYear,
		sys_time.wMonth,
		sys_time.wDay,
		sys_time.wHour,
		sys_time.wMinute,
		sys_time.wSecond,
		sys_time.wMilliseconds
	);
}

int main()
{
	WSADATA wsa_data;
	uint32_t error_id;
	if (error_id = WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
		print_error("(Error: %u)%s %s", error_id, get_error_message(error_id).c_str(), "WSAStartup");
		return 1;
	}

	SYSTEMTIME sys_time;
	while (1) {
		GetLocalTime(&sys_time);
		print_time(sys_time);
		GetSystemTime(&sys_time);
		print_time(sys_time);
		Sleep(1000);
	}

	if (SOCKET_ERROR == WSACleanup()) {
		error_id = WSAGetLastError();
		print_error("(Error: %u)%s %s", error_id, get_error_message(error_id).c_str(), "WSACleanup");
		return 1;
	}

    return 0;
}

