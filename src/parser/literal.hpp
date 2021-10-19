#ifndef _LITERAL_JCH_HPP
#define _LITERAL_JCH_HPP
#include "charset.hpp"
#include <string>
#include "../util/jc_array.h"
namespace ama {
	/*#pragma add("jc_files", "./literal.jc");*/
	std::string escapeJSString(JC::array_base<char> s);
	std::string ParseJCString(JC::array_base<char> s);
};

#endif
