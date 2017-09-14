#pragma once
#include "clock.h"

class accept_processor_t
{
	enum
	{
		recv_buf_size = 1 << 10,
	};
	char recv_buf[recv_buf_size];

	bool set_socket_options(SOCKET sock, DWORD send_timeout, DWORD recv_timeout, LINGER lgr)
	{
		do {
			if (SOCKET_ERROR == setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&lgr, sizeof(LINGER))) {
				break;
			}
			if (SOCKET_ERROR == setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&send_timeout, sizeof(DWORD))) {
				break;
			}
			if (SOCKET_ERROR == setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&recv_timeout, sizeof(DWORD))) {
				break;
			}
			return true;
		} while (0);

		int error_id = WSAGetLastError();
		print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), "");
		return false;
	}
	
	const clock_t* p_clock;

public:
	accept_processor_t(clock_t* p_clock)
	{
		this->p_clock = p_clock;
	}

	void accept_processor_t::process(SOCKET sock)
	{
		LINGER lgr = { 0, 0 };
		if (!set_socket_options(sock, 5000, 5000, lgr)) {
			closesocket(sock);
			return;
		}
		
		const char* msg = "";
		try {
			uint32_t recved_len = 0;
			while (1) {
				uint32_t cursor = recved_len % recv_buf_size;
				int rv = recv(sock, recv_buf + cursor, recv_buf_size - cursor, 0);
				if (SOCKET_ERROR == rv) {
					msg = "recv socket_error";
					throw WSAGetLastError();
				}
				if (0 == rv) {
					break;
				}
				recved_len += rv;

#				if 1
				break;	// client hasn't shutdown send
#				endif
			}
			if (recved_len > recv_buf_size) {
				print_error("recv buf overflow");
			}

#			if 0
			if (SOCKET_ERROR == shutdown(sock, SD_RECEIVE)) {
				msg = "shutdown recv";
				throw WSAGetLastError();
			}
#			endif

			uint32_t x = *(uint32_t*)recv_buf;
			if (x != 0xFEDCBA98 || recved_len != 4) {
				print_warning("fail to check receive code");
			}

			SYSTEMTIME cur_time;
			p_clock->get_cur_time(&cur_time);
			if (SOCKET_ERROR == send(sock, (char*)&cur_time, sizeof(cur_time), 0)) {
				msg = "send";
				throw WSAGetLastError();
			}

#			if 0
			if (SOCKET_ERROR == shutdown(sock, SD_SEND)) {
				msg = "shutdown send";
				throw WSAGetLastError();
			}
#			endif
		}
		catch (int error_id) {
			print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), msg);
		}

		if (SOCKET_ERROR == closesocket(sock)) {
			int error_id = WSAGetLastError();
			print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), "closesocket");
		}

		return;
	}

};