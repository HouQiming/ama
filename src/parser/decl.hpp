#ifndef _DECL_JCH_HPP
#define _DECL_JCH_HPP
/*#pragma add("jc_files", "./decl.jc");*/
namespace ama {
	ama::Node* ConvertToParameterList(ama::Node* nd_raw);
	ama::Node* ParseScopedStatements(ama::Node* nd_root, JSValue options);
	ama::Node* ParseKeywordStatements(ama::Node* nd_root, JSValue options);
	ama::Node* ParseDeclarations(ama::Node* nd_root, JSValue options);
};

#endif
