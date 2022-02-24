#include <stdio.h>
#include <string>
#include <vector>
#include <array>
#include "../util/jc_array.h"
#include "../ast/node.hpp"
#include "../script/jsenv.hpp"
#include "../../modules/cpp/json/json.h"
#include "charset.hpp"
#include "simppair.hpp"

static const int CHAR_TYPE_SYMBOL = 0;
static const int CHAR_TYPE_SPACE = 1;
static const int CHAR_TYPE_QUOTE = 2;
static const int CHAR_TYPE_SLASH = 3;
static const int CHAR_TYPE_NUMBER = 4;
static const int CHAR_TYPE_IDENTIFIER = 5;
static const int CHAR_TYPE_NEWLINE = 6;
static const int CHAR_TYPE_ZERO = 7;
static const int CHAR_TYPE_BACKSLASH = 8;
static const int CHAR_TYPE_OPENING = 9;
static const int CHAR_TYPE_CLOSING = 10;
static const int CHAR_TYPE_SHELL_STRING = 11;
static std::array<uint32_t, 8> g_spaces = ama::CreateCharSet(" \t\r");
static std::array<uint32_t, 8> g_not_newline = ama::CreateCharSet("^\n");
struct ParserState {
	ama::Node* nd_parent{};
	ama::Node** p_nd_next{};
	intptr_t indent_level{};
	char is_shell_arg = 0;
};
//updates comment_indent_level0 to the minimum indent inside the comment, which could be larger than its original value
static ama::gcstring FormatComment(std::vector<char>& tmp, intptr_t& comment_indent_level0, int32_t tab_width, char const* comment_begin, char const* comment_end) {
	intptr_t& comment_indent_level = comment_indent_level0;
	//if( !comment_indent_level || !memchr(comment_begin, '\n', comment_end - comment_begin) ) {
	//	return new char[|]!(comment_begin, comment_end - comment_begin);
	//}
	//std::string tmp;
	//tmp.reserve(comment_end - comment_begin);
	tmp.clear();
	retry:
	int32_t min_indent_level = 0x7fffffff;
	for (char const* s = comment_begin; s != comment_end; s++) {
		char ch = *s;
		tmp.push_back(ch);
		if ( ch == '\n' ) {
			int32_t current_indent_level = 0;
			while ( s != comment_end && ama::isInCharSet(g_spaces, uint32_t(uint8_t(s[intptr_t(1L)]))) ) {
				if ( current_indent_level >= comment_indent_level ) {
					break;
				}
				if ( s[intptr_t(1L)] == ' ' ) {
					current_indent_level += intptr_t(1L);
				} else if ( s[intptr_t(1L)] == '\t' ) {
					current_indent_level /= tab_width;
					current_indent_level += 1;
					current_indent_level *= tab_width;
				}
				s += 1;
			}
			///////////////
			int32_t real_indent_level = current_indent_level;
			char const* s1 = s;while ( s1 != comment_end && ama::isInCharSet(g_spaces, uint32_t(uint8_t(s1[intptr_t(1L)]))) ) {
				if ( s1[intptr_t(1L)] == ' ' ) {
					real_indent_level += intptr_t(1L);
				} else if ( s1[intptr_t(1L)] == '\t' ) {
					real_indent_level /= tab_width;
					real_indent_level += 1;
					real_indent_level *= tab_width;
				}
				s1 += 1;
			}
			///////////////
			while ( current_indent_level > comment_indent_level ) {
				current_indent_level -= 1;
				tmp.push_back(' ');
			}
			if ( !(s != comment_end && s[1] == '\n') ) {
				//empty line doesn't count
				if ( min_indent_level > real_indent_level ) {
					min_indent_level = real_indent_level;
				}
			}
			if ( s == comment_end ) { break; }
		}
	}
	if ( min_indent_level != 0x7fffffff && min_indent_level != comment_indent_level ) {
		comment_indent_level = min_indent_level;
		comment_indent_level0 = min_indent_level;
		tmp.clear();
		goto retry;
	}
	//size_t! n = tmp.length;
	//while( n > 0L && (tmp[n - 1] == ' ' || tmp[n - 1] == '\t') ) {
	//	n -= 1;
	//}
	//if( n > 0L && tmp[n - 1] == '\n' ) {
	//	//drop trailing indent
	//	tmp.resize(n);
	//}
	return ama::gcstring(tmp);
}
static int canHaveRegexpAfter(ama::Node* nd) {
	//the first node can be regexp
	if ( !nd ) { return 1; }
	if ( 
		nd->isSymbol("=") || nd->isSymbol(";") || nd->isSymbol(",") || 
		nd->isSymbol(":") || nd->isSymbol("+") || nd->isSymbol("-") || 
		nd->isSymbol("||") || nd->isSymbol("&&") || nd->isSymbol("!") || 
		nd->isSymbol("==") || nd->isSymbol("!=") || 
		nd->isSymbol("===") || nd->isSymbol("!==") ||
		nd->isSymbol(">") || nd->isSymbol(">=") ||
		nd->isSymbol("<") || nd->isSymbol("<=") ||
		nd->isRef("return")
	) { return 1; }
	return 0;
}
static uint8_t g_utf8_bof[3] = {0xef, 0xbb, 0xbf};
namespace ama {
	//we rely on zero termination, thus the char*
	//start with comments_before almost everywhere
	static std::array<uint32_t, 8> GetOptionCSet(JSValueConst options, char const* name) {
		JSValue ppt = JS_GetPropertyStr(ama::jsctx, options, name);
		char const* cstr = JS_ToCString(ama::jsctx, ppt);
		std::array<uint32_t, 8> ret = ama::CreateCharSet(cstr);
		JS_FreeCString(ama::jsctx, cstr);
		JS_FreeValue(ama::jsctx, ppt);
		return std::move(ret);
	}
	ama::Node* ParseSimplePairing(char const* feed, JSValueConst options) {
		if (memcmp(feed, g_utf8_bof, 3) == 0) {
			//UTF-8 BOF marker
			feed += 3;
		}
		//for debugging
		//char const* feed_all_begin = feed;
		//use a shared buffer to avoid excessive allocations
		std::vector<char> comment_buffer{};
		ama::gcstring s_symbols = ama::UnwrapString(JS_GetPropertyStr(ama::jsctx, options, "symbols"));
		std::vector<std::span<char>> symbol_array{};
		size_t I0 = intptr_t(0L);
		for (size_t I = 0; I <= s_symbols.size(); ++I) {
			//char ch=(I<str.size()?str[I]:0)
			if ( I >= s_symbols.size() || s_symbols[I] == ' ' ) {
				//this scope will be if-wrapped
				size_t I00 = I0;
				I0 = I + 1;
				//the return will be replaced
				std::span<char> s_symbol(s_symbols.data() + I00, I - I00);
				if ( s_symbol.size() > 0 ) {
					symbol_array.push_back(s_symbol);
				}
			}
		}
		symbol_array--->sortby([](auto item)->uint32_t{ return (uint32_t(item[0]) << 8) + uint32_t(255u - item.size()); });
		if ( symbol_array.size() > 256 ) {
			fprintf(stderr, "we only support up to 256 symbols\n");
			symbol_array.resize(256);
		}
		std::array<uint8_t, 256> n_symbol{};
		std::array<uint8_t, 256> p_symbol{};
		for (size_t I = 0; I < symbol_array.size(); ++I) {
			std::span<char> const& s_symbol = symbol_array[I];
			if ( 0 == n_symbol[uint8_t(s_symbol[0])]++ ) {
				p_symbol[uint8_t(s_symbol[0])] = uint8_t(I);
			}
		}
		int32_t tab_width = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "tab_width"), 4);
		int32_t tab_indent = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "tab_indent"), 2);
		int32_t parse_js_regexp = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_js_regexp"), 1);
		int32_t finish_incomplete_code = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "finish_incomplete_code"), 0);
		//create char LUT
		std::array<uint8_t, 256> char_lut{};
		memset(char_lut.data(), CHAR_TYPE_SYMBOL, char_lut.size());
		std::array<uint32_t, 8> identifier_charset = GetOptionCSet(options, "identifier_charset");
		for (int ch = 0; ch <= 255; ch += 1) {
			if ( identifier_charset[ch >> 5] & 1u << (ch & 31) ) {
				char_lut[ch] = CHAR_TYPE_IDENTIFIER;
			}
		}
		//
		char_lut[0] = CHAR_TYPE_ZERO;
		char_lut[' '] = CHAR_TYPE_SPACE;
		char_lut['\t'] = CHAR_TYPE_SPACE;
		char_lut['\r'] = CHAR_TYPE_SPACE;
		for (int i = '0'; i <= '9'; i++) {
			char_lut[i] = CHAR_TYPE_NUMBER;
		}
		if ( ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "enable_unicode_identifiers"), 1) ) {
			for (int i = 128; i < 256; i++) {
				char_lut[i] = CHAR_TYPE_IDENTIFIER;
			}
		}
		char_lut['/'] = CHAR_TYPE_SLASH;
		char_lut['\n'] = CHAR_TYPE_NEWLINE;
		char_lut['"'] = CHAR_TYPE_QUOTE;
		char_lut['\''] = CHAR_TYPE_QUOTE;
		char_lut['\\'] = CHAR_TYPE_BACKSLASH;
		char_lut['('] = CHAR_TYPE_OPENING;
		char_lut['['] = CHAR_TYPE_OPENING;
		char_lut['{'] = CHAR_TYPE_OPENING;
		char_lut[')'] = CHAR_TYPE_CLOSING;
		char_lut[']'] = CHAR_TYPE_CLOSING;
		char_lut['}'] = CHAR_TYPE_CLOSING;
		if ( ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "enable_hash_comment"), 0) ) {
			char_lut['#'] = CHAR_TYPE_SLASH;
		}
		JSValue ppt{};
		char const* shell_string_quotes0 = JS_ToCString(ama::jsctx, ppt = JS_GetPropertyStr(ama::jsctx, options, "shell_string_quotes"));
		char const* shell_string_quotes = shell_string_quotes0;
		for (; *shell_string_quotes; shell_string_quotes += 1) {
			char_lut[intptr_t(*shell_string_quotes) & 0xff] = CHAR_TYPE_SHELL_STRING;
		}
		JS_FreeCString(ama::jsctx, shell_string_quotes0);
		JS_FreeValue(ama::jsctx, ppt);
		std::array<uint32_t, 8> cset_identifier{};
		cset_identifier--->fill(0u);
		for (int i = 0; i <= 255; i += 1) {
			if ( char_lut[i] == CHAR_TYPE_IDENTIFIER || char_lut[i] == CHAR_TYPE_NUMBER ) {
				cset_identifier[i >> 5] |= 1u << (i & 31);
			}
		}
		//////////////////
		//p and P are for C++ hex float, u and L are for C/C++ integers, n is for JS BigInt
		//b and o are for 0b and 0o
		std::array<uint32_t, 8> cset_number = GetOptionCSet(options, "number_charset");
		std::array<uint32_t, 8> cset_hex_number = GetOptionCSet(options, "hex_number_charset");
		std::array<uint32_t, 8> cset_exponent = GetOptionCSet(options, "exponent_charset");
		std::array<uint32_t, 8> cset_regexp_flags = GetOptionCSet(options, "regexp_flags_charset");
		//UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, ''),0)
		ama::Node* nd_root = ama::CreateNode(ama::N_RAW, nullptr);
		std::vector<ParserState> state_stack{
			ParserState{nd_root, &nd_root->c, 0}
		};
		char const* comment_begin = feed;
		char const* comment_end = feed;
		//interleave with parser lexer to fill indent_level, comments and all
		intptr_t comment_indent_level = intptr_t(0L);
		intptr_t current_indent_level = intptr_t(0L);
		intptr_t vote_tabness = intptr_t(0L);
		intptr_t vote_spaceness = intptr_t(0L);
		for (; ;) {
			uint32_t ch = uint32_t(uint8_t(feed[intptr_t(0L)]));
			// [ \t\r] \n "' / 0-9 A-Za-z_\u0080+
			switch ( char_lut[intptr_t(ch)] ) {
				case CHAR_TYPE_SLASH:{
					if ( ch == '#' || feed[intptr_t(1L)] == '/' ) {
						feed = ama::SkipChars(feed, g_not_newline);
						comment_end = feed;
						break;
					} else if ( feed[intptr_t(1L)] == '*' ) {
						intptr_t lg = intptr_t(2L);
						for (; ;) {
							if ( !feed[lg] ) {
								break;
							}
							if ( feed[lg] == '*' && feed[lg + intptr_t(1L)] == '/' ) {
								lg += intptr_t(2L);
								break;
							}
							lg += intptr_t(1L);
						}
						feed += lg;
						comment_end = feed;
						break;
					} else if ( parse_js_regexp && ch == '/' && canHaveRegexpAfter(state_stack.back().nd_parent->LastChildSP()) ) {
						uint32_t ch_closing = ch;
						intptr_t lg = intptr_t(1L);
						int backslash_counter = 0;
						int bracket_counter = 0;
						int in_charset = 0;
						int premature_close = 0;
						uint32_t flags = 0;
						for (; ;) {
							uint8_t ch_i = feed[lg];
							if ( ch_i == 0 ) {
								premature_close = 1;
								break;
							}
							lg += intptr_t(1L);
							if ( ch_i == '\\' ) {
								backslash_counter ^= 1;
							} else {
								if ( !backslash_counter ) {
									if ( uint32_t(ch_i) == ch_closing && !in_charset ) {
										if (bracket_counter > 0) {
											//unmatched group error, but we aren't checking it
											flags = ama::REGEXP_ERROR;
										}
										break;
									}
									if ( in_charset ) {
										if ( ch_i == ']' ) {
											in_charset = 0;
											bracket_counter -= 1;
										}
									} else {
										if ( char_lut[ch_i] == CHAR_TYPE_OPENING ) {
											if ( ch_i == '[' ) { in_charset = 1; }
											bracket_counter += 1;
										} else if ( char_lut[ch_i] == CHAR_TYPE_CLOSING ) {
											if ( bracket_counter > 0 ) { bracket_counter -= 1; }
										}
									}
								}
								backslash_counter = 0;
							}
						}
						//search flags
						char const* feed_end = ama::SkipChars(feed + lg, cset_regexp_flags);
						ama::Node* nd = ama::CreateNode(ama::N_JS_REGEXP, nullptr);
						nd->comments_before = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
						nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
						nd->flags = flags;
						if ( premature_close && finish_incomplete_code ) {
							std::string tmp_buffer(feed, feed_end - feed);
							if ( backslash_counter ) {
								tmp_buffer.push_back('n');
								backslash_counter = 0;
							}
							if ( in_charset ) {
								tmp_buffer.push_back(']');
							}
							while ( bracket_counter > 0 ) {
								tmp_buffer.push_back(')');
								bracket_counter -= 1;
							}
							tmp_buffer.push_back(char(ch_closing));
							nd->data = ama::gcstring(tmp_buffer);
						} else {
							nd->data = ama::gcstring(feed, feed_end - feed);
						}
						*state_stack.back().p_nd_next = nd;
						state_stack.back().p_nd_next = &nd->s;
						feed = feed_end;
						comment_indent_level = current_indent_level;
						comment_begin = feed;
						comment_end = feed;
						break;
					} else {
						goto handle_symbol;
					}
				}
				case CHAR_TYPE_SYMBOL:{
					handle_symbol:
					if (ch == '.' && char_lut[feed[1]] == CHAR_TYPE_NUMBER) {
						goto handle_number;
					}
					intptr_t lg = intptr_t(1L);
					for (intptr_t i = 0; i < intptr_t(n_symbol[ch]); i += intptr_t(1L)) {
						std::span<char> sym_i = symbol_array[p_symbol[ch] + i];
						if ( memcmp(feed, sym_i.data(), sym_i.size()) == 0 ) {
							lg = sym_i.size();
							break;
						}
					}
					ama::Node* nd = ama::CreateNode(ama::N_SYMBOL, nullptr);
					nd->comments_before = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
					nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
					nd->data = ama::gcstring(feed, lg);
					*state_stack.back().p_nd_next = nd;
					state_stack.back().p_nd_next = &nd->s;
					feed += lg;
					comment_indent_level = current_indent_level;
					comment_begin = feed;
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_OPENING:{
					ama::Node* nd = ama::CreateNode(ama::N_RAW, nullptr);
					nd->comments_before = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
					nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
					nd->flags = uint32_t(ch);
					*state_stack.back().p_nd_next = nd;
					state_stack.back().p_nd_next = &nd->s;
					state_stack.push_back(
						ParserState{.nd_parent = nd, .p_nd_next = &nd->c, .indent_level = current_indent_level}
					);
					feed += 1;
					comment_indent_level = current_indent_level;
					comment_begin = feed;
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_CLOSING:{
					if ( state_stack.size() > intptr_t(1L) ) {
						char is_shell_arg = state_stack.back().is_shell_arg;
						ama::Node* nd = state_stack.back().nd_parent;
						if ( (comment_end - comment_begin) > 0 ) {
							ama::gcstring trailing_comment = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
							if ( !nd->c ) {
								nd->Insert(ama::POS_FRONT, ama::nAir()->setIndent(
									ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level)
								));
							}
							ama::Node* nd_last = nd->LastChildSP();
							nd_last->comments_after = ama::gcscat(nd_last->comments_after, trailing_comment);
						}
						nd->flags |= uint32_t(ch) << 8;
						state_stack.pop_back();
						//snap to closing_indent_level: we could have the opening indent level screwed up in code like this
						/*
						void foo(   int bar,
									int baz) {
							//this scope's indent level will be screwed up
						}
						*/
						int8_t closing_indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
						if ( closing_indent_level < nd->indent_level ) {
							//adjust inner indent levels
							for (ama::Node* ndi_0 = nd->c; ndi_0; ndi_0 = ndi_0->s) {
								{
									ndi_0->AdjustIndentLevel(nd->indent_level - closing_indent_level);
								}
							}
							nd->indent_level = closing_indent_level;
						}
						feed += 1;
						comment_indent_level = current_indent_level;
						comment_begin = feed;
						comment_end = feed;
						if (is_shell_arg) {
							//continue the shell string, tag 0x100 to update lg
							ch = uint32_t(uint8_t(is_shell_arg)) | 0x100;
							goto continue_shell_string;
						}
					} else {
						goto handle_symbol;
					}
					break;
				}
				case CHAR_TYPE_BACKSLASH:{
					if ( feed[intptr_t(1L)] == '\n' ) {
						feed += 2;
					} else if ( feed[intptr_t(1L)] == '\r' && feed[intptr_t(2L)] == '\n' ) {
						feed += 3;
					} else {
						feed += feed[intptr_t(1L)] != char(0) ? intptr_t(2L) : intptr_t(1L);
					}
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_SPACE:{
					feed = ama::SkipChars(feed, g_spaces);
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_IDENTIFIER:{
					char const* feed0 = feed;
					feed = ama::SkipChars(feed, cset_identifier);
					ama::Node* nd = ama::CreateNode(ama::N_REF, nullptr);
					nd->comments_before = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
					nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
					nd->data = ama::gcstring(feed0, feed - feed0);
					*state_stack.back().p_nd_next = nd;
					state_stack.back().p_nd_next = &nd->s;
					comment_indent_level = current_indent_level;
					comment_begin = feed;
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_NUMBER:{
					handle_number:
					char const* feed0 = feed;
					int has_exponent = 0;
					if ( feed[intptr_t(0L)] == '0' && feed[intptr_t(1L)] == 'x' ) {
						feed = ama::SkipChars(feed, cset_hex_number);
						has_exponent = feed[-intptr_t(1L)] == 'p' || feed[-intptr_t(1L)] == 'P';
					} else {
						feed = ama::SkipChars(feed, cset_number);
						has_exponent = feed[-intptr_t(1L)] == 'e' || feed[-intptr_t(1L)] == 'E';
					}
					if ( (feed[intptr_t(0L)] == '+' || feed[intptr_t(0L)] == '-') && has_exponent ) {
						//0.000e+00
						feed += intptr_t(1L);
						feed = ama::SkipChars(feed, cset_exponent);
					}
					assert(feed != feed0);
					ama::Node* nd = ama::CreateNode(ama::N_NUMBER, nullptr);
					nd->comments_before = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
					nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
					nd->data = ama::gcstring(feed0, feed - feed0);
					*state_stack.back().p_nd_next = nd;
					state_stack.back().p_nd_next = &nd->s;
					comment_indent_level = current_indent_level;
					comment_begin = feed;
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_SHELL_STRING:
				case CHAR_TYPE_QUOTE:{
				continue_shell_string: {
					uint32_t ss_start = ch;
					intptr_t lg = intptr_t(1L);
					if (ch & 0x100) {
						//continuation
						lg = 0;
						ch &= 0xff;
						ss_start = 0;
					}
					uint32_t ch_closing = ch;
					uint32_t ss_end = ch;
					int backslash_counter = 0;
					bool premature_close = false;
					bool is_shell_string = char_lut[intptr_t(ch)] == CHAR_TYPE_SHELL_STRING;
					char is_shell_arg = 0;
					int done = 0;
					for (; ;) {
						uint8_t ch_i = feed[lg];
						if ( ch_i == uint8_t(0) ) {
							premature_close = true;
							break;
						}
						lg += intptr_t(1L);
						if ( int(ch_i) == int('\\') ) {
							backslash_counter ^= 1;
						} else {
							if ( uint32_t(ch_i) == ch_closing && !backslash_counter ) {
								break;
							}
							if (is_shell_string && ch_i == '$' && feed[lg] == '{') {
								//treat as termination
								is_shell_arg = ch_closing;
								ss_end = '$';
								break;
							}
							backslash_counter = 0;
						}
					}
					if (is_shell_string && is_shell_arg && ss_start == ch) {
						//we're the beginning, push a raw
						ama::Node* nd = ama::CreateNode(ama::N_RAW, nullptr);
						nd->comments_before = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
						nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
						nd->flags = uint32_t(ch);
						*state_stack.back().p_nd_next = nd;
						state_stack.back().p_nd_next = &nd->s;
						state_stack.push_back(
							ParserState{.nd_parent = nd, .p_nd_next = &nd->c, .indent_level = current_indent_level}
						);
						comment_indent_level = current_indent_level;
						comment_begin = feed;
						comment_end = feed;
					}
					ama::Node* nd = ama::CreateNode(ama::N_STRING, nullptr);
					if (is_shell_string) {
						nd->flags |= ama::STRING_SHELL_LIKE;
						if (ss_start == ch) {nd->flags |= ama::STRING_SHELL_LIKE_START;}
						if (ss_end == ch) {nd->flags |= ama::STRING_SHELL_LIKE_END;}
					}
					nd->comments_before = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
					nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
					if ( premature_close && finish_incomplete_code ) {
						std::string tmp_buffer(feed, lg);
						if ( backslash_counter ) {
							tmp_buffer.push_back('n');
							backslash_counter = 0;
						}
						tmp_buffer.push_back(char(ch_closing));
						nd->data = ama::gcstring(tmp_buffer);
					} else {
						char const* s0 = feed;
						intptr_t lg0 = lg;
						if (is_shell_string) {
							if (ss_start == ch && lg0 > 0) {
								s0 += 1;
								lg0 -= 1;
							}
							if (ss_end == ch && lg0 > 0) {
								lg0 -= 1;
							}
						}
						nd->data = ama::gcstring(s0, lg0);
					}
					*state_stack.back().p_nd_next = nd;
					state_stack.back().p_nd_next = &nd->s;
					feed += lg;
					comment_indent_level = current_indent_level;
					comment_begin = feed;
					comment_end = feed;
					/////////////
					if (is_shell_arg) {
						ch = uint32_t(uint8_t(feed[intptr_t(0L)]));
						assert(ch == '{');
						//push the attached bracket here
						ama::Node* nd = ama::CreateNode(ama::N_RAW, nullptr);
						//nd->comments_before = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
						nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
						nd->flags = uint32_t(ch);
						*state_stack.back().p_nd_next = nd;
						state_stack.back().p_nd_next = &nd->s;
						state_stack.push_back(
							ParserState{.nd_parent = nd, .p_nd_next = &nd->c, .indent_level = current_indent_level, .is_shell_arg = is_shell_arg}
						);
						feed += 1;
						comment_indent_level = current_indent_level;
						comment_begin = feed;
						comment_end = feed;
					} else if (is_shell_string && ss_start != ch && ss_end == ch && state_stack.size() && state_stack.back().nd_parent->flags == uint32_t(ch)) {
						//we're closing, pop once more
						ama::Node* nd = state_stack.back().nd_parent;
						nd->flags |= uint32_t(ch) << 8;
						state_stack.pop_back();
					}
					break;
				}
				}
				case CHAR_TYPE_NEWLINE:{
					current_indent_level = intptr_t(0L);
					feed += intptr_t(1L);
					if ( ama::isInCharSet(g_spaces, uint32_t(uint8_t(feed[intptr_t(0L)]))) ) {
						char const* feed0 = feed;
						//feed = SkipChars(feed, g_spaces);
						//a more mixing-friendly counting - tab_width
						current_indent_level = 0;
						while ( ama::isInCharSet(g_spaces, uint32_t(uint8_t(feed[intptr_t(0L)]))) ) {
							if ( feed[intptr_t(0L)] == ' ' ) {
								current_indent_level += intptr_t(1L);
								vote_spaceness += 1;
							} else if ( feed[intptr_t(0L)] == '\t' ) {
								current_indent_level /= tab_width;
								current_indent_level += 1;
								current_indent_level *= tab_width;
								vote_tabness += tab_width;
							}
							feed += 1;
						}
					}
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_ZERO:{
					if ( finish_incomplete_code ) {
						ama::Node* nd_last = state_stack.back().nd_parent;
						if (nd_last->c) {nd_last = nd_last->LastChildSP();}
						nd_last->comments_after = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
						comment_indent_level = current_indent_level;
						comment_begin = feed;
						comment_end = feed;
						while ( state_stack.size() > intptr_t(1L) ) {
							ama::Node* nd = state_stack.back().nd_parent;
							uint32_t ch = nd->flags & 0xffu;
							if ( ch == '(' ) {
								ch = ')';
							} else if ( ch == '[' ) {
								ch = ']';
							} else if ( ch == '{' ) {
								ch = '}';
							}
							nd->flags |= ch << 8;
							state_stack.pop_back();
						}
					}
					ama::FixParents(nullptr, nd_root);
					nd_root->comments_after = FormatComment(comment_buffer, comment_indent_level, tab_width, comment_begin, comment_end);
					if ( tab_indent == 2 ) {
						//auto tab-indent: update the root flag
						if ( vote_spaceness > vote_tabness ) {
							//treat as space indent
							nd_root->flags |= 0x10000;
						}
						//JS_SetPropertyStr(ama::jsctx, options, 'tab_indent', JS_NewInt32(
						//	ama::jsctx,
						//	vote_tabness >= vote_spaceness ? 1 : 0
						//));
					}
					return nd_root;
				}
			}
		}
	}
};
