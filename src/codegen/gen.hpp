#ifndef _GEN_JCH_HPP
#define _GEN_JCH_HPP
#include "../ast/node.hpp"
#include <string>
#include "../util/jc_array.h"
/*#pragma add("jc_files", "./gen.jc");*/
namespace ama {
	class CodeGenerator;
	typedef int(*FGeneratorHook)(ama::CodeGenerator*, ama::Node*);
	typedef int(*FGeneratorHookComment)(ama::CodeGenerator*, const char*, intptr_t, int, intptr_t);
	struct CodeGenerator {
		//ignore offsets for now
		//COULDDO: provide a separate service to convert some (unicode) tag into cite-like comments
		std::string code{};
		ama::Node* nd_current = nullptr;
		void const* opaque = nullptr;
		FGeneratorHook hook = nullptr;
		FGeneratorHookComment hook_comment = nullptr;
		intptr_t scope_indent_level = intptr_t(0L);
		intptr_t p_last_indent = intptr_t(0L);
		int32_t tab_width = 4;
		int8_t auto_space = 1;
		int8_t tab_indent = 1;
		void GenerateDefault(ama::Node* nd);
		void GenerateComment(int is_after, intptr_t expected_indent_level, std::span<char> comment);
		void Generate(ama::Node* nd);
		void GenerateSpaceBefore(ama::Node* nd_next);
		void GenerateIndent(intptr_t expected_indent_level);
		void GenerateCommentDefault(int is_after, intptr_t expected_indent_level, std::span<char> comment);
		void GenerateSpaceAfter(ama::Node* nd_last);
		void GenerateSpaceBetween(ama::Node* nd_last, ama::Node* nd_next);
	};
	std::string GenerateCode(ama::Node* nd, JSValueConst options);
};

#endif
