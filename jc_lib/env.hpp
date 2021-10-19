#ifndef _ENV_JCH_HPP
#define _ENV_JCH_HPP
#include <string>
#include "../src/util/jc_array.h"
#include <memory>
#include <functional>
/*#pragma add("jc_files", "./env.jc");*/
namespace ENV {
	std::shared_ptr<std::string> get(JC::array_base<char> name);
	int set(JC::array_base<char> name, JC::array_base<char> value);
};

#endif
