#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <new>
#include <stdint.h>
#include "print_log.h"
#include "utils.h"
#include "sock_server.h"

#pragma comment(lib, "Ws2_32.lib")

sock_server_t::sock_server_t(const char* addr, uint32_t port, accept_processor_t* accept_processor_list)
{
	this->addr = addr;
	this->port = port;
	this->accept_processor_list = accept_processor_list;
	listen_socket = INVALID_SOCKET;
	accept_socket = INVALID_SOCKET;
	event_accept_socket_set = NULL;
	event_accept_socket_get = NULL;
	accept_run_cnt = 0;
	error_code = no_error;
}
sock_server_t::~sock_server_t() {}

bool sock_server_t::init()
{
	if (error_code != no_error) {
		print_error("unexpectied error");
		return false;
	}

	if (NULL == (event_accept_socket_get = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		DWORD error_id = GetLastError();
		print_error("(Error: %u)%s %s", error_id, get_error_message(error_id).c_str(), "CreateEvent");
		return false;
	}
	if (NULL == (event_accept_socket_set = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		DWORD error_id = GetLastError();
		print_error("(Error: %u)%s %s", error_id, get_error_message(error_id).c_str(), "CreateEvent");
		return false;
	}
	return true;
}

void sock_server_t::uninit()
{
	if (!event_accept_socket_get && !CloseHandle(event_accept_socket_get)) {
		DWORD error_id = GetLastError();
		const char* error_info = "close event_accept_socket_get";
		print_error("(Error: %u)%s %s", error_id, get_error_message(error_id).c_str(), error_info);
		error_code = unexpected_error;
	}
	else {
		event_accept_socket_get = NULL;
	}

	if (!event_accept_socket_set && !CloseHandle(event_accept_socket_set)) {
		DWORD error_id = GetLastError();
		const char* error_info = "close event_accept_socket_set";
		print_error("(Error: %u)%s %s", error_id, get_error_message(error_id).c_str(), error_info);
		error_code = unexpected_error;
	}
	else {
		event_accept_socket_set = NULL;
	}
}

bool sock_server_t::start_listen()
{
	if (error_code != no_error) {
		print_error("unexpectied error");
		return false;
	}

	const char* error_info = "";
	try {

		listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listen_socket == INVALID_SOCKET) {
			error_info = "create listen_socket";
			throw WSAGetLastError();
		}

		sockaddr_in sock_addr;
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = inet_addr(addr);
		sock_addr.sin_port = htons(port);

		if (SOCKET_ERROR == bind(listen_socket, (SOCKADDR*)&sock_addr, sizeof(sock_addr))) {
			error_info = "bind()";
			throw WSAGetLastError();
		}

		if (SOCKET_ERROR == listen(listen_socket, SOMAXCONN)) {
			error_info = "listen()";
			throw WSAGetLastError();
		}
	}
	catch (int error_id) {
		print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), "listen");

		if (listen_socket != INVALID_SOCKET) {
			if (SOCKET_ERROR == closesocket(listen_socket)) {
				error_id = WSAGetLastError();
				print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), "closesocket");
				error_code = unexpected_error;
			}
			else {
				listen_socket = INVALID_SOCKET;
			}
		}
		return false;
	}

	return true;
}

void sock_server_t::end_wait()
{
	if (listen_socket != INVALID_SOCKET && SOCKET_ERROR == closesocket(listen_socket)) {
		DWORD error_id = GetLastError();
		print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), "closesocket()");
		error_code = unexpected_error;
	}
	else {
		listen_socket = INVALID_SOCKET;
	}

	return;
}

void sock_server_t::wait_for_query()
{
	if (error_code != no_error) {
		print_error("unexpectied error");
		return;
	}

	print_log("wait_for_query");

	do {
		accept_socket = accept(listen_socket, NULL, NULL);
		if (!SetEvent(event_accept_socket_set)) {
			DWORD error_id = GetLastError();
			print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), "SetEvent()");
			error_code = unexpected_error;
		}
		if (accept_socket == INVALID_SOCKET && !accept_run_cnt) { break; }
		WaitForSingleObject(event_accept_socket_get, INFINITE);
	} while (accept_socket != INVALID_SOCKET);

	if (listen_socket != INVALID_SOCKET) {
		error_code = unexpected_error;
		print_error("unexpectied exit, listen_socket != INVALID_SOCKET");
	}

	print_log("wait_for_query thread exit");
}

