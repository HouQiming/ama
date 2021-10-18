#include "jc_unique_string.h"
#include <unordered_set>
#include <stdlib.h>
#include <memory.h>

namespace JC{

struct string_interned_hash{
	std::size_t operator()(const string_interned* a) const noexcept{
	    return a->m_hash;
	}
};

//note: .data() could be invalid here, must use m_data
struct string_interned_equal{
    int operator()(const string_interned* a,const string_interned* b) const noexcept{
        return a->m_size==b->m_size&&memcmp(a->m_data,b->m_data,b->m_size)==0;
    }
};

struct CTableSingleton{
	static std::unordered_set<string_interned*,string_interned_hash,string_interned_equal>* g_table;
	static std::unordered_set<string_interned*,string_interned_hash,string_interned_equal>* get(){
		if(!g_table){
			g_table=new std::unordered_set<string_interned*,string_interned_hash,string_interned_equal>();
		}
		return g_table;
	}
};
std::unordered_set<string_interned*,string_interned_hash,string_interned_equal>* CTableSingleton::g_table=NULL;

#if __cplusplus<201703L
// https://github.com/Cyan4973/xxHash/
// hacked a bit to adapt to our build environment
// removed big endian support, templatified aligned-ness
/*
*  xxHash - Fast Hash algorithm
*  Copyright (C) 2012-2016, Yann Collet
*
*  BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are
*  met:
*
*  * Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above
*  copyright notice, this list of conditions and the following disclaimer
*  in the documentation and/or other materials provided with the
*  distribution.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*  You can contact the author at :
*  - xxHash homepage: http://www.xxhash.com
*  - xxHash source repository : https://github.com/Cyan4973/xxHash
*/

#if defined(_MSC_VER)
#  define XXH_rotl32(x,r) _rotl(x,r)
#  define XXH_rotl64(x,r) _rotl64(x,r)
#else
#  define XXH_rotl32(x,r) ((x << r) | (x >> (32 - r)))
#  define XXH_rotl64(x,r) ((x << r) | (x >> (64 - r)))
#endif

template<int is_aligned>
static uint32_t XXH_get32bits(const uint8_t* p){
	return *(uint32_t*)p;
}

template<>
uint32_t XXH_get32bits<0>(const uint8_t* p){
	uint32_t val;
	memcpy(&val,p,sizeof(val));
	return val;
}

template<int is_aligned>
static uint64_t XXH_get64bits(const uint8_t* p){
	return *(uint64_t*)p;
}

template<>
uint64_t XXH_get64bits<0>(const uint8_t* p){
	uint64_t val;
	memcpy(&val,p,sizeof(val));
	return val;
}

#if JC_POINTER_SIZE>=8
	static const uint64_t PRIME64_1 = 11400714785074694791ULL;
	static const uint64_t PRIME64_2 = 14029467366897019727ULL;
	static const uint64_t PRIME64_3 =  1609587929392839161ULL;
	static const uint64_t PRIME64_4 =  9650029242287828579ULL;
	static const uint64_t PRIME64_5 =  2870177450012600261ULL;
	
	static uint64_t XXH64_round(uint64_t acc, uint64_t input)
	{
		acc += input * PRIME64_2;
		acc  = XXH_rotl64(acc, 31);
		acc *= PRIME64_1;
		return acc;
	}
	
	static uint64_t XXH64_mergeRound(uint64_t acc, uint64_t val)
	{
		val  = XXH64_round(0, val);
		acc ^= val;
		acc  = acc * PRIME64_1 + PRIME64_4;
		return acc;
	}
	
	template<int is_aligned>
	static uint64_t XXH(const void* input, size_t len)
	{
		uint64_t seed = 0;
		const uint8_t* p = (const uint8_t*)input;
		const uint8_t* bEnd = p + len;
		uint64_t h64;
	
		if (len>=32) {
			const uint8_t* const limit = bEnd - 32;
			uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
			uint64_t v2 = seed + PRIME64_2;
			uint64_t v3 = seed + 0;
			uint64_t v4 = seed - PRIME64_1;
	
			do {
				v1 = XXH64_round(v1, XXH_get64bits<is_aligned>(p)); p+=8;
				v2 = XXH64_round(v2, XXH_get64bits<is_aligned>(p)); p+=8;
				v3 = XXH64_round(v3, XXH_get64bits<is_aligned>(p)); p+=8;
				v4 = XXH64_round(v4, XXH_get64bits<is_aligned>(p)); p+=8;
			} while (p<=limit);
	
			h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);
			h64 = XXH64_mergeRound(h64, v1);
			h64 = XXH64_mergeRound(h64, v2);
			h64 = XXH64_mergeRound(h64, v3);
			h64 = XXH64_mergeRound(h64, v4);
	
		} else {
			h64  = seed + PRIME64_5;
		}
	
