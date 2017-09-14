#pragma once
#include <new>
#include "print_log.h"

#define NODE_STATUS_OCCUPIED 1
#define NODE_STATUS_HAS_NEXT_NODE 2
#define NODE_STATUS_USED (NODE_STATUS_OCCUPIED | NODE_STATUS_HAS_NEXT_NODE)
#define NODE_STATUS_BIT_CNT 2

#define IS_OCCUPIED(node) ((node).status & NODE_STATUS_OCCUPIED)
#define IS_USED(node) ((node).status & (NODE_STATUS_USED))
#define HAS_NEXT(node) ((node).status & NODE_STATUS_HAS_NEXT_NODE)
#define GET_NEXT(node) ((node).status >> NODE_STATUS_BIT_CNT)
#define SET_KEY(node,KEY) do { (node).key = KEY; (node).status |= NODE_STATUS_OCCUPIED; } while (0)
#define SET_NEXT(node,pos) do { (node).status = ((pos) << NODE_STATUS_BIT_CNT) | NODE_STATUS_USED; } while (0)
#define SET_NO_NEXT(node) do { (node).status &= NODE_STATUS_OCCUPIED; } while (0)
#define SET_UNOCCUPIED(node) do { (node).status &= (~NODE_STATUS_OCCUPIED); } while (0)
#define SET_UNUSED(node) do { (node).status = 0; } while (0)

#define HASH(key) ((uint32_t)(hash_func(key) & mask))
#define IS_EQUAL(k1,k2) (is_equal_key(k1, k2))

template <typename key_t, typename dest_t, uint32_t mask_wide_orig>
class hash_t
{
	enum cfg
	{
		max_mask_wide = 24,
	};
	static uint32_t verify_mask_wide[mask_wide_orig > max_mask_wide ? -1 : 1];

	typedef uint64_t (*hash_func_t)(const key_t&);
	typedef bool (*is_equal_key_t)(const key_t&, const key_t&);

	struct node_t
	{
		key_t key;
		dest_t dest;
		uint32_t status;	//used by macros
	};

	uint32_t occupied_cnt;
	uint32_t used_cnt;
	uint32_t mask;

	node_t* node_buf;

	hash_func_t hash_func;
	is_equal_key_t is_equal_key;

	__forceinline uint32_t get_unused_pos(uint32_t pos)
	{
		while (IS_USED(node_buf[pos & mask])) { pos++; }
		return pos & mask;
	}

	__forceinline bool check_node_buf_size()
	{
		if (4 * (mask + 1) < 5 * used_cnt) {
			if (2 * used_cnt > 3 * occupied_cnt) {
				print_log("begin rebuild()");
				rebuild();
				return true;
			}
			if ((mask + 1) >> max_mask_wide) {
				if (occupied_cnt != used_cnt) { rebuild(); }
				if (used_cnt >> max_mask_wide) {
					print_error("buf exhausted");
					return false;
				}
				return true;
			}
			if (!expand_node_buf()) {
				print_error("fail to expand_node_buf()");
				return false;
			}
		}
		return true;
	}

	bool expand_node_buf()
	{
		node_t* old_buf = node_buf;
		mask += mask + 1;
		try {
			node_buf = new node_t[mask + 1];
		}
		catch (std::bad_alloc& b_a) {
			print_error("%s", b_a.what());
			return false;
		}
		memcpy(node_buf, old_buf, ((mask + 1) >> 1) * sizeof(node_t));
		memset(node_buf + ((mask + 1) >> 1), 0, ((mask + 1) >> 1) * sizeof(node_t));

		rebuild();
		
		delete[] old_buf;
		return true;
	}

	void rebuild()
	{
		for (uint32_t i = 0; i <= mask; i++) {
			SET_NO_NEXT(node_buf[i]);
		}

		for (uint32_t i = 0; i <= mask; i++) {
			if (!IS_OCCUPIED(node_buf[i])) { continue; }

			uint32_t start_pos = HASH(node_buf[i].key);
			if (start_pos == i) { continue; }

			node_t* p_node = node_buf + i;

			if (!IS_OCCUPIED(node_buf[start_pos])) {
				node_buf[start_pos] = *p_node;
				SET_UNUSED(*p_node);
				continue;
			}

			uint32_t start_start_pos = HASH(node_buf[start_pos].key);
			if (start_start_pos != start_pos) {
				node_t tmp_node = *p_node;
				*p_node = node_buf[start_pos];
				node_buf[start_pos] = tmp_node;

				if (start_pos < i) {
					uint32_t pre_pos = start_start_pos;
					while ((start_start_pos = GET_NEXT(node_buf[start_start_pos])) != start_pos) {
						pre_pos = start_start_pos;
					}
					SET_NEXT(node_buf[pre_pos], i);
				}
				else {
					i--;
				}
				continue;
			}

			SET_NEXT(*p_node, i);
			node_t tmp_node = *p_node;
			*p_node = node_buf[start_start_pos];
			node_buf[start_start_pos] = tmp_node;
		}
		used_cnt = occupied_cnt;
	}

public:
	hash_t(hash_func_t hash_func, is_equal_key_t is_equal_key)
	{
		this->hash_func = hash_func;
		this->is_equal_key = is_equal_key;
		occupied_cnt = 0;
		used_cnt = 0;
		mask = (1 << mask_wide_orig) - 1;
	}

	bool init()
	{
		try {
			node_buf = new node_t[mask + 1];
		}
		catch(std::bad_alloc& b_a) {
			node_buf = NULL;
			print_error("%s", b_a.what());
			return false;
		}
		memset(node_buf, 0, (mask + 1) * sizeof(node_t));
		return true;
	}

	dest_t* insert(const key_t& key)
	{
		if (!check_node_buf_size()) { return NULL; }
		node_t* p_node = node_buf + HASH(key);

		while (1) {
			if (IS_OCCUPIED(*p_node)) {
				if (IS_EQUAL(p_node->key, key)) {
					return &p_node->dest;
				}
			}
			else {
				for (node_t* tmp_node = p_node; HAS_NEXT(*tmp_node); ) {
					tmp_node = node_buf + GET_NEXT(*tmp_node);
					if (IS_OCCUPIED(*tmp_node) && IS_EQUAL(tmp_node->key, key)) {
						return &tmp_node->dest;
					}
				}
				used_cnt += IS_USED(*p_node) ? 0 : 1;
				occupied_cnt++;
				SET_KEY(*p_node, key);
				return &p_node->dest;
			}

			if (HAS_NEXT(*p_node)) {
				p_node = node_buf + GET_NEXT(*p_node);
				continue;
			}

			uint32_t pos = get_unused_pos(uint32_t(p_node - node_buf));
			used_cnt++;
			occupied_cnt++;
			SET_KEY(node_buf[pos], key);
			SET_NEXT(*p_node, pos);
			return &node_buf[pos].dest;
		}
	}

	dest_t* insert(const key_t& key, bool& exist)
	{
		if (!check_node_buf_size()) { return NULL; }
		node_t* p_node = node_buf + HASH(key);
		exist = false;

		while (1) {
			if (IS_OCCUPIED(*p_node)) {
				if (IS_EQUAL(p_node->key, key)) {
					exist = true;
					return &p_node->dest;
				}
			}
			else {
				for (node_t* tmp_node = p_node; HAS_NEXT(*tmp_node); ) {
					tmp_node = node_buf + GET_NEXT(*tmp_node);
					if (IS_OCCUPIED(*tmp_node) && IS_EQUAL(tmp_node->key, key)) {
						exist = true;
						return &tmp_node->dest;
					}
				}
				used_cnt += IS_USED(*p_node) ? 0 : 1;
				occupied_cnt++;
				SET_KEY(*p_node, key);
				return &p_node->dest;
			}

			if (HAS_NEXT(*p_node)) {
				p_node = node_buf + GET_NEXT(*p_node);
				continue;
			}

			uint32_t pos = get_unused_pos(uint32_t(p_node - node_buf));
			used_cnt++;
			occupied_cnt++;
			SET_KEY(node_buf[pos], key);
			SET_NEXT(*p_node, pos);
			return &node_buf[pos].dest;
		}
	}

	dest_t* erase(const key_t& key)
	{
		node_t* p_node = node_buf + HASH(key);

		while (1) {
			if (IS_OCCUPIED(*p_node) && IS_EQUAL(p_node->key, key)) {
				SET_UNOCCUPIED(*p_node);
				used_cnt -= IS_USED(*p_node) ? 0 : 1;
				occupied_cnt--;
				return &p_node->dest;
			}

			if (HAS_NEXT(*p_node)) {
				p_node = node_buf + GET_NEXT(*p_node);
				continue;
			}
			return NULL;
		}
	}

	dest_t* find(const key_t& key)
	{
		node_t* p_node = node_buf + HASH(key);
		while (1) {
			if (IS_OCCUPIED(*p_node) && IS_EQUAL(p_node->key, key)) {
				return &p_node->dest;
			}
			if (HAS_NEXT(*p_node)) {
				p_node = node_buf + GET_NEXT(*p_node);
			}
			else {
				return NULL;
			}
		}

		return NULL;
	}

	void build() { rebuild(); }

	void clean()
	{
		for (uint32_t i = 0; i <= mask; i++) {
			SET_UNUSED(node_buf[i]);
		}
		occupied_cnt = 0;
		used_cnt = 0;
	}

	bool add(const key_t& key, const dest_t& dest)
	{
		if (occupied_cnt == mask + 1) {
			if ((mask + 1) >> max_mask_wide) {
				print_error("buf exhausted");
				return false;
			}
			node_t* old_buf = node_buf;
			mask += mask + 1;
			try {
				node_buf = new node_t[mask + 1];
			}
			catch (std::bad_alloc& b_a) {
				print_error("%s", b_a.what());
				return false;
			}
			memcpy(node_buf, old_buf, ((mask + 1) >> 1) * sizeof(node_t));
			memset(node_buf + ((mask + 1) >> 1), 0, ((mask + 1) >> 1) * sizeof(node_t));
		}

		node_t* p_node = node_buf + occupied_cnt++;
		SET_KEY(*p_node, key);
		p_node->dest = dest;
		return true;
	}

	dest_t* enumerate(uint32_t& pos, const key_t*& p_key)
	{
		if (pos > mask) { return NULL; }
		while (pos <= mask && !IS_OCCUPIED(node_buf[pos])) { pos++; }
		if (pos > mask) { return NULL; }
		p_key = &node_buf[pos].key;
		return &node_buf[pos++].dest;
	}

	uint32_t get_cnt() const { return occupied_cnt; }

	void check() const
	{
		uint32_t u_calc = 0;
		uint32_t o_calc = 0;
		for (uint32_t i = 0; i <= mask; i++) {
			if (!IS_USED(node_buf[i])) { continue; }
			u_calc++;
			o_calc += IS_OCCUPIED(node_buf[i]) ? 1 : 0;
		}
		if (u_calc != used_cnt) {
			print_error("u_calc(%u) != used_cnt(%u)", u_calc, used_cnt);
		}
		if (o_calc != occupied_cnt) {
			print_error("o_calc(%u) != occupied_cnt(%u)", o_calc, occupied_cnt);
		}
	}

