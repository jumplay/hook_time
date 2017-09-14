#include "stdafx.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>
#include "startup.h"
#include "print_log.h"

bool kill_others()
{
	const char* target_name = "clock.exe";
	DWORD process_id = GetCurrentProcessId();
	PROCESSENTRY32 process_entry;
	process_entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (snapshot == INVALID_HANDLE_VALUE) { return false; }
	bool r_v = true;

	if (Process32First(snapshot, &process_entry) == TRUE) {
		while (Process32Next(snapshot, &process_entry) == TRUE) {
			if (_stricmp(process_entry.szExeFile, target_name) == 0 && process_id != process_entry.th32ProcessID) {
				HANDLE h_process = OpenProcess(PROCESS_TERMINATE, 0, process_entry.th32ProcessID);
				if (h_process != NULL) {
					TerminateProcess(h_process, 9);
					CloseHandle(h_process);
				}
				else {
					r_v = false;
					break;
				}
			}
		}
	}

	CloseHandle(snapshot);
	return r_v;
}

int main(int argc, const char* argv[])
{
	if (!kill_others()) {
		print_error("fail to kill_others()");
		getchar();
		return 1;
	}

	WSADATA wsa_data;
	uint32_t error_id;
	if (error_id = WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
		print_error("(Error: %u)%s %s", error_id, get_error_message(error_id).c_str(), "WSAStartup");
		getchar();
		return 1;
	}


	if (argc != 3) { return 1; }
	int r = startup(argv[1], atoi(argv[2])) ? 0 : 1;

	if (SOCKET_ERROR == WSACleanup()) {
		error_id = WSAGetLastError();
		print_error("(Error: %u)%s %s", error_id, get_error_message(error_id).c_str(), "WSACleanup");
		getchar();
		return 1;
	}

	return r;
}