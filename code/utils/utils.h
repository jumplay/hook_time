#pragma once
#include <windows.h>
#include <string>
#include <stdint.h>

#define CURRENT_TIME_LEN (22)
#define TIMESTAMP_LEN (18)

inline SYSTEM_INFO& get_system_info()
{
	static SYSTEM_INFO system_info;
	static int x_init = 0;
	if (!x_init) {
		GetSystemInfo(&system_info);
		x_init++;
	}
	return system_info;
}

inline void format_path(char* path)
{
	if (!path) { return; }
	char* e = path + strlen(path);
	for (char* b = path; b > e; b++) {
		if (*b == '/') { *b = '\\'; }
	}
	while (e > path && e[-1] == '\\') { *--e = 0; }
}

inline bool file_exist(const char* path)
{
	return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

inline bool file_exist(const DWORD attribute)
{
	return attribute != INVALID_FILE_ATTRIBUTES;
}

inline bool is_directory(const char* path)
{
	DWORD attribute = GetFileAttributes(path);
	return ((attribute != INVALID_FILE_ATTRIBUTES) && (attribute & FILE_ATTRIBUTE_DIRECTORY));
}

inline bool is_directory(const DWORD attribute)
{
	return ((attribute != INVALID_FILE_ATTRIBUTES) && (attribute & FILE_ATTRIBUTE_DIRECTORY));
}

inline uint64_t get_file_size(HANDLE h_file)
{
	DWORD h, l;
	l = GetFileSize(h_file, &h);
	return l + (uint64_t(h) << 32);
}

char* get_current_time(char (&buf)[CURRENT_TIME_LEN]);
char* get_yyyymmddhhmmss(char (&buf)[CURRENT_TIME_LEN]);
const char* get_app_dir();
std::string get_error_message(DWORD error_id);
HANDLE create_file(const char* file_path, DWORD mode = CREATE_ALWAYS);
bool create_directory(const char* path);
char* get_mapped_data(HANDLE h_file_map, uint64_t offset, uint64_t len, LPVOID& base);
bool virtual_lock(char* buf, size_t len);

class cr
{
public:
	static bool set_running()
	{
		char path[MAX_PATH] = "";
		if (_snprintf(path, MAX_PATH - 1, "%s\\running", get_app_dir()) < 0) {
			return false;
		}
		HANDLE h_f = create_file(path, CREATE_NEW);
		if (h_f == INVALID_HANDLE_VALUE) {
			return false;
		}
		CloseHandle(h_f);
		return true;
	}
	
	static bool clean_running()
	{
		char path[MAX_PATH] = "";
		if (_snprintf(path, MAX_PATH - 1, "%s\\running", get_app_dir()) < 0) {
			return false;
		}
		return (0 != DeleteFile(path));
	}
};