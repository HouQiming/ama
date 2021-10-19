#ifndef _POSTFIX_JCH_HPP
#define _POSTFIX_JCH_HPP
/*#pragma add("jc_files", "./postfix.jc");*/
namespace ama {
	ama::Node* ParsePostfix(ama::Node* nd_root, int parse_air_object);
	ama::Node* UnparseCall(ama::Node* nd);
	ama::Node* TranslatePostfixCall(ama::Node* ndi);
};

#endif
