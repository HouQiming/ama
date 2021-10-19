#ifndef _CHARSET_JCH_HPP
#define _CHARSET_JCH_HPP
#include <array>
#include "../util/jc_array.h"
namespace ama {
	/*#pragma add("jc_files", "./charset.jc");*/
	static inline uint32_t isInCharSet(JC::array_base<uint32_t> cset, uint32_t ch) {
		ch &= 0xffu;
		return cset[ch >> 5] >> (ch & 31u) & 1u;
	}
	char const* SkipChars(char const* feed, JC::array_base<uint32_t> cset);
	std::array<uint32_t, 8> CreateCharSet(char const* s);
};

#endif
