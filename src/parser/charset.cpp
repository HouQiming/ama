#include "charset.hpp"
#include <array>
#include "../util/jc_array.h"
namespace ama {
	char const* SkipChars(char const* feed, JC::array_base<uint32_t> cset) {
		while ( ama::isInCharSet(cset, uint32_t(uint8_t(feed[intptr_t(0L)]))) ) {
			feed += 1;
		}
		return feed;
	}
	static inline void set(std::array<uint32_t, 8>& ok, int c) {
		c &= 0xff;
		ok[c >> 5] |= 1u << (c & 31);
	}
	std::array<uint32_t, 8> CreateCharSet(char const* s) {
		std::array<uint32_t, 8> ok{};
		ok--->fill(0u);
		int is_not = 0;
		if ( s[0] == '^' ) {
			is_not = 1;
			s += 1;
		}
		for (; *s; s++) {
			if ( s[1] == '-' && s[2] ) {
				for (int32_t j = s[0]; j <= s[2]; j++) {
					set(ok, j);
				}
				s += 2;
				//} else if( s[0] == '-'&&!s[1] ) {
				//	//hack for high bytes
				//	for(int j = 4; j < 8; j++) {
				//		ok[j] = 0xffffffffu;
				//	}
			} else {
				set(ok, *s);
			}
		}
		if ( is_not ) {
			for (int j = 0; j < 8; j++) {
				ok[j] = ~ok[j];
			}
		}
		//\0 is never in any charset
		ok[0] &= ~1u;
		return std::move(ok);
	}
};
