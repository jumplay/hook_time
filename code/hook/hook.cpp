#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <ImageHlp.h>
#pragma comment(lib, "ImageHlp")

#define HOOK_FUNC extern "C" __declspec(dllexport)

#include "hook.h"
#include <stdint.h>

#define QUERY_FOR_TIME 0
#if QUERY_FOR_TIME
#	include "query.h"
#else
#	include "clock.h"
#	define TIME_INIT_PATH "e:\\time_init.txt"
#endif

typedef void (WINAPI *pfunc_get_local_time)(SYSTEMTIME*);
typedef void (WINAPI *pfunc_get_system_time)(SYSTEMTIME*);

#if QUERY_FOR_TIME
void hook_get_system_time(LPSYSTEMTIME p_sys_time)
{
	const char* server_addr = "127.0.0.1";
	const uint32_t server_port = 9991;

	query_t qry(server_addr, server_port);
	const uint32_t code = 0xFEDCBA98;
	memset((char*)p_sys_time, 0, sizeof(SYSTEMTIME));
	while (!qry.query((char*)&code, 4, (char*)p_sys_time, sizeof(SYSTEMTIME))) {
		break;
	}
	SYSTEMTIME tmp = *p_sys_time;
	TzSpecificLocalTimeToSystemTime(NULL, &tmp, p_sys_time);
}
#else
void hook_get_system_time(LPSYSTEMTIME p_sys_time)
{
	static LONGLONG file_time_interval = 0;
	static clock_t clock;
	if (file_time_interval == 0) {
		file_time_interval = 1;
		uint32_t scale = 1;
		FILE* fp = fopen("e:\\time_init.txt", "rb");
		if (fp) {
			char buf[10] = "";
			do {
				if (!fgets(buf, 9, fp)) { break; }
				char* b = buf;
				char* p = strchr(b, '-');
				if (!p) { break; }
				*p = 0;
				LONGLONG tmp = LONGLONG(atoi(b)) * 24 * 60 * 60 * 10000000;

				b = p + 1;
				p = strchr(b, '-');
				if (!p) { break; }
				*p = 0;
				tmp += LONGLONG(atoi(b)) * 60 * 60 * 10000000;

				b = p + 1;
				if (!*b || *b == '\r' || *b == '\n') { break; }
				tmp += LONGLONG(atoi(b)) * 60 * 10000000;
				
				if (!fgets(buf, 9, fp)) { break; }
				scale = atoi(buf);
				if (!scale) {
					scale = 1;
					break;
				}
				file_time_interval += tmp;
			} while (0);
		}

		memset(p_sys_time, 0, sizeof(SYSTEMTIME));
		HMODULE h_dll = GetModuleHandle("kernel32");
		PROC hooked_GetSystemTime = GetProcAddress(h_dll, "GetSystemTime");
		((pfunc_get_system_time)(hooked_GetSystemTime))(p_sys_time);

		FILETIME file_time;
		SystemTimeToFileTime(p_sys_time, &file_time);
		LONGLONG tmp = (LONGLONG(file_time.dwHighDateTime) << 32) + file_time.dwLowDateTime;
		tmp -= file_time_interval;
		file_time.dwHighDateTime = tmp >> 32;
		file_time.dwLowDateTime = tmp & 0xFFFFFFFF;

		clock.init(file_time, scale);
	}

	clock.get_cur_time(p_sys_time);
}
#endif

#if QUERY_FOR_TIME
void hook_get_local_time(LPSYSTEMTIME p_sys_time)
{
	const char* server_addr = "127.0.0.1";
	const uint32_t server_port = 9991;

	query_t qry(server_addr, server_port);
	const uint32_t code = 0xFEDCBA98;
	memset((char*)p_sys_time, 0, sizeof(SYSTEMTIME));
	while (!qry.query((char*)&code, 4, (char*)p_sys_time, sizeof(SYSTEMTIME))) {
		break;
	}
}
#else
void hook_get_local_time(LPSYSTEMTIME p_sys_time)
{
	static LONGLONG file_time_interval = 0;
	static clock_t clock;
	if (file_time_interval == 0) {
		file_time_interval = 1;
		uint32_t scale = 1;
		FILE* fp = fopen(TIME_INIT_PATH, "rb");
		if (fp) {
			char buf[10] = "";
			do {
				if (!fgets(buf, 9, fp)) { break; }
				char* b = buf;
				char* p = strchr(b, '-');
				if (!p) { break; }
				*p = 0;
				LONGLONG tmp = LONGLONG(atoi(b)) * 24 * 60 * 60 * 10000000;

				b = p + 1;
				p = strchr(b, '-');
				if (!p) { break; }
				*p = 0;
				tmp += LONGLONG(atoi(b)) * 60 * 60 * 10000000;

				b = p + 1;
				if (!*b || *b == '\r' || *b == '\n') { break; }
				tmp += LONGLONG(atoi(b)) * 60 * 10000000;
				
				if (!fgets(buf, 9, fp)) { break; }
				scale = atoi(buf);
				if (!scale) {
					scale = 1;
					break;
				}
				file_time_interval += tmp;
			} while (0);
		}

		memset(p_sys_time, 0, sizeof(SYSTEMTIME));
		HMODULE h_dll = GetModuleHandle("kernel32");
		PROC hooked_GetLocalTime = GetProcAddress(h_dll, "GetLocalTime");
		((pfunc_get_local_time)(hooked_GetLocalTime))(p_sys_time);

		FILETIME file_time;
		SystemTimeToFileTime(p_sys_time, &file_time);
		LONGLONG tmp = (LONGLONG(file_time.dwHighDateTime) << 32) + file_time.dwLowDateTime;
		tmp -= file_time_interval;
		file_time.dwHighDateTime = tmp >> 32;
		file_time.dwLowDateTime = tmp & 0xFFFFFFFF;

		clock.init(file_time, scale);
	}

	clock.get_cur_time(p_sys_time);
}
#endif

