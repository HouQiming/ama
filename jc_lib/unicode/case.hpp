#ifndef _CASE_JCH_HPP
#define _CASE_JCH_HPP
#include <string>
#include "../../src/util/jc_array.h"
#include <functional>
/*#pragma add("jc_files", "./case.jc");*/
namespace unicode {
	std::string toUpper(JC::array_base<char> s);
	std::string toLower(JC::array_base<char> s);
	std::string toUpperASCII(JC::array_base<char> s);
	std::string toLowerASCII(JC::array_base<char> s);
};

#endif
