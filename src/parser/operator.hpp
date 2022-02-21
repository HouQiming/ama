#ifndef _OPERATOR_JCH_HPP
#define _OPERATOR_JCH_HPP
/*#pragma add("jc_files", "./operator.jc");*/
namespace ama {
	ama::Node* ParseAssignment(ama::Node* nd_root, JSValueConst options);
	ama::Node* ParseOperators(ama::Node* nd_root, JSValueConst options);
	ama::Node* ParsePointedBrackets(ama::Node* nd_root);
	ama::Node* UnparseBinop(ama::Node* nd_binop);
	ama::Node* ParseColons(ama::Node* nd_root, JSValueConst options);
	ama::Node* UnparseLabel(ama::Node* nd_label);
	ama::Node* UnparsePrefix(ama::Node* nd_unary);
	ama::Node* UnparsePostfix(ama::Node* nd_unary);
	ama::Node* ParseCommaExpr(ama::Node* nd_root);
};

#endif
