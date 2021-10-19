#ifndef _DEPENDS_JCH_HPP
#define _DEPENDS_JCH_HPP
/*#pragma add("jc_files", "./depends.jc");*/
namespace ama {
	void ParseCInclude(ama::Node* nd_root);
	void ParseJSRequire(ama::Node* nd_root);
	ama::Node* ParseDependency(ama::Node* nd_root, JSValueConst options);
};

#endif