bool implement_hook(const char* func_name, PROC hook_func, int& error_code)
{
	error_code = 0;
	HMODULE h_exe = GetModuleHandle(NULL);
	if (!h_exe) { error_code = 1; return false; }

	HMODULE h_dll = GetModuleHandle("kernel32");
	if (!h_dll) { error_code = 2; return false; }

	PROC hooked_func = GetProcAddress(h_dll, func_name);
	if (!hooked_func) { error_code = 3; return false; }

	PIMAGE_IMPORT_DESCRIPTOR p_import_desc = NULL;
	ULONG ul_size = 0;

	p_import_desc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(h_exe, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ul_size);

	if (p_import_desc == NULL) { error_code = 4; return false; }

	for (; p_import_desc->Name; p_import_desc++) {
		PSTR psz_mod_name = (PSTR)((PBYTE)h_exe + p_import_desc->Name);
		PIMAGE_THUNK_DATA p_thunk = (PIMAGE_THUNK_DATA)((PBYTE)h_exe + p_import_desc->FirstThunk);
		for (; p_thunk->u1.Function; p_thunk++) {
			PROC* ppfn = (PROC*)&p_thunk->u1.Function;
			if (*ppfn == (PROC)hooked_func) {
				PROC func = (PROC)hook_func;
				if (!WriteProcessMemory(GetCurrentProcess(), ppfn, &func, sizeof(func), NULL)) {
					if ((ERROR_NOACCESS == GetLastError())) {
						DWORD dwOldProtect;
						if (VirtualProtect(ppfn, sizeof(func), PAGE_WRITECOPY, &dwOldProtect)) {
							WriteProcessMemory(GetCurrentProcess(), ppfn, &func, sizeof(func), NULL);
							VirtualProtect(ppfn, sizeof(func), dwOldProtect, &dwOldProtect);
							return true;
						}
					}
				}
				else {
					return true;
				}
				error_code = 5;
				return false;
			}
		}
	}
	error_code = 6;
	return false;
}

bool cancel_hook(const char* func_name, PROC hook_func, int& error_code)
{
	error_code = 0;
	HMODULE h_exe = GetModuleHandle(NULL);
	if (!h_exe) { error_code = 1; return false; }

	HMODULE h_dll = GetModuleHandle("kernel32");
	if (!h_dll) { error_code = 2; return false; }

	PROC hooked_func = GetProcAddress(h_dll, func_name);
	if (!hooked_func) { error_code = 3; return false; }

	PIMAGE_IMPORT_DESCRIPTOR p_import_desc = NULL;
	ULONG ul_size = 0;

	p_import_desc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(h_exe, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ul_size);

	if (p_import_desc == NULL) { error_code = 4; return false; }

	for (; p_import_desc->Name; p_import_desc++) {
		PSTR psz_mod_name = (PSTR)((PBYTE)h_exe + p_import_desc->Name);
		PIMAGE_THUNK_DATA p_thunk = (PIMAGE_THUNK_DATA)((PBYTE)h_exe + p_import_desc->FirstThunk);
		for (; p_thunk->u1.Function; p_thunk++) {
			PROC* ppfn = (PROC*)&p_thunk->u1.Function;
			if (*ppfn == (PROC)hook_func) {
				PROC func = (PROC)hooked_func;
				if (!WriteProcessMemory(GetCurrentProcess(), ppfn, &func, sizeof(func), NULL)) {
					if ((ERROR_NOACCESS == GetLastError())) {
						DWORD dwOldProtect;
						if (VirtualProtect(ppfn, sizeof(func), PAGE_WRITECOPY, &dwOldProtect)) {
							WriteProcessMemory(GetCurrentProcess(), ppfn, &func, sizeof(func), NULL);
							VirtualProtect(ppfn, sizeof(func), dwOldProtect, &dwOldProtect);
							return true;
						}
					}
				}
				else {
					return true;
				}
				error_code = 5;
				return false;
			}
		}
	}
	error_code = 6;
	return false;
}

#define PRINT_ERROR 0

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH) {
		//puts("inject");
		int error_code;
		if (!implement_hook("GetLocalTime", (PROC)hook_get_local_time, error_code)) {
#			if PRINT_ERROR
			printf("error: fail to implement hook GetLocalTime, error_code<%u>\n", error_code);
#			endif
		}
		if (!implement_hook("GetSystemTime", (PROC)hook_get_system_time, error_code)) {
#			if PRINT_ERROR
			printf("error: fail to implement hook GetSystemTime, error_code<%u>\n", error_code);
#			endif
		}
	}
	if (fdwReason == DLL_PROCESS_DETACH) {
		//puts("eject");
		int error_code;
		if (!cancel_hook("GetLocalTime", (PROC)hook_get_local_time, error_code)) {
#			if PRINT_ERROR
			printf("error: fail to cancel hook GetLocalTime, error_code<%u>\n", error_code);
#			endif
		}
		if (!cancel_hook("GetSystemTime", (PROC)hook_get_system_time, error_code)) {
#			if PRINT_ERROR
			printf("error: fail to cancel hook GetSystemTime, error_code<%u>\n", error_code);
#			endif
		}
	}
	return TRUE;
}