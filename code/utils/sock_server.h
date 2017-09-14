#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdint.h>
#include "print_log.h"
#include "utils.h"
#include "accept_processor.h"

class sock_server_t
{
	enum {
		no_error,
		unexpected_error,
	} error_code;

	const char* addr;
	uint32_t port;
	LONG accept_run_cnt;
	SOCKET listen_socket;
	SOCKET accept_socket;
	HANDLE event_accept_socket_set;
	HANDLE event_accept_socket_get;
	accept_processor_t* accept_processor_list;

public:
	void wait_for_query();
	bool init();
	void process_accept();
	void uninit();
	bool start_listen();
	void end_wait();

	sock_server_t(const char* addr, uint32_t port, accept_processor_t* accept_processor_list);
	~sock_server_t();
};

class s_server_t
{
	sock_server_t sock_server;
	uint32_t process_thread_set_cnt;
	uint32_t process_thread_run_cnt;
	HANDLE* process_thread_list;
	HANDLE wait_for_query_thread;

public:
	bool init();
	bool start_up();
	bool end_up();

	s_server_t(const char* addr, uint32_t port, accept_processor_t* accept_processor_list, uint32_t accept_processor_cnt);
	~s_server_t();
};