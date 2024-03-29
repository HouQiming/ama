#ifndef _CHARSET_JCH_HPP
#define _CHARSET_JCH_HPP
#include <array>
#include "../util/jc_array.h"
namespace ama {
	#pragma add("c_files", "./charset.cpp");
	static inline uint32_t isInCharSet(std::span<uint32_t> cset, uint32_t ch) {
		ch &= 0xffu;
		return cset[ch >> 5] >> (ch & 31u) & 1u;
	}
	char const* SkipChars(char const* feed, std::span<uint32_t> cset);
	std::array<uint32_t, 8> CreateCharSet(char const* s);
};

#endif
