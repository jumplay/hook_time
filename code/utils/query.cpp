#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <new>
#include <stdint.h>
#include "print_log.h"
#include "utils.h"
#include "query.h"

#pragma comment(lib, "Ws2_32.lib")

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

query_t::query_t(const char* addr, uint32_t port)
{
	this->addr = addr;
	this->port = port;
}

query_t::~query_t() {}

bool query_t::query(const char* q, uint32_t q_len, char* recv_buf, uint32_t recv_buf_size)
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		int error_id = WSAGetLastError();
		print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), "socket()");
		return false;
	}

	LINGER lgr = { 1, 0 };
	if (!set_socket_options(sock, 5000, 5000, lgr)) {
		closesocket(sock);
		return false;
	}

	bool rt = true;
	const char* msg = "";
	try {
		sockaddr_in sock_addr;
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = inet_addr(addr);
		sock_addr.sin_port = htons(port);
		if (SOCKET_ERROR == connect(sock, (SOCKADDR*)&sock_addr, sizeof(sock_addr))) {
			msg = "connect";
			throw WSAGetLastError();
		}

		if (SOCKET_ERROR == send(sock, q, q_len, 0)) {
			msg = "send";
			throw WSAGetLastError();
		}

#		if 0
		if (SOCKET_ERROR == shutdown(sock, SD_SEND)) {
			msg = "shutdown send";
			throw WSAGetLastError();
		}
#		endif

		uint32_t recved_len = 0;
		if (recv_buf_size) {
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

#				if 0
				break;
#				endif
			}
			if (recved_len > recv_buf_size) {
				print_error("recv buf overflow");
				rt = false;
			}
		}

#		if 1
		if (SOCKET_ERROR == shutdown(sock, SD_RECEIVE)) {
			msg = "shutdown recv";
			throw WSAGetLastError();
		}
#		endif
	}
	catch (int error_id) {
		print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), msg);
		rt = false;
	}

	if (SOCKET_ERROR == closesocket(sock)) {
		int error_id = WSAGetLastError();
		print_error("(Error: %d)%s %s", error_id, get_error_message(error_id).c_str(), "closesocket");
		rt = false;
	}

	return rt;
}
