#ifndef __FOO
#define __FOO
namespace ama {
	typedef int(*FGeneratorHook)(ama::CodeGenerator*,ama::Node*);
	typedef int(*FGeneratorHookComment)(ama::CodeGenerator*,const char*,intptr_t,int,intptr_t);
	static const int REACH_FORWARD=0;
	struct CodeGenerator {
		//ignore offsets for now
		//COULDDO: provide a separate service to convert some (unicode) tag into cite-like comments
		std::string code;
		Node* nd_current;
		void const* opaque;
		FGeneratorHook hook;
		FGeneratorHookComment hook_comment{};
		intptr_t scope_indent_level = intptr_t(0L);
		intptr_t p_last_indent = intptr_t(0L);
		int32_t tab_width = 4;
		int8_t auto_space = 1;
		int8_t tab_indent = 1;
		void GenerateDefault(ama::Node* nd);
		void GenerateComment(int is_after, intptr_t expected_indent_level, JC::array_base<char> comment);
		void Generate(ama::Node* nd);
		void GenerateSpaceBefore(ama::Node* nd_next);
		void GenerateIndent(intptr_t expected_indent_level);
		void GenerateCommentDefault(int is_after, intptr_t expected_indent_level, JC::array_base<char> comment);
		void GenerateSpaceAfter(ama::Node* nd_last);
		void GenerateSpaceBetween(ama::Node* nd_last, ama::Node* nd_next);
	};
}
#endif
