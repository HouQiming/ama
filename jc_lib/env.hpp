#ifndef _ENV_JCH_HPP
#define _ENV_JCH_HPP
#include <string>
#include "../src/util/jc_array.h"
#include <memory>
#include <functional>
/*#pragma add("jc_files", "./env.jc");*/
namespace ENV {
	std::shared_ptr<std::string> get(std::span<char> name);
	int set(std::span<char> name, std::span<char> value);
};

#endif