		h64 += (uint64_t) len;
	
		while (p+8<=bEnd) {
			uint64_t const k1 = XXH64_round(0, XXH_get64bits<is_aligned>(p));
			h64 ^= k1;
			h64  = XXH_rotl64(h64,27) * PRIME64_1 + PRIME64_4;
			p+=8;
		}
	
		if (p+4<=bEnd) {
			h64 ^= (uint64_t)(XXH_get32bits<is_aligned>(p)) * PRIME64_1;
			h64 = XXH_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
			p+=4;
		}
	
		while (p<bEnd) {
			h64 ^= (*p) * PRIME64_5;
			h64 = XXH_rotl64(h64, 11) * PRIME64_1;
			p++;
		}
	
		h64 ^= h64 >> 33;
		h64 *= PRIME64_2;
		h64 ^= h64 >> 29;
		h64 *= PRIME64_3;
		h64 ^= h64 >> 32;
	
		return h64;
	}
#else
	static const uint32_t PRIME32_1 = 2654435761U;
	static const uint32_t PRIME32_2 = 2246822519U;
	static const uint32_t PRIME32_3 = 3266489917U;
	static const uint32_t PRIME32_4 =  668265263U;
	static const uint32_t PRIME32_5 =  374761393U;
	
	static uint32_t XXH32_round(uint32_t seed, uint32_t input)
	{
		seed += input * PRIME32_2;
		seed  = XXH_rotl32(seed, 13);
		seed *= PRIME32_1;
		return seed;
	}
	
	template<int is_aligned>
	static uint32_t XXH(const void* input, size_t len)
	{
		uint32_t seed = 0u;
		const uint8_t* p = (const uint8_t*)input;
		const uint8_t* bEnd = p + len;
		uint32_t h32;

		if (len>=16) {
			const uint8_t* const limit = bEnd - 16;
			uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
			uint32_t v2 = seed + PRIME32_2;
			uint32_t v3 = seed + 0;
			uint32_t v4 = seed - PRIME32_1;
	
			do {
				v1 = XXH32_round(v1, XXH_get32bits<is_aligned>(p)); p+=4;
				v2 = XXH32_round(v2, XXH_get32bits<is_aligned>(p)); p+=4;
				v3 = XXH32_round(v3, XXH_get32bits<is_aligned>(p)); p+=4;
				v4 = XXH32_round(v4, XXH_get32bits<is_aligned>(p)); p+=4;
			} while (p<=limit);
	
			h32 = XXH_rotl32(v1, 1) + XXH_rotl32(v2, 7) + XXH_rotl32(v3, 12) + XXH_rotl32(v4, 18);
		} else {
			h32  = seed + PRIME32_5;
		}
	
		h32 += (uint32_t) len;
	
		while (p+4<=bEnd) {
			h32 += XXH_get32bits<is_aligned>(p) * PRIME32_3;
			h32  = XXH_rotl32(h32, 17) * PRIME32_4 ;
			p+=4;
		}
	
		while (p<bEnd) {
			h32 += (*p) * PRIME32_5;
			h32 = XXH_rotl32(h32, 11) * PRIME32_1 ;
			p++;
		}
	
		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;
	
		return h32;
	}
#endif
static uintptr_t hash_string(const int8_t *input, intptr_t len){
	if((intptr_t)input&(intptr_t)(sizeof(intptr_t)-1)){
		//unaligned
		return XXH<0>((void*)input,(size_t)len);
	}else{
		//aligned
		return XXH<1>((void*)input,(size_t)len);
	}
}
#endif

string_interned* intern_string(const char* data,size_t size){
	if(!data){return NULL;}
	auto g_table=CTableSingleton::get();
	size_t hash;
	#if __cplusplus>=201703L
		hash=std::hash<std::string_view>{}(std::string_view(data,size));
	#else
		hash=hash_string((const int8_t*)data,(intptr_t)size);
	#endif
	string_interned tmp;
	tmp.m_data=data;
	tmp.m_size=size;
	tmp.m_hash=hash;
	tmp.rc=0;
	auto iter=g_table->find(&tmp);
	if(iter!=g_table->end()){
		string_interned* ret=(*iter);
		ret->rc+=1;
		return ret;
	}
	string_interned* ret=(string_interned*)calloc(1,sizeof(string_interned)+size+1);
	ret->m_hash=hash;
	ret->rc=1;
	ret->m_data=(const char*)(ret+1);
	ret->m_size=size;
	memcpy(ret+1,data,size);
	g_table->insert(ret);
	return ret;
}

void free_interned_string(string_interned* p){
	auto g_table=CTableSingleton::get();
	g_table->erase(p);
	free(p);
}

}
