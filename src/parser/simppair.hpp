#ifndef _SIMPPAIR_JCH_HPP
#define _SIMPPAIR_JCH_HPP
#include "../ast/node.hpp"
#include "../script/jsenv.hpp"
/*#pragma add("jc_files", "./simppair.jc");*/
namespace ama {
	ama::Node* ParseSimplePairing(char const* feed, JSValueConst options);
};

#endif
