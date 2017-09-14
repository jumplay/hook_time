#pragma once

#ifndef HOOK_FUNC
#	define HOOK_FUNC extern "C" __declspec(dllimport)
#endif

HOOK_FUNC void hook_get_local_time(LPSYSTEMTIME p_sys_time);
HOOK_FUNC BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