void sock_server_t::process_accept()
{
	if (error_code != no_error) {
		print_error("unexpectied error");
		return;
	}

	uint32_t thread_idx = InterlockedIncrement(&accept_run_cnt) - 1;
	print_log("accept_thread %u run", thread_idx);
	accept_processor_t* p_accept_processor = accept_processor_list + thread_idx;

	while (1) {
		WaitForSingleObject(event_accept_socket_set, INFINITE);
		SOCKET acc_sock = accept_socket;
		if (acc_sock == INVALID_SOCKET) {
			if (0 == InterlockedDecrement(&accept_run_cnt)) {
				if (!SetEvent(event_accept_socket_get)) {
					DWORD error_id = GetLastError();
					print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), "SetEvent()");
					error_code = unexpected_error;
				}
			}
			else {
				if (!SetEvent(event_accept_socket_set)) {
					DWORD error_id = GetLastError();
					print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), "SetEvent()");
					error_code = unexpected_error;
				}
			}
			break;
		}
		SetEvent(event_accept_socket_get);

		p_accept_processor->process(acc_sock);
	}
	
	print_log("accept_thread %u exit", thread_idx);
}

static DWORD WINAPI thread_socket_server_process(LPVOID p)
{
	sock_server_t* p_sock_server = (sock_server_t*)p;
	p_sock_server->process_accept();
	return 0;
}

static DWORD WINAPI thread_socket_server(LPVOID p)
{
	sock_server_t* p_sock_server = (sock_server_t*)p;
	p_sock_server->wait_for_query();
	return 0;
}

s_server_t::s_server_t(const char* addr, uint32_t port, accept_processor_t* accept_processor_list, uint32_t accept_processor_cnt)
	: sock_server(addr, port, accept_processor_list)
{
	process_thread_list = NULL;
	this->process_thread_set_cnt = accept_processor_cnt;
	this->process_thread_run_cnt = 0;
}
s_server_t::~s_server_t()
{
	delete[] process_thread_list;
}

bool s_server_t::init()
{
	try {
		process_thread_list = new HANDLE[process_thread_set_cnt];
		memset(process_thread_list, 0, sizeof(HANDLE) * process_thread_set_cnt);
	}
	catch (std::bad_alloc& b_a) {
		print_error("process_thread_list %s", b_a.what());
		return false;
	}
	return true;
}

bool s_server_t::start_up()
{
	if (!sock_server.init()) {
		print_error("init() failed");
		sock_server.uninit();
		return false;
	}
	if (!sock_server.start_listen()) {
		print_error("start_listen() failed");
		return false;
	}

	if (NULL == (wait_for_query_thread = CreateThread(NULL, 0, thread_socket_server, (LPVOID)&sock_server, 0, 0))) {
		DWORD error_id = GetLastError();
		const char* error_info = "create wait_for_query thread";
		print_error("(Error: %u)%s %s", error_id, get_error_message(error_id).c_str(), error_info);
		return false;
	}
	
	for (; process_thread_run_cnt < process_thread_set_cnt; process_thread_run_cnt++) {
		if (NULL == (process_thread_list[process_thread_run_cnt] = CreateThread(NULL, 0, thread_socket_server_process, (LPVOID)&sock_server, 0, 0))) {
			break;
		}
	}
	if (process_thread_run_cnt != process_thread_set_cnt) {
		print_error("process_thread_run_cnt != process_thread_set_cnt, wait for end_up()");
		end_up();
		return false;
	}
	return true;
}

bool s_server_t::end_up()
{
	print_log("start end_up");
	sock_server.end_wait();
	WaitForSingleObject(wait_for_query_thread, INFINITE);
	if (process_thread_run_cnt) {
		WaitForMultipleObjects(process_thread_run_cnt, process_thread_list, TRUE, INFINITE);
	}
	print_log("end end_up()");
	
	sock_server.uninit();
	return true;
}