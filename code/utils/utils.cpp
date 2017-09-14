#include "stdafx.h"
#include <stdio.h>
#include <string>
#include <windows.h>
#include "utils.h"

std::string get_error_message(DWORD error_id)
{
	HLOCAL hlocal = NULL;
	size_t size = FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_MAX_WIDTH_MASK,
		NULL, error_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (PTSTR)&hlocal, 0, NULL);
	
	std::string error_message = size ? std::string((PTSTR)hlocal, size) : "No corresponding error message";
	if (hlocal) { LocalFree(hlocal); }

	return error_message;
}

char* get_current_time(char (&buf)[CURRENT_TIME_LEN])
{
	SYSTEMTIME t;
	GetLocalTime(&t);
	sprintf(buf, "%04d%02d%02d %02d:%02d:%02d.%03d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
	return buf;
}
char* get_yyyymmddhhmmss(char (&buf)[CURRENT_TIME_LEN])
{
	SYSTEMTIME t;
	GetLocalTime(&t);
	sprintf(buf, "%04d%02d%02d%02d%02d%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
	return buf;
}

const char* get_app_dir()
{
	static char path[MAX_PATH] = "";
	if (path[0]) {
		return path;
	}
	
	GetModuleFileName(NULL, path, sizeof(path));
	strrchr(path, '\\')[0] = 0;
	return path;
}

HANDLE create_file(const char* file_path, DWORD mode)
{
	HANDLE hFile = CreateFile(file_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, mode, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		return hFile;
	}
	if (GetLastError() == ERROR_PATH_NOT_FOUND) {
		const char* p = strrchr(file_path, '\\');
		if (!p) { return INVALID_HANDLE_VALUE; }
		char dir_path[MAX_PATH] = "";
		strncpy(dir_path, file_path, p - file_path);
		create_directory(dir_path);
		hFile = CreateFile(file_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		return hFile;
	}
	return INVALID_HANDLE_VALUE;
}

bool create_directory(const char* path)
{
	if (is_directory(path)) { return true; }

	char dir_path[MAX_PATH] = "";
	strcpy(dir_path, path);

	char* t = strrchr(dir_path, '\\');
	while (t) {
		*t = 0;
		DWORD attribute = GetFileAttributes(dir_path);
		if (file_exist(attribute)) {
			if (is_directory(attribute)) { break; }
			return false;
		}
		char* x = t;
		t = strrchr(dir_path, '\\');
		*x = '\\';
	}

	while (t) {
		*t++ = '\\';
		char* x = strchr(t, '\\');
		if (x) { *x = 0; }
		CreateDirectory(dir_path, NULL);
		t = x;
	}

	return is_directory(dir_path);
}

char* get_mapped_data(HANDLE h_file_map, uint64_t offset, uint64_t len, LPVOID& base)
{
	uint64_t shift = offset % get_system_info().dwAllocationGranularity;
	offset -= shift;
	len += shift;
	base = MapViewOfFile(h_file_map, FILE_MAP_ALL_ACCESS, offset>>32, uint32_t(offset), size_t(len));
	if (!base) { return NULL; }
	return ((char*)(base)) + shift;
}

bool virtual_lock(char* buf, size_t len)
{
	if (!SetProcessWorkingSetSize(GetCurrentProcess(), len + (1<<24), len + (1<<24)) ||
		!VirtualLock(buf, len))
	{
		return false;
	}

	return true;
}