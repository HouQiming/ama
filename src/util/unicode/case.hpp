#ifndef _CASE_JCH_HPP
#define _CASE_JCH_HPP
#include <string>
#include "../jc_array.h"
#include <functional>
/*#pragma add("jc_files", "./case.jc");*/
namespace unicode {
	std::string toUpper(std::span<char> s);
	std::string toLower(std::span<char> s);
	std::string toUpperASCII(std::span<char> s);
	std::string toLowerASCII(std::span<char> s);
};

#endif
