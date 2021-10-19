//////////////////////////////
//Unicode case conversion - here we have to rely on STL
#include "jc_platform.h"
#if JC_OS == JC_OS_WINDOWS
	#include <windows.h>
	/*#pragma add("ldflags", "user32.lib");*/
#else
	#include <wctype.h>
	//the C95 to-w-foo-er doesn't work on Unicode by default on Windows
#endif
#include "unicode.hpp"
#include "unicode/case.hpp"
#include <string>
#include <vector>
#include "../../src/util/jc_array.h"
//std::locale is unreliable garbage, use [NSString uppercaseString] for Apple, and towupper / towlower for Linux
std::string unicode::toUpper(std::span<char> s) {
	std::vector<uint16_t> s16 = unicode::WTF8ToUTF16(s);
	#if JC_OS == JC_OS_WINDOWS
		for (size_t i = uintptr_t(0uL); i < s.size(); i += uintptr_t(1048576uL)) {
			size_t n_i = s.size() - i;
			if ( n_i > uintptr_t(1048576uL) ) {
				n_i = uintptr_t(1048576uL);
			}
			CharUpperBuffW(LPWSTR(s16.data() + i), int(n_i));
		}
		return JC::array_cast<std::string>(unicode::UTF16ToUTF8(s16));
	#else
		{
			std::vector<uint16_t> ret((s16.size()));
			for (size_t i = 0; i < s16.size(); ++i) {
				uint16_t value = uint16_t(towupper(s16[i]));
				ret[i] = std::move(value);
			}
			return JC::array_cast<std::string>(unicode::UTF16ToUTF8(ret));
		}
	#endif
}
std::string unicode::toLower(std::span<char> s) {
	std::vector<uint16_t> s16 = unicode::WTF8ToUTF16(s);
	#if JC_OS == JC_OS_WINDOWS
		for (size_t i = uintptr_t(0uL); i < s.size(); i += uintptr_t(1048576uL)) {
			size_t n_i = s.size() - i;
			if ( n_i > uintptr_t(1048576uL) ) {
				n_i = uintptr_t(1048576uL);
			}
			CharLowerBuffW(LPWSTR(s16.data() + i), int(n_i));
		}
		return JC::array_cast<std::string>(unicode::UTF16ToUTF8(s16));
	#else
		{
			std::vector<uint16_t> ret((s16.size()));
			for (size_t i = 0; i < s16.size(); ++i) {
				uint16_t value = uint16_t(towlower(s16[i]));
				ret[i] = std::move(value);
			}
			return JC::array_cast<std::string>(unicode::UTF16ToUTF8(ret));
		}
	#endif
}
std::string unicode::toUpperASCII(std::span<char> s) {
	std::string ret(s.size(), '\000');
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
	std::string ret(s.size(), '\000');
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
