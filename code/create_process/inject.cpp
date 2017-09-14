#include "stdafx.h"
#include <windows.h>
#include <tlhelp32.h>
#include "inject.h"
#include "print_log.h"

extern char target_name[MAX_PATH];
extern char dll_path[MAX_PATH];

bool get_target_process_id(DWORD& process_id)
{
	process_id = 0;
	PROCESSENTRY32 process_entry;
	process_entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (snapshot == INVALID_HANDLE_VALUE) { return false; }
	bool r_v = false;

	if (Process32First(snapshot, &process_entry) == TRUE) {
		while (Process32Next(snapshot, &process_entry) == TRUE) {
			if (_stricmp(process_entry.szExeFile, target_name) == 0) {
				process_id = process_entry.th32ProcessID;
				r_v = true;
			}
		}
	}

	CloseHandle(snapshot);
	return r_v;
}

bool get_target_process_handle(HANDLE& h_target)
{
	DWORD process_id;
	if (!get_target_process_id(process_id)) { return false; }

	h_target = OpenProcess(
		PROCESS_QUERY_INFORMATION |
		PROCESS_CREATE_THREAD |
		PROCESS_VM_OPERATION |
		PROCESS_VM_WRITE,
		FALSE, process_id);

	return h_target != NULL;
}

bool write_dll_path(HANDLE h_target, LPVOID& p)
{
	size_t write_len = strlen(dll_path) + 1;
	p = VirtualAllocEx(h_target, NULL, write_len, MEM_COMMIT, PAGE_READWRITE);
	if (p == NULL) { return false; }

	if (!WriteProcessMemory(h_target, p, dll_path, write_len, NULL)) {
		VirtualFreeEx(h_target, p, 0, MEM_RELEASE);
		return false;
	}
	return true;
}

bool inject()
{
	HANDLE h_target = NULL;
	if (!get_target_process_handle(h_target)) {
		print_error("fail to get_target_process_handle()");
		return false;
	}

	LPVOID p = NULL;
	if (!write_dll_path(h_target, p)) {
		print_error("fail to write_dll_path()");
		return false;
	}

	PTHREAD_START_ROUTINE thread_rtn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("Kernel32"), "LoadLibraryA");
	if (thread_rtn == NULL) {
		print_error("fail to GetProcAddress()");
		return false;
	}

	HANDLE h_thread = CreateRemoteThread(h_target, NULL, 0, thread_rtn, (PVOID)p, 0, NULL);
	if (h_thread == NULL) {
		print_error("fail to CreateRemoteThread()");
		return false;
	}

	WaitForSingleObject(h_thread, INFINITE);

	VirtualFreeEx(h_target, p, 0, MEM_RELEASE);
	CloseHandle(h_thread);
	CloseHandle(h_target);

	return true;
}

bool get_module_entry32(MODULEENTRY32& module_entry)
{
	DWORD process_id;
	if (!get_target_process_id(process_id)) { return false; }

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_id);
	if (snapshot == INVALID_HANDLE_VALUE) { return false; }
	
	for (BOOL more_modules = Module32First(snapshot, &module_entry); more_modules; more_modules = Module32Next(snapshot, &module_entry)) {
		if (_stricmp(module_entry.szExePath, dll_path) == 0) {
			CloseHandle(snapshot);
			return true;
		}
	}

	return false;
}

bool eject()
{
	HANDLE h_target = NULL;
	if (!get_target_process_handle(h_target)) {
		print_error("fail to get_target_process_handle()");
		return false;
	}

	MODULEENTRY32 module_entry = { sizeof(module_entry) };
	if (!get_module_entry32(module_entry)) {
		print_error("fail to get_module_entry32");
		return false;
	}

	PTHREAD_START_ROUTINE thread_rtn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("Kernel32"), "FreeLibrary");
	if (thread_rtn == NULL) {
		print_error("fail to GetProcAddress()");
		return false;
	}

	HANDLE h_thread = CreateRemoteThread(h_target, NULL, 0, thread_rtn, module_entry.modBaseAddr, 0, NULL);
	if (h_thread == NULL) {
		print_error("fail to CreateRemoteThread()");
		return false;
	}

	WaitForSingleObject(h_thread, INFINITE);

	CloseHandle(h_thread);
	CloseHandle(h_target);

	return true;
}