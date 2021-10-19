#include <stdio.h>
#include <string>
#include <vector>
#include <array>
#include "../util/jc_array.h"
#include "../util/jc_unique_string.h"
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
static std::array<uint32_t, 8> g_spaces = ama::CreateCharSet(" \t\r");
static std::array<uint32_t, 8> g_not_newline = ama::CreateCharSet("^\n");
struct ParserState {
	ama::Node* nd_parent{};
	ama::Node** p_nd_next{};
	intptr_t indent_level{};
};
//updates comment_indent_level0 to the minimum indent inside the comment, which could be larger than its original value
static JC::unique_string FormatComment(intptr_t& comment_indent_level0, int32_t tab_width, char const* comment_begin, char const* comment_end) {
	intptr_t& comment_indent_level = comment_indent_level0;
	//if( !comment_indent_level || !memchr(comment_begin, '\n', comment_end - comment_begin) ) {
	//	return new char[|]!(comment_begin, comment_end - comment_begin);
	//}
	std::string tmp{};
	tmp.reserve(comment_end - comment_begin);
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
			char const* s1 = s;
			while ( s1 != comment_end && ama::isInCharSet(g_spaces, uint32_t(uint8_t(s1[intptr_t(1L)]))) ) {
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
	return JC::array_cast<JC::unique_string>(tmp);
}
static int canHaveRegexpAfter(ama::Node* nd) {
	//the first node can be regexp
	if ( !nd ) { return 1; }
	if ( nd->isSymbol("=") || nd->isSymbol(";") || nd->isSymbol(",") ) { return 1; }
	return 0;
}
namespace ama {
	//we rely on zero termination, thus the char*
	//start with comments_before almost everywhere
	ama::Node* ParseSimplePairing(char const* feed, JSValueConst options) {
		//for debugging
		char const* feed_all_begin = feed;
		JC::unique_string s_symbols = ama::UnwrapString(JS_GetPropertyStr(ama::jsctx, options, "symbols"));
		std::vector<std::span<char>> symbol_array{};
		{
			{
				//coulddo: SSE / NEON vectorization
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
			}
		}
		symbol_array--->sortby([] (auto item) -> uint32_t { return (uint32_t(item[0]) << 8) + uint32_t(255u - item.size()); });
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
		std::array<uint32_t, 8> identifier_charset = ama::CreateCharSet(JS_ToCString(ama::jsctx, JS_GetPropertyStr(ama::jsctx, options, "identifier_charset")));
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
		std::array<uint32_t, 8> cset_number = ama::CreateCharSet(JS_ToCString(ama::jsctx, JS_GetPropertyStr(ama::jsctx, options, "number_charset")));
		std::array<uint32_t, 8> cset_hex_number = ama::CreateCharSet(JS_ToCString(ama::jsctx, JS_GetPropertyStr(ama::jsctx, options, "hex_number_charset")));
		std::array<uint32_t, 8> cset_exponent = ama::CreateCharSet(JS_ToCString(ama::jsctx, JS_GetPropertyStr(ama::jsctx, options, "exponent_charset")));
		std::array<uint32_t, 8> cset_regexp_flags = ama::CreateCharSet(JS_ToCString(ama::jsctx, JS_GetPropertyStr(ama::jsctx, options, "regexp_flags_charset")));
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
			ama::Node* nd{};
			// [ \t\r] \n "' / 0-9 A-Za-z_\u0080+
			switch ( char_lut[intptr_t(ch)] ) {
				case CHAR_TYPE_SLASH: {
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
									if ( bracket_counter <= 0 && uint32_t(ch_i) == ch_closing ) {
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
						nd = ama::CreateNode(ama::N_JS_REGEXP, nullptr);
						nd->comments_before = FormatComment(comment_indent_level, tab_width, comment_begin, comment_end);
						nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
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
							nd->data = JC::array_cast<JC::unique_string>(tmp_buffer);
						} else {
							nd->data = JC::unique_string(feed, feed_end - feed);
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
						//*** PASS THROUGH ***
					}
				}
				case CHAR_TYPE_SYMBOL: {
					handle_symbol: 
					intptr_t lg = intptr_t(1L);
					for (intptr_t i = 0; i < intptr_t(n_symbol[ch]); i += intptr_t(1L)) {
						std::span<char> sym_i = symbol_array[p_symbol[ch] + i];
						if ( memcmp(feed, sym_i.data(), sym_i.size()) == 0 ) {
							lg = sym_i.size();
							break;
						}
					}
					nd = ama::CreateNode(ama::N_SYMBOL, nullptr);
					nd->comments_before = FormatComment(comment_indent_level, tab_width, comment_begin, comment_end);
					nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
					nd->data = JC::unique_string(feed, lg);
					*state_stack.back().p_nd_next = nd;
					state_stack.back().p_nd_next = &nd->s;
					feed += lg;
					comment_indent_level = current_indent_level;
					comment_begin = feed;
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_OPENING: {
					ama::Node* nd = ama::CreateNode(ama::N_RAW, nullptr);
					nd->comments_before = FormatComment(comment_indent_level, tab_width, comment_begin, comment_end);
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
				case CHAR_TYPE_CLOSING: {
					if ( state_stack.size() > intptr_t(1L) ) {
						ama::Node* nd = state_stack.back().nd_parent;
						if ( (comment_end - comment_begin) > 0 ) {
							JC::unique_string trailing_comment = FormatComment(comment_indent_level, tab_width, comment_begin, comment_end);
							if ( !nd->c ) {
								nd->Insert(ama::POS_FRONT, ama::nAir()->setIndent(
									ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level)
								));
							}
							ama::Node* nd_last = nd->LastChildSP();
							nd_last->comments_after = JC::array_cast<JC::unique_string>(JC::string_concat(nd_last->comments_after, trailing_comment));
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
					} else {
						goto handle_symbol;
					}
					break;
				}
				case CHAR_TYPE_BACKSLASH: {
					if ( feed[intptr_t(1L)] == '\n' ) {
						feed += 2;
					} else if ( feed[intptr_t(1L)] == '\r' && feed[intptr_t(2L)] == '\n' ) {
						feed += 3;
					} else {
						feed += feed[intptr_t(1L)] != '\000' ? intptr_t(2L) : intptr_t(1L);
					}
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_SPACE: {
					feed = ama::SkipChars(feed, g_spaces);
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_IDENTIFIER: {
					char const* feed0 = feed;
					feed = ama::SkipChars(feed, cset_identifier);
					nd = ama::CreateNode(ama::N_REF, nullptr);
					nd->comments_before = FormatComment(comment_indent_level, tab_width, comment_begin, comment_end);
					nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
					nd->data = JC::unique_string(feed0, feed - feed0);
					*state_stack.back().p_nd_next = nd;
					state_stack.back().p_nd_next = &nd->s;
					comment_indent_level = current_indent_level;
					comment_begin = feed;
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_NUMBER: {
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
					nd = ama::CreateNode(ama::N_NUMBER, nullptr);
					nd->comments_before = FormatComment(comment_indent_level, tab_width, comment_begin, comment_end);
					nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
					nd->data = JC::unique_string(feed0, feed - feed0);
					*state_stack.back().p_nd_next = nd;
					state_stack.back().p_nd_next = &nd->s;
					comment_indent_level = current_indent_level;
					comment_begin = feed;
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_QUOTE: {
					uint32_t ch_closing = ch;
					intptr_t lg = intptr_t(1L);
					int backslash_counter = 0;
					int premature_close = 0;
					for (; ;) {
						uint8_t ch_i = feed[lg];
						if ( ch_i == uint8_t(0) ) {
							premature_close = 1;
							break;
						}
						lg += intptr_t(1L);
						if ( int(ch_i) == int('\\') ) {
							backslash_counter ^= 1;
						} else {
							if ( uint32_t(ch_i) == ch_closing && !backslash_counter ) {
								break;
							}
							backslash_counter = 0;
						}
					}
					nd = ama::CreateNode(ama::N_STRING, nullptr);
					nd->comments_before = FormatComment(comment_indent_level, tab_width, comment_begin, comment_end);
					nd->indent_level = ama::ClampIndentLevel(comment_indent_level - state_stack.back().indent_level);
					if ( premature_close && finish_incomplete_code ) {
						std::string tmp_buffer(feed, lg);
						if ( backslash_counter ) {
							tmp_buffer.push_back('n');
							backslash_counter = 0;
						}
						tmp_buffer.push_back(char(ch_closing));
						nd->data = JC::array_cast<JC::unique_string>(tmp_buffer);
					} else {
						nd->data = JC::unique_string(feed, lg);
					}
					*state_stack.back().p_nd_next = nd;
					state_stack.back().p_nd_next = &nd->s;
					feed += lg;
					comment_indent_level = current_indent_level;
					comment_begin = feed;
					comment_end = feed;
					break;
				}
				case CHAR_TYPE_NEWLINE: {
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
				case CHAR_TYPE_ZERO: {
					if ( finish_incomplete_code ) {
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
					nd_root->comments_after = FormatComment(comment_indent_level, tab_width, comment_begin, comment_end);
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
