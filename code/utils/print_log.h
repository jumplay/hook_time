#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <stdint.h>
#include "utils.h"

#define print_raw(fmt,...) print_t::print('R', fmt "\n", __VA_ARGS__)
#define print_log(fmt,...) print_t::print('L', fmt "\n", __VA_ARGS__)
#define print_cmd(fmt,...) print_t::print('C', fmt "\n", __VA_ARGS__)
#define print_error(fmt,...) print_t::print('E', "error<" __FUNCTION__ ">: " fmt "\n", __VA_ARGS__)
#define print_warning(fmt,...) print_t::print('W', "warning<" __FUNCTION__ ">:" fmt "\n", __VA_ARGS__)

template <uint32_t idx>
void print_tmp(const char* fmt, ...)
{
	static char file_path[MAX_PATH] = "";
	static FILE* fp = NULL;
	if (!fp) {
		_snprintf(file_path, sizeof(file_path), "%s\\print_tmp", get_app_dir());
		create_directory(file_path);
		_snprintf(file_path, sizeof(file_path), "%s\\print_tmp\\tmp_%u.txt", get_app_dir(), idx);
		fp = fopen(file_path, "w");
		if (!fp) {
			print_error("fail to open %s", file_path);
			return;
		}
	}
	va_list vl;
	va_start(vl, fmt);
	vfprintf(fp, fmt, vl);
	fflush(fp);
	va_end(vl);
}

struct print_t
{
	static CRITICAL_SECTION& get_print_cs()
	{
		static CRITICAL_SECTION print_cs;
		static uint32_t init_print_cs = (InitializeCriticalSection(&print_cs), 1);
		return print_cs;
	}

	static FILE* get_log_fp()
	{
		static char file_path[MAX_PATH] = "";
		static const char* log_file_name = "log.txt";
		char* p = NULL;
		if (!GetModuleFileName(0, file_path, sizeof(file_path)) || !(p = strrchr(file_path, '\\'))) {
			printf("error<%s>: fail to GetModuleFileName()\n", __FUNCTION__);
			return NULL;
		}
		*++p = 0;
		if (strlen(file_path) + strlen(log_file_name) >= sizeof(file_path)) {
			printf("error<%s>: file_path buffer overflow\n", __FUNCTION__);
			return NULL;
		}
		strcpy(p, log_file_name);

		static FILE* fp = fopen(file_path, "a+");
		if (!fp) {
			printf("error<%s>: fail to open log file[%s]\n", __FUNCTION__, file_path);
			return NULL;
		}
		return fp;
	}

	static void print(char type, const char* fmt, ...)
	{
		EnterCriticalSection(&get_print_cs());
		static FILE* fp = get_log_fp();
		static char cur_time[CURRENT_TIME_LEN];
		get_current_time(cur_time);

		va_list vl;
		va_start(vl, fmt);
		if (type != 'R') { printf("(%c)%s>", type, cur_time); }
		vprintf(fmt, vl);
		if (fp) {
			if (type != 'R') { fprintf(fp, "(%c)%s>", type, cur_time); }
			vfprintf(fp, fmt, vl);
			fflush(fp);
		}
		va_end(vl);

		LeaveCriticalSection(&get_print_cs());
	}
};
