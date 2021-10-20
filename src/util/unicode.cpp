#include "unicode.hpp"
#include <string>
#include <vector>
#include "jc_array.h"
std::vector<uint16_t> unicode::WTF8ToUTF16(std::span<char> s) {
	std::vector<uint16_t> ret{};
	unicode::TWTF8Filter filter{.ch0 = 0, .nnxt = 0, .II = intptr_t(0L), .surrogate_1st = -1, .surrogate_II = intptr_t(0L), .ch_min = 0};
	for (intptr_t I = intptr_t(0L); I < intptr_t(s.size()); I++) {
		int32_t ch = filter.NextByte(I, int32_t(uint32_t(uint8_t(s[I]))));
		if ( ch < 0 ) {
			continue;
		}
		if ( ch >= 0x10000 ) {
			//surrogate pair
			ch -= 0x10000;
			ret--->push(uint16_t(uint32_t(0xd800 + (ch >> 10 & 0x3ff))));
			ch &= 0x3ff;
			ch += 0xdc00;
		}
		ret--->push(uint16_t(uint32_t(ch)));
	}
	return std::move(ret);
}
std::vector<int32_t> unicode::WTF8ToUTF32(std::span<char> s) {
	std::vector<int32_t> ret{};
	unicode::TWTF8Filter filter{.ch0 = 0, .nnxt = 0, .II = intptr_t(0L), .surrogate_1st = -1, .surrogate_II = intptr_t(0L), .ch_min = 0};
	for (intptr_t I = intptr_t(0L); I < intptr_t(s.size()); I++) {
		int32_t ch = filter.NextByte(I, int32_t(uint32_t(uint8_t(s[I]))));
		if ( ch >= 0 ) {
			ret--->push(ch);
		}
	}
	return std::move(ret);
}
std::string unicode::UTF16ToUTF8(std::span<uint16_t> s) {
	std::string ret{};
	int32_t surrogate_1st = -1;
	intptr_t surrogate_II = intptr_t(0L);
	for ( uint16_t const &ch: s ) {
		int32_t chi = int32_t(uint32_t(ch));
		if ( (chi & ~0x3ff) == 0xd800 ) {
			surrogate_1st = chi & 0x3ff;
			surrogate_II = ret.size();
		} else {
			if ( (chi & ~0x3ff) == 0xdc00 && surrogate_1st >= 0 ) {
				//resolve surrogate pairs if found
				//leave unpaired halves unchanged 
				chi = surrogate_1st * 0x400 + (chi & 0x3ff) + 0x10000;
				ret.resize(surrogate_II);
			}
			surrogate_1st = -1;
			surrogate_II = intptr_t(0L);
		}
		unicode::AppendUTF8Char(ret, chi);
	}
	return std::move(ret);
}
std::string unicode::UTF32ToUTF8(std::span<int32_t> s) {
	std::string ret{};
	for ( int32_t const &chi: s ) {
		unicode::AppendUTF8Char(ret, chi);
	}
	return std::move(ret);
}
