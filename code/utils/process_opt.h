#pragma once
#include <string.h>
#include "hash.h"

template <uint32_t opt_size>
class com_opt_t
{
	enum cfg
	{
		value_size = 32,
	};

	enum enum_error
	{
		no_error,
		bad_alloc,
		opt_overflow,
		value_overflow,
	};

	struct opt_t
	{
		char type;
		char value[value_size + 1];
		const char* default_value;
		bool set;
		const char* name;
		uint32_t name_len;
		
		opt_t() { default_value = ""; }
	};
	opt_t *p_opt;

	uint32_t opt_num;

	enum_error error_code;

public:
	com_opt_t()
	{
		error_code = no_error;
		opt_num = 0;
	}

	bool init()
	{
		try {
			p_opt = new opt_t[opt_size];
		}
		catch (...) {
			error_code = bad_alloc;
			return false;
		}
		return true;
	}

	bool add_opt(const char* str, char type = 'u', const char* default_value = "")
	{
		if (opt_num == opt_size) {
			error_code = opt_overflow;
			return false;
		}
		p_opt[opt_num].name = str;
		p_opt[opt_num].name_len = (uint32_t)strlen(str);
		p_opt[opt_num].type = type;
		p_opt[opt_num].default_value = default_value;
		opt_num++;

		return true;
	}

	void end_add_opt()
	{
		opt_t tmp;
		for (uint32_t i = 0; i < opt_num; i++) {
			uint32_t bub = i;
			for (uint32_t j = i + 1; j < opt_num; j++) {
				if (p_opt[j].name_len > p_opt[bub].name_len) { bub = j; }
			}
			if (bub != i) {
				tmp = p_opt[bub];
				p_opt[bub] = p_opt[i];
				p_opt[i] = tmp;
			}
		}
	}

	void parse(const char* str)
	{
		for (uint32_t i = 0; i < opt_num; i++) {
			p_opt[i].set = false;
		}

		if (!str[2] && *(uint16_t*)str == *(uint16_t*)"-h") {
			for (uint32_t i = 0; i < opt_num; i++) {
				printf("    %s\n", p_opt[i].name);
			}
			puts("");
			return;
		}

		if (!str) { return; }

		const char* b = str;
		const char* e = str + strlen(str);

		while (b < e) {
			while (*b == ' ' && b < e) { b++; }
			if (b >= e) { return; }

			for (uint32_t i = 0; i < opt_num; i++) {
				if (uint32_t(e - b) < p_opt[i].name_len) { continue; }
				if (!strncmp(b, p_opt[i].name, p_opt[i].name_len)) {
					p_opt[i].set = true;
					b += p_opt[i].name_len;
					uint32_t cpy_len = uint32_t(e - b) < value_size ? uint32_t(e - b) : value_size;
					strncpy(p_opt[i].value, b, cpy_len)[cpy_len] = 0;
					for (uint32_t j = 0; j < value_size && p_opt[i].value[j]; j++) {
						if (p_opt[i].value[j] == ' ') {
							p_opt[i].value[j] = 0;
							break;
						}
					}
					break;
				}
			}
			while (b < e && *b != ' ') { b++; }
			if (b >= e) { return; }
			b++;
		}
	}

	bool parse(uint32_t argc, const char* argv[])
	{
		if (!argc) { return true; }
		for (uint32_t i = 0; i < opt_num; i++) {
			p_opt[i].set = false;
		}

		if (!strcmp(argv[0], "--help") || !strcmp(argv[0], "-h")) {
			for (uint32_t i = 0; i < opt_num; i++) {
				printf("    %s\n", p_opt[i].name);
			}
			puts("");
			return false;
		}

		for (uint32_t i = 0; i < argc; i++) {
			for (uint32_t j = 0; j < opt_num; j++) {
				if (!strncmp(argv[i], p_opt[j].name, p_opt[j].name_len)) {
					p_opt[j].set = true;
					const char* v = argv[i] + p_opt[j].name_len;
					uint32_t cpy_len = (uint32_t)strlen(v);
					cpy_len = cpy_len < value_size ? cpy_len : value_size;
					strncpy(p_opt[j].value, v, cpy_len)[cpy_len] = 0;
				}
			}
		}
		return true;
	}

	const char* get_value(const char* str)
	{
		uint32_t len = (uint32_t)strlen(str);
		for (uint32_t i = 0; i < opt_num; i++) {
			if (len > p_opt[i].name_len) { break; }
			if (len == p_opt[i].name_len && !strcmp(str, p_opt[i].name)) {
				if (p_opt[i].set) {
					return p_opt[i].value;
				}
				else {
					return p_opt[i].default_value;
				}
			}
		}

		return "";
	}

	bool is_set(const char* str)
	{
		uint32_t len = (uint32_t)strlen(str);
		for (uint32_t i = 0; i < opt_num; i++) {
			if (len > p_opt[i].name_len) { break; }
			if (len == p_opt[i].name_len && !strcmp(str, p_opt[i].name)) {
				return p_opt[i].set;
			}
		}

		return false;
	}
};