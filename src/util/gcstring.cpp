#include <mutex>
#include "gcstring.h"
#pragma no_auto_header()

namespace ama {
	extern int8_t enable_threading;
}

static ama::gcstring_long* g_null_hash = nullptr;
static ama::gcstring_long** g_hash = &g_null_hash;
static size_t g_hash_size = 1;
static size_t g_n_elements = 0;

//use the same hash as QuickJS
static inline uint64_t hash_string8(uint8_t const* str, size_t len, uint64_t h) {
	for (size_t i = 0; i < len; i++) {
		h = h * 263 + str[i];
	}
	return h;
}

static std::mutex g_gcstring_mutex{};
ama::gcstring_long * ama::toGCStringLongCat(char const* data0, size_t length0, char const* data1, size_t length1) {
	//printf("%p %d %p %d\n",data0,int(length0),data1,int(length1));
	size_t length = length0 + length1;
	assert(length >= 8);
	uint64_t h = hash_string8((uint8_t const*)data0, length0, 0uLL);
	h = hash_string8((uint8_t const*)data1, length1, h);
	//the single instruction loading the pointer is slower than std::string construction
	//it's faster without the hash test
	//p->hash == h && 
	if (ama::enable_threading) {g_gcstring_mutex.lock();}
	for (ama::gcstring_long* p = g_hash[h & (g_hash_size - 1)]; p; p = p->next) {
		if (p->length == length && memcmp(p->data(), data0, length0) == 0 && memcmp(p->data() + length0, data1, length1) == 0) {
			if (ama::enable_threading) {g_gcstring_mutex.unlock();}
			return p;
		}
	}
	if (g_n_elements >= g_hash_size) {
		//resize
		uint64_t old_size = g_hash_size;
		ama::gcstring_long** old_hash = g_hash;
		uint64_t new_size = old_size * 2;
		if (new_size < 512) {new_size = 512;}
		assert(!(new_size & (new_size - 1)));
		ama::gcstring_long** new_hash = (ama::gcstring_long**)calloc(sizeof(ama::gcstring_long*), new_size);
		if (new_hash) {
			for (uint64_t i = 0; i < old_size; i++) {
				ama::gcstring_long* p = old_hash[i];
				while (p) {
					ama::gcstring_long* p_next = p->next;
					uint64_t hp = p->hash & (new_size - 1);
					p->next = new_hash[hp];
					new_hash[hp] = p;
					p = p_next;
				}
			}
			if (old_size > 1) {
				free(old_hash);
				old_hash = nullptr;
			}
			g_hash = new_hash;
			g_hash_size = new_size;
		} else {
			//just give up, do nothing
		}
	}
	g_n_elements += 1;
	uint64_t hp = h & (g_hash_size - 1);
	ama::gcstring_long* p_new = (ama::gcstring_long*)malloc(sizeof(ama::gcstring_long) + length + 1);
	p_new->length = length;
	p_new->next = g_hash[hp];
	p_new->hash = h;
	g_hash[hp] = p_new;
	memcpy(p_new->data(), data0, length0);
	memcpy(p_new->data() + length0, data1, length1);
	p_new->data()[length] = 0;
	//fprintf(stderr,"alloc string %p %s\n",p_new,p_new->data());
	if (ama::enable_threading) {g_gcstring_mutex.unlock();}
	return p_new;
}

void ama::gcstring_gcsweep() {
	uint64_t old_size = g_hash_size;
	ama::gcstring_long** old_hash = g_hash;
	for (uint64_t i = 0; i < old_size; i++) {
		ama::gcstring_long** pp_last = &old_hash[i];
		ama::gcstring_long* p = old_hash[i];
		while (p) {
			ama::gcstring_long* p_next = p->next;
			if (p->length & (1uLL << 63)) {
				//keep it
				p->length &= ~(1uLL << 63);
				*pp_last = p;
				pp_last = &p->next;
			} else {
				//drop it
				//fprintf(stderr,"dropped string %p %s\n",p,p->data());
				g_n_elements -= 1;
				free(p);
			}
			p = p_next;
		}
		*pp_last = nullptr;
	}
}

ama::gcstring ama::gcscat(std::span<char> a, std::span<char> b) {
	size_t length = a.size() + b.size();
	if (length <= 7) {
		union{
			uint64_t v{};
			char s[8];
		}u{};
		u.v = 0;
		memcpy(u.s, a.data(), a.size());
		memcpy(u.s + a.size(), b.data(), b.size());
		return ama::gcstring(u.v, (ama::raw_construction*)nullptr);
	} else {
		return ama::gcstring(ama::toGCStringLongCat(a.data(), a.size(), b.data(), b.size()), (ama::raw_construction*)nullptr);
	}
}
