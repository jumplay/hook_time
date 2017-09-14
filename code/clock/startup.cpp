#include "stdafx.h"
#include "clock.h"
#include "sock_server.h"
#include "string.h"
#include <stdint.h>
#include "print_log.h"

const char* server_addr = "127.0.0.1";
const uint32_t server_port = 9991;

bool parse_int(char*& t, uint32_t* v, uint32_t min, uint32_t max, const char sep, const uint32_t len)
{
	if (t[len] != sep) { return false; }
	t[len] = 0;
	*v = 0;
	while (*t) {
		if (!(*t >= '0' && *t <= '9')) { return false; }
		*v = *v * 10 + *t++ - '0';
	}
	if (!(*v <= max && *v >= min)) { return false; }
	t++;
	return true;
}

bool parse_time(const char* time_str, PSYSTEMTIME time)
{
	if (strlen(time_str) != 19) {
		print_error("strlen(time_str) != 19");
		return false;
	}
	char buf[20] = "";
	strcpy(buf, time_str);
	char* t = buf;

	if (!parse_int(t, (uint32_t*)&time->wYear, 2000, 2050, '/', 4)) { return false; }
	if (!parse_int(t, (uint32_t*)&time->wMonth, 1, 12, '/', 2)) { return false; }
	if (!parse_int(t, (uint32_t*)&time->wDay, 1, 31, '-', 2)) { return false; }
	if (!parse_int(t, (uint32_t*)&time->wHour, 0, 23, ':', 2)) { return false; }
	if (!parse_int(t, (uint32_t*)&time->wMinute, 0, 59, ':', 2)) { return false; }
	if (!parse_int(t, (uint32_t*)&time->wSecond, 0, 59, 0, 2)) { return false; }
	return true;
}

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


bool startup(const char* time_str, uint32_t scale)
{
	SYSTEMTIME start_time;
	if (!parse_time(time_str, &start_time)) {
		print_error("fail to parse_time()");
		return false;
	}

	clock_t clock(&start_time, scale);

	accept_processor_t accept_processor(&clock);
	s_server_t s_server(server_addr, server_port, &accept_processor, 1);
	if (!s_server.init()) { return false; }
	if (!s_server.start_up()) { return false; }
	Sleep(INFINITE);
	
	return true;
}