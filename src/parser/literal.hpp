#ifndef _LITERAL_JCH_HPP
#define _LITERAL_JCH_HPP
#include "charset.hpp"
#include <string>
#include "../util/jc_array.h"
namespace ama {
	/*#pragma add("jc_files", "./literal.jc");*/
	std::string escapeJSString(std::span<char> s);
	std::string ParseJCString(std::span<char> s);
	void escapeStringBody(std::string &ret, std::span<char> s);
	std::string ParseStringBody(std::span<char> s);
};

#endif