	void print_info() const
	{
		uint32_t mask_wide = 1;
		while (mask & (1 << mask_wide)) { mask_wide++; }
		
		print_log("hash::print_info");
		print_log("\tmask_wide:    %u", mask_wide);
		print_log("\toccupied_cnt: %u", occupied_cnt);
		print_log("\tused_cnt      %u", used_cnt);
		print_log("\toccupy ratio£º%.2f", double(occupied_cnt * 100) / (mask + 1));

		uint32_t max_chain_len = 0;
		uint32_t cumulate = 0;
		uint32_t start_cnt = 0;
		for (uint32_t i = 0; i <= mask; i++) {
			if (!IS_USED(node_buf[i]) || HASH(node_buf[i].key) != i) { continue; }
			start_cnt++;
			node_t* p_node = node_buf + i;
			uint32_t chain_len = 1;
			while (HAS_NEXT(*p_node)) {
				chain_len++;
				p_node = node_buf + GET_NEXT(*p_node);
			}
			max_chain_len = max_chain_len > chain_len ? max_chain_len : chain_len;
			cumulate += chain_len;
		}

		print_log("\tmax_chain_len: %u, everage_len: %.2f", max_chain_len, double(cumulate) / start_cnt);
	}
};

#if 0
#include <stdlib.h>
#include <time.h>
#include "time_count.h"
namespace test_hash
{
	uint64_t hash_func(uint64_t x) { return x; }
	bool is_equal_key(uint64_t a, uint64_t b) { return a == b; }

	struct elem_t
	{
		uint64_t key;
		uint32_t dest;
		bool inserted;
	};
	const uint64_t elem_cnt = 1 << 20;
	static elem_t p_elem[elem_cnt];

	void set_elems()
	{
		for (uint64_t i = 0; i < elem_cnt; i++) {
			p_elem[i].key = (i << 24) | (((rand() & ((1 << 12) - 1)) << 12) | (rand() & ((1 << 12) - 1)));
			p_elem[i].dest = (uint32_t)i;
			p_elem[i].inserted = false;
		}
	}

	uint32_t get_inserted_cnt()
	{
		uint32_t cnt = 0;
		for (uint32_t i = 0; i < elem_cnt; i++) { cnt += p_elem[i].inserted ? 1 : 0; }
		return cnt;
	}

	void test_hash()
	{
		time_count tc;
		print_log("begin test_hash()");

#		if 0
		srand(17);
#		else
		srand((uint32_t)time(NULL));
#		endif

		set_elems();
		hash_t<uint64_t, uint32_t, 2> hash(hash_func, is_equal_key);
		hash.init();

#		define MODE 1
#		if MODE == 1
		tc.start();
		for (uint32_t i = 0; i < elem_cnt; i++) {
			uint32_t* p_dest = hash.insert(p_elem[i].key);
			if (!p_dest) { break; }
			*p_dest = p_elem[i].dest;
			p_elem[i].inserted = true;
		}
		tc.end("");
#		elif MODE == 2
		hash.clean();
		for (uint32_t i = 0; i < elem_cnt; i++) {
			if(!hash.add(p_elem[i].key, p_elem[i].dest)) {
				break;
			}
			p_elem[i].inserted = true;
		}
		tc.start();
		hash.build();
		tc.end("");
#		elif MODE == 3
		tc.start();
		for (uint32_t I = 0; I < 10; I++) {
			for (uint32_t i = 0; i < elem_cnt; i++) {
				bool do_insert = rand() & 1;
				if (do_insert) {
					*hash.insert(p_elem[i].key) = p_elem[i].dest;
				}
				else {
					hash.erase(p_elem[i].key);
				}
				p_elem[i].inserted = do_insert;
			}
			printf(">%u\n", I);
		}
		printf("inserted_cnt: %u\n", get_inserted_cnt());
		tc.end("");
#		endif

		hash.check();
		hash.print_info();

		hash.build();
		hash.check();
		hash.print_info();


		for (uint32_t i = 0; i < elem_cnt; i++) {
			uint32_t* p_dest = hash.find(p_elem[i].key);
			if ((p_elem[i].inserted && !p_dest) || (!p_elem[i].inserted && p_dest)) {
				printf("error: fail to find\n");
			}
		}

		print_log("end");
	}

	static int xx_test_hash = (test_hash(), exit(1), 1);
}
#endif