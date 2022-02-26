#ifndef _LITERAL_JCH_HPP
#define _LITERAL_JCH_HPP
#include "charset.hpp"
#include <string>
#include "../util/jc_array.h"
namespace ama {
	static const uint32_t FLAG_ESCAPE_SINGLE_QUOTE = 1u;
	static const uint32_t FLAG_ESCAPE_DOUBLE_QUOTE = 2u;
	static const uint32_t FLAG_ESCAPE_NEWLINE = 4u;
	/*#pragma add("jc_files", "./literal.jc");*/
	std::string escapeJSString(std::span<char> s);
	std::string ParseJCString(std::span<char> s);
	void escapeStringBody(std::string &ret, std::span<char> s, uint32_t flags);
	std::string ParseStringBody(std::span<char> s);
};

#endif
