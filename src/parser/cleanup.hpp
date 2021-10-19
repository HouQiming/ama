#ifndef _CLEANUP_JCH_HPP
#define _CLEANUP_JCH_HPP
/*#pragma add("jc_files", "./cleanup.jc");*/
namespace ama {
	ama::Node* CleanupDummyRaws(ama::Node* nd_root);
	ama::Node* SanitizeCommentPlacement(ama::Node* nd_root);
	//ama::Node*+! StripBinaryOperatorSpaces(ama::Node*+! nd_root);
	ama::Node* AutoFormat(ama::Node* nd_root);
	ama::Node* NodifySemicolonAndParenthesis(ama::Node* nd_root);
	ama::Node* FixPriorityReversal(ama::Node* nd_root);
};

#endif
