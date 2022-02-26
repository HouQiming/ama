#include <string>
#include <array>
#include "../util/jc_array.h"
#include "../util/unicode.hpp"
#include "charset.hpp"
#include "literal.hpp"
namespace ama {
	static char const* g_hex = "0123456789abcdef";
	static const std::array<uint32_t, 8> g_hex_charset = ama::CreateCharSet("0-9A-Fa-f");
	void escapeStringBody(std::string &ret, std::span<char> s, uint32_t flags) {
		for ( char const & ch: s ) {
			switch ( ch ) {
				default:{
					if ( uint8_t(ch) >= uint8_t(' ') ) {
						ret--->push(ch);
						break;
					} else {
						//*** PASS THROUGH ***
					}
					//*** PASS THROUGH ***
				}
				case char(0x7f):{
					ret--->push("\\u00", ama::g_hex[intptr_t(ch >> 4) & intptr_t(0xfL)], ama::g_hex[intptr_t(ch) & intptr_t(0xfL)]);
					break;
				}
				case'\t':{
					if (flags & ama::FLAG_ESCAPE_NEWLINE) {
						ret--->push('\\', 't');
					} else {
						ret.push_back('\t');
					}
					break;
				}
				case'\b':{
					ret--->push('\\', 'b');
					break;
				}
				case'\r':{
					if (flags & ama::FLAG_ESCAPE_NEWLINE) {
						ret--->push('\\', 'r');
					} else {
						ret.push_back('\r');
					}
					break;
				}
				case'\n':{
					if (flags & ama::FLAG_ESCAPE_NEWLINE) {
						ret--->push('\\', 'n');
					} else {
						ret.push_back('\n');
					}
					break;
				}
				case'\\':{
					ret--->push('\\', '\\');
					break;
				}
				case'\'':{
					if (flags & ama::FLAG_ESCAPE_SINGLE_QUOTE) {
						ret.push_back('\\');
					}
					ret.push_back('\'');
					break;
				}
				case'\"':{
					if (flags & ama::FLAG_ESCAPE_DOUBLE_QUOTE) {
						ret.push_back('\\');
					}
					ret.push_back('"');
					break;
				}
			}
		}
	}
	std::string escapeJSString(std::span<char> s) {
		std::string ret{};
		ret--->push('\'');
		escapeStringBody(ret, s, ama::FLAG_ESCAPE_SINGLE_QUOTE | ama::FLAG_ESCAPE_NEWLINE);
		ret--->push('\'');
		return std::move(ret);
	}
	//we don't have to be fully conformant here - this won't get executed when we convert C / C++ code
	std::string ParseStringBody(std::span<char> s) {
		std::string ret{};
		for (intptr_t i = 0; i < intptr_t(s.size()); ++i) {
			char ch = s[i];
			if ( ch == '\\' ) {
				i += intptr_t(1L);
				char ch_next = s[i];
				switch ( ch_next ) {
					default:{
						// #'
						if ( (s.size() - i) >= intptr_t(2L) ) {
							ret--->push(ch_next);
						}
						break;
					}
					case'a':{
						ret--->push(char(7));
						break;
					}
					case'b':{
						ret--->push('\b');
						break;
					}
					case't':{
						ret--->push('\t');
						break;
					}
					case'n':{
						ret--->push('\n');
						break;
					}
					case'v':{
						ret--->push(char(11));
						break;
					}
					case'f':{
						ret--->push(char(12));
						break;
					}
					case'r':{
						ret--->push('\r');
						break;
					}
					case'x':{
						// x##'
						if ( (s.size() - i) >= intptr_t(4L) ) {
							std::array<char, intptr_t(3L)> tmp{};
							tmp[intptr_t(0L)] = s[i + intptr_t(1L)];
							tmp[intptr_t(1L)] = s[i + intptr_t(2L)];
							ret--->push(char(uint8_t(strtoull((char const*)(tmp.data()), nullptr, 16))));
						}
						i += intptr_t(2L);
						break;
					}
					case'0': case'1': case'2': case'3': case'4': case'5': case'6': case'7':{
						int oct = 0;
						oct = oct * 8 + int(int(ch_next) - 48);
						// ##'
						if ( (s.size() - i) >= intptr_t(3L) && s[i + intptr_t(1L)] >= '0' && s[i + intptr_t(1L)] <= '7' ) {
							i += intptr_t(1L);
							oct = oct * 8 + int(s[i]) - 48;
						}
						if ( (s.size() - i) >= intptr_t(3L) && s[i + intptr_t(1L)] >= '0' && s[i + intptr_t(1L)] <= '7' ) {
							i += intptr_t(1L);
							oct = oct * 8 + int(s[i]) - 48;
						}
						ret--->push(char(uint8_t(uint32_t(oct))));
						break;
					}
					case'u':{
						// u{#}'
						std::array<char, intptr_t(9L)> tmp{};
						if ( (s.size() - i) >= intptr_t(5L) && s[i + intptr_t(1L)] == '{' ) {
							i += intptr_t(2L);
							intptr_t i0 = i;
							// #}'
							while ( (s.size() - i) >= intptr_t(3L) && (i - i0) <= intptr_t(8L) && ama::isInCharSet(g_hex_charset, uint32_t(s[i])) ) {
								i += intptr_t(1L);
							}
							memcpy((void*)(tmp.data()), (void const*)(&s[i0]), i - i0);
							//the for skips the remaining }
						} else if ( (s.size() - i) >= intptr_t(6L) ) {
							// u####'
							memcpy((void*)(tmp.data()), (void const*)(&s[i + intptr_t(1L)]), intptr_t(4L));
							i += intptr_t(4L);
						}
						int codepoint = int32_t(int64_t(strtoull((char const*)(tmp.data()), nullptr, 16)));
						unicode::AppendUTF8Char(ret, codepoint);
						break;
					}
				}
			} else {
				ret--->push(ch);
			}
		}
		return std::move(ret);
	}
	std::string ParseJCString(std::span<char> s) {
		if ( !s.size() || (s[intptr_t(0L)] != '\'' && s[intptr_t(0L)] != '"') || s.size() < 2 ) {
			return JC::array_cast<std::string>(s);
		}
		return ParseStringBody(s--->subarray(1, s.size() - 2));
	}
};
