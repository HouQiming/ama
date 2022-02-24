#ifndef _SCOPING_JCH_HPP
#define _SCOPING_JCH_HPP
/*#pragma add("jc_files", "./scoping.jc");*/
namespace ama {
	ama::Node* DelimitCLikeStatements(ama::Node* nd_root, JSValue options);
	void ConvertToScope(ama::Node* nd_raw);
	ama::Node* ConvertIndentToScope(ama::Node* nd_root, JSValue options);
	ama::Node* InsertJSSemicolons(ama::Node* nd_root, JSValue options);
};

#endif
