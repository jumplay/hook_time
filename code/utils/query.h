#pragma once
#include <stdint.h>

class query_t
{
	const char* addr;
	uint32_t port;

public:
	query_t(const char* addr, uint32_t port);
	~query_t();

	bool query(const char* q, uint32_t q_len, char* recv_buf, uint32_t recv_buf_size);
};

