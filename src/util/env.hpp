#ifndef _ENV_JCH_HPP
#define _ENV_JCH_HPP
#include <string>
#include <memory>
#include "jc_array.h"
/*#pragma add("jc_files", "./env.jc");*/
namespace ENV {
	JC::StringOrError get(std::span<char> name);
	int set(std::span<char> name, std::span<char> value);
};

#endif
