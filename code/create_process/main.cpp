#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include "print_log.h"
#include "inject.h"

char target_name[MAX_PATH] = "";
char dll_path[MAX_PATH] = "";

#define OPEN_IN_NEW_WINDOW 1

int create_process(uint32_t argc, char* argv[])
{
	if (argc < 2) { return 3; }

	char command_line[4096] = "";
	for (uint32_t i = 1; i < argc; i++) {
		strcat(command_line, argv[i]);
		strcat(command_line, " ");
	}

	for (char* b = argv[1]; *b; b++) {
		if (*b == '/') { *b = '\\'; }
	}
	char target_path[MAX_PATH];
	char* b = strrchr(argv[1], '\\');
	if (!b) {
		strcpy(target_name, argv[1]);
		sprintf(target_path, "%s\\%s", get_app_dir(), target_name);
	}
	else {
		strcpy(target_name, b + 1);
		strcpy(target_path, argv[1]);
	}
	sprintf(dll_path, "%s\\hook.dll", get_app_dir());

	if (strlen(target_name) < 4 || strcmp(target_name + strlen(target_name) - 4, ".exe")) {
		strcat(target_name, ".exe");
		strcat(target_path, ".exe");
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	GetStartupInfo(&si);
	si.cb = sizeof(si);
	si.lpTitle = target_path;
	memset(&pi, 0, sizeof(pi));

#	if !OPEN_IN_NEW_WINDOW
	DWORD creation_flag = CREATE_SUSPENDED;
#	else
	DWORD creation_flag = CREATE_SUSPENDED | CREATE_NEW_CONSOLE;
#	endif

	// Start the child process. 
	if (!CreateProcess(target_path,   // No module name (use command line)
		command_line,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		creation_flag,              // creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
	) {
		
		DWORD error_id = GetLastError();
		print_error("(Error: %d)%s [%s][%s]", error_id, get_error_message(error_id).c_str(), target_path, target_name);
		return 3;
	}

	inject();

	ResumeThread(pi.hThread);

	int r_v = 0;
#	if !OPEN_IN_NEW_WINDOW
	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, (LPDWORD)&r_v);
#	endif

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return r_v;
}

int main(int argc, char* argv[])
{
	return create_process(argc, argv);
}