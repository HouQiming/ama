//////////////////////////////
//Unicode case conversion - here we have to rely on STL
#if defined(_WIN32)
	#include <windows.h>
	/*#pragma add("ldflags", "user32.lib");*/
#else
	#include <wctype.h>
	//the C95 to-w-foo-er doesn't work on Unicode by default on Windows
#endif
#include "../unicode.hpp"
#include "./case.hpp"
#include <string>
#include <vector>
#include "../jc_array.h"
//std::locale is unreliable garbage, use [NSString uppercaseString] for Apple, and towupper / towlower for Linux
std::string unicode::toUpper(std::span<char> s) {
	std::vector<uint16_t> s16 = unicode::WTF8ToUTF16(s);
	#if defined(_WIN32)
		for (size_t i = uintptr_t(0uL); i < s.size(); i += uintptr_t(1048576uL)) {
			size_t n_i = s.size() - i;
			if ( n_i > uintptr_t(1048576uL) ) {
				n_i = uintptr_t(1048576uL);
			}
			CharUpperBuffW(LPWSTR(s16.data() + i), int(n_i));
		}
		return (unicode::UTF16ToUTF8(s16));
	#else
		{
			std::vector<uint16_t> ret((s16.size()));
			for (size_t i = 0; i < s16.size(); ++i) {
				uint16_t value = uint16_t(towupper(s16[i]));
				ret[i] = std::move(value);
			}
			return (unicode::UTF16ToUTF8(ret));
		}
	#endif
}
std::string unicode::toLower(std::span<char> s) {
	std::vector<uint16_t> s16 = unicode::WTF8ToUTF16(s);
	#if defined(_WIN32)
		for (size_t i = uintptr_t(0uL); i < s.size(); i += uintptr_t(1048576uL)) {
			size_t n_i = s.size() - i;
			if ( n_i > uintptr_t(1048576uL) ) {
				n_i = uintptr_t(1048576uL);
			}
			CharLowerBuffW(LPWSTR(s16.data() + i), int(n_i));
		}
		return (unicode::UTF16ToUTF8(s16));
	#else
		{
			std::vector<uint16_t> ret((s16.size()));
			for (size_t i = 0; i < s16.size(); ++i) {
				uint16_t value = uint16_t(towlower(s16[i]));
				ret[i] = std::move(value);
			}
			return (unicode::UTF16ToUTF8(ret));
		}
	#endif
}
std::string unicode::toUpperASCII(std::span<char> s) {
	std::string ret(s.size(), char(0));
	for (size_t i = 0; i < s.size(); ++i) {
		char const& ch0 = s[i];
		int ch = int(ch0);
		if ( ch >= 'a' && ch <= 'z' ) {
			ch -= 0x20;
		}
		ret[i] = char(ch);
	}
	return std::move(ret);
}
std::string unicode::toLowerASCII(std::span<char> s) {
	std::string ret(s.size(), char(0));
	for (size_t i = 0; i < s.size(); ++i) {
		char const& ch0 = s[i];
		int ch = int(ch0);
		if ( ch >= 'A' && ch <= 'Z' ) {
			ch += 0x20;
		}
		ret[i] = char(ch);
	}
	return std::move(ret);
}
