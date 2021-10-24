#ifndef _POSTFIX_JCH_HPP
#define _POSTFIX_JCH_HPP
#include "../script/jsenv.hpp"
//#pragma add("jc_files", "./postfix.jc");
namespace ama {
	ama::Node* ParsePostfix(ama::Node* nd_root, JSValue options);
	ama::Node* UnparseCall(ama::Node* nd);
	ama::Node* TranslatePostfixCall(ama::Node* ndi);
};

#endif
