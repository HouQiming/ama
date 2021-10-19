//@ama require('./jsgen.js')('ama',ParseCurrentFile()).Save('.cpp');
#include "./jsenv.hpp"
#pragma gen(js_bindings)
namespace ama {
	auto NodeGet_node_class(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return JS_NewInt32(jsctx, nd->node_class);
	}
	auto NodeSet_node_class(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res0 = 0;
		if (JS_ToInt32(jsctx, &res0, val) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `val`");
		}
		nd->node_class = (uint8_t)(res0);
		return JS_UNDEFINED;
	}
	auto NodeGet_indent_level(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return JS_NewInt32(jsctx, nd->indent_level);
	}
	auto NodeSet_indent_level(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res1 = 0;
		if (JS_ToInt32(jsctx, &res1, val) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `val`");
		}
		nd->indent_level = res1;
		return JS_UNDEFINED;
	}
	auto NodeGet_tmp_flags(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return JS_NewInt32(jsctx, nd->tmp_flags);
	}
	auto NodeSet_tmp_flags(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res2 = 0;
		if (JS_ToInt32(jsctx, &res2, val) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `val`");
		}
		nd->tmp_flags = res2;
		return JS_UNDEFINED;
	}
	auto NodeGet_flags(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return JS_NewInt32(jsctx, nd->flags);
	}
	auto NodeSet_flags(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res3 = 0;
		if (JS_ToInt32(jsctx, &res3, val) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `val`");
		}
		nd->flags = res3;
		return JS_UNDEFINED;
	}
	auto NodeGet_data(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapStringNullable(nd->data);
	}
	auto NodeSet_data(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (JS_IsNull(val)) {
			nd->data = NULL;
		} else {
			if (!JS_IsString(val)) {
				return JS_ThrowTypeError(jsctx, "string expected for `val`");
			}
			nd->data = ama::UnwrapString(val);
		};
		return JS_UNDEFINED;
	}
	auto NodeGet_comments_before(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapStringNullable(nd->comments_before);
	}
	auto NodeSet_comments_before(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (JS_IsNull(val)) {
			nd->comments_before = NULL;
		} else {
			if (!JS_IsString(val)) {
				return JS_ThrowTypeError(jsctx, "string expected for `val`");
			}
			nd->comments_before = ama::UnwrapString(val);
		};
		return JS_UNDEFINED;
	}
	auto NodeGet_comments_after(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapStringNullable(nd->comments_after);
	}
	auto NodeSet_comments_after(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (JS_IsNull(val)) {
			nd->comments_after = NULL;
		} else {
			if (!JS_IsString(val)) {
				return JS_ThrowTypeError(jsctx, "string expected for `val`");
			}
			nd->comments_after = ama::UnwrapString(val);
		};
		return JS_UNDEFINED;
	}
	auto NodeGet_c(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->c);
	}
	auto NodeSet_c(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		nd->c = ama::UnwrapNode(val);
		return JS_UNDEFINED;
	}
	auto NodeGet_s(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->s);
	}
	auto NodeSet_s(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		nd->s = ama::UnwrapNode(val);
		return JS_UNDEFINED;
	}
	auto NodeGet_p(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->p);
	}
	auto NodeSet_p(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		nd->p = ama::UnwrapNode(val);
		return JS_UNDEFINED;
	}
	auto NodeGet_v(JSContext* jsctx, JSValueConst this_val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->v);
	}
	auto NodeSet_v(JSContext* jsctx, JSValueConst this_val, JSValueConst val) {
		auto nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		nd->v = ama::UnwrapNode(val);
		return JS_UNDEFINED;
	}
	JSValue NodeCall_setData(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (argc > 0 && !JS_IsNull(argv[0L])) {
			if (!JS_IsString((0 < argc ? argv[0L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
			}
		}
		return ama::WrapNode(nd->setData(argc > 0 && !JS_IsNull(argv[0L]) ? ama::UnwrapString((0 < argc ? argv[0L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_setFlags(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res4 = 0;
		if (JS_ToInt32(jsctx, &res4, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		return ama::WrapNode(nd->setFlags(res4));
	}
	JSValue NodeCall_setCommentsBefore(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (argc > 0 && !JS_IsNull(argv[0L])) {
			if (!JS_IsString((0 < argc ? argv[0L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
			}
		}
		return ama::WrapNode(nd->setCommentsBefore(argc > 0 && !JS_IsNull(argv[0L]) ? ama::UnwrapString((0 < argc ? argv[0L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_setCommentsAfter(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (argc > 0 && !JS_IsNull(argv[0L])) {
			if (!JS_IsString((0 < argc ? argv[0L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
			}
		}
		return ama::WrapNode(nd->setCommentsAfter(argc > 0 && !JS_IsNull(argv[0L]) ? ama::UnwrapString((0 < argc ? argv[0L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_setIndent(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res5 = 0;
		if (JS_ToInt32(jsctx, &res5, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		return ama::WrapNode(nd->setIndent(res5));
	}
	JSValue NodeCall_Clone(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->Clone());
	}
	JSValue NodeCall_ReplaceWith(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->ReplaceWith(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_Unlink(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->Unlink());
	}
	JSValue NodeCall_Insert(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (nd == NULL) {
			return JS_ThrowTypeError(jsctx, "cannot insert at a null node");
		}
		if (JS_IsNull(argv[1]) || JS_IsUndefined(argv[1])) {
			return JS_ThrowTypeError(jsctx, "cannot insert a null node");
		}
		int32_t res6 = 0;
		if (JS_ToInt32(jsctx, &res6, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		return ama::WrapNode(nd->Insert(res6, ama::UnwrapNode((1 < argc ? argv[1L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_Root(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->Root());
	}
	JSValue NodeCall_RootStatement(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->RootStatement());
	}
	JSValue NodeCall_isAncestorOf(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return JS_NewInt32(jsctx, nd->isAncestorOf(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_Owning(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res7 = 0;
		if (JS_ToInt32(jsctx, &res7, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		return ama::WrapNode(nd->Owning(res7));
	}
	JSValue NodeCall_Owner(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->Owner());
	}
	JSValue NodeCall_LastChildSP(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->LastChildSP());
	}
	JSValue NodeCall_LastChild(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->LastChild());
	}
	JSValue NodeCall_CommonAncestor(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->CommonAncestor(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_GetStringValue(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapStringNullable(nd->GetStringValue());
	}
	JSValue NodeCall_dot(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (argc > 0 && !JS_IsNull(argv[0L])) {
			if (!JS_IsString((0 < argc ? argv[0L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
			}
		}
		return ama::WrapNode(nd->dot(argc > 0 && !JS_IsNull(argv[0L]) ? ama::UnwrapString((0 < argc ? argv[0L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_FreeASTStorage(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		nd->FreeASTStorage(); return JS_UNDEFINED;
	}
	JSValue NodeCall_Find(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res8 = 0;
		if (JS_ToInt32(jsctx, &res8, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		if (argc > 1 && !JS_IsNull(argv[1L])) {
			if (!JS_IsString((1 < argc ? argv[1L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(1<argc?argv[1L]:JS_UNDEFINED)`");
			}
		}
		return ama::WrapNode(nd->Find(res8, argc > 1 && !JS_IsNull(argv[1L]) ? ama::UnwrapString((1 < argc ? argv[1L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_FindAll(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res9 = 0;
		if (JS_ToInt32(jsctx, &res9, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		if (argc > 1 && !JS_IsNull(argv[1L])) {
			if (!JS_IsString((1 < argc ? argv[1L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(1<argc?argv[1L]:JS_UNDEFINED)`");
			}
		}
		return ama::WrapNodeArray(nd->FindAll(res9, argc > 1 && !JS_IsNull(argv[1L]) ? ama::UnwrapString((1 < argc ? argv[1L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_FindAllWithin(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res10 = 0;
		if (JS_ToInt32(jsctx, &res10, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		int32_t res11 = 0;
		if (JS_ToInt32(jsctx, &res11, (1 < argc ? argv[1L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(1<argc?argv[1L]:JS_UNDEFINED)`");
		}
		if (argc > 2 && !JS_IsNull(argv[2L])) {
			if (!JS_IsString((2 < argc ? argv[2L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(2<argc?argv[2L]:JS_UNDEFINED)`");
			}
		}
		return ama::WrapNodeArray(nd->FindAllWithin(res10, res11, argc > 2 && !JS_IsNull(argv[2L]) ? ama::UnwrapString((2 < argc ? argv[2L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_FindAllBefore(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res12 = 0;
		if (JS_ToInt32(jsctx, &res12, (1 < argc ? argv[1L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(1<argc?argv[1L]:JS_UNDEFINED)`");
		}
		int32_t res13 = 0;
		if (JS_ToInt32(jsctx, &res13, (2 < argc ? argv[2L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(2<argc?argv[2L]:JS_UNDEFINED)`");
		}
		if (argc > 3 && !JS_IsNull(argv[3L])) {
			if (!JS_IsString((3 < argc ? argv[3L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(3<argc?argv[3L]:JS_UNDEFINED)`");
			}
		}
		return ama::WrapNodeArray(nd->FindAllBefore(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED)), res12, res13, argc > 3 && !JS_IsNull(argv[3L]) ? ama::UnwrapString((3 < argc ? argv[3L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_isRawNode(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res14 = 0;
		if (JS_ToInt32(jsctx, &res14, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		int32_t res15 = 0;
		if (JS_ToInt32(jsctx, &res15, (1 < argc ? argv[1L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(1<argc?argv[1L]:JS_UNDEFINED)`");
		}
		return JS_NewInt32(jsctx, nd->isRawNode(res14, res15));
	}
	JSValue NodeCall_GetName(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapStringNullable(nd->GetName());
	}
	JSValue NodeCall_toSource(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapString(nd->toSource());
	}
	JSValue NodeCall_isMethodCall(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (argc > 0 && !JS_IsNull(argv[0L])) {
			if (!JS_IsString((0 < argc ? argv[0L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
			}
		}
		return JS_NewInt32(jsctx, nd->isMethodCall(argc > 0 && !JS_IsNull(argv[0L]) ? ama::UnwrapString((0 < argc ? argv[0L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_InsertDependency(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res16 = 0;
		if (JS_ToInt32(jsctx, &res16, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		if (argc > 1 && !JS_IsNull(argv[1L])) {
			if (!JS_IsString((1 < argc ? argv[1L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(1<argc?argv[1L]:JS_UNDEFINED)`");
			}
		}
		return ama::WrapNode(nd->InsertDependency(res16, argc > 1 && !JS_IsNull(argv[1L]) ? ama::UnwrapString((1 < argc ? argv[1L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_InsertCommentBefore(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (argc > 0 && !JS_IsNull(argv[0L])) {
			if (!JS_IsString((0 < argc ? argv[0L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
			}
		}
		return ama::WrapNode(nd->InsertCommentBefore(argc > 0 && !JS_IsNull(argv[0L]) ? ama::UnwrapString((0 < argc ? argv[0L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_MergeCommentsBefore(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->MergeCommentsBefore(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_MergeCommentsAfter(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->MergeCommentsAfter(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_MergeCommentsAndIndentAfter(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->MergeCommentsAndIndentAfter(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_DestroyForSymbol(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapStringNullable(nd->DestroyForSymbol());
	}
	JSValue NodeCall_isSymbol(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (argc > 0 && !JS_IsNull(argv[0L])) {
			if (!JS_IsString((0 < argc ? argv[0L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
			}
		}
		return JS_NewInt32(jsctx, nd->isSymbol(argc > 0 && !JS_IsNull(argv[0L]) ? ama::UnwrapString((0 < argc ? argv[0L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_isRef(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		if (argc > 0 && !JS_IsNull(argv[0L])) {
			if (!JS_IsString((0 < argc ? argv[0L] : JS_UNDEFINED))) {
				return JS_ThrowTypeError(jsctx, "string expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
			}
		}
		return JS_NewInt32(jsctx, nd->isRef(argc > 0 && !JS_IsNull(argv[0L]) ? ama::UnwrapString((0 < argc ? argv[0L] : JS_UNDEFINED)) : nullptr));
	}
	JSValue NodeCall_Validate(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		nd->Validate(); return JS_UNDEFINED;
	}
	JSValue NodeCall_ValidateEx(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res17 = 0;
		if (JS_ToInt32(jsctx, &res17, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		int32_t res18 = 0;
		if (JS_ToInt32(jsctx, &res18, (1 < argc ? argv[1L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(1<argc?argv[1L]:JS_UNDEFINED)`");
		}
		return JS_NewInt32(jsctx, nd->ValidateEx(res17, res18));
	}
	JSValue NodeCall_NeedTrailingSemicolon(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return JS_NewInt32(jsctx, nd->NeedTrailingSemicolon());
	}
	JSValue NodeCall_GetCommentedIndentLevel(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res19 = 0;
		if (JS_ToInt32(jsctx, &res19, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		return JS_NewInt32(jsctx, nd->GetCommentedIndentLevel(res19));
	}
	JSValue NodeCall_ParentStatement(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->ParentStatement());
	}
	JSValue NodeCall_Prev(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->Prev());
	}
	JSValue NodeCall_BreakSibling(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->BreakSibling());
	}
	JSValue NodeCall_BreakChild(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->BreakChild());
	}
	JSValue NodeCall_BreakSelf(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->BreakSelf());
	}
	JSValue NodeCall_ReplaceUpto(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->ReplaceUpto(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED)), ama::UnwrapNode((1 < argc ? argv[1L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_ValidateChildCount(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res20 = 0;
		if (JS_ToInt32(jsctx, &res20, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		return JS_NewInt32(jsctx, nd->ValidateChildCount(res20));
	}
	JSValue NodeCall_AdjustIndentLevel(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		int32_t res21 = 0;
		if (JS_ToInt32(jsctx, &res21, (0 < argc ? argv[0L] : JS_UNDEFINED)) < 0) {
			return JS_ThrowTypeError(jsctx, "int expected for `(0<argc?argv[0L]:JS_UNDEFINED)`");
		}
		nd->AdjustIndentLevel(res21); return JS_UNDEFINED;
	}
	JSValue NodeCall_PreorderNext(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->PreorderNext(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_PreorderSkipChildren(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->PreorderSkipChildren(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_PreorderLastInside(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->PreorderLastInside());
	}
	JSValue NodeCall_PostorderFirst(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->PostorderFirst());
	}
	JSValue NodeCall_PostorderNext(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->PostorderNext(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED))));
	}
	JSValue NodeCall_toSingleNode(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->toSingleNode());
	}
	JSValue NodeCall_Unparse(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return ama::WrapNode(nd->Unparse());
	}
	JSValue NodeCall_GetCFGRole(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return JS_NewInt32(jsctx, nd->GetCFGRole());
	}
	JSValue NodeCall_isChildCFGDependent(JSContext* jsctx, JSValueConst this_val, int argc, JSValueConst* argv) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(this_val, ama::g_node_classid));
		return JS_NewInt32(jsctx, nd->isChildCFGDependent(ama::UnwrapNode((0 < argc ? argv[0L] : JS_UNDEFINED))));
	}
	void GeneratedJSBindings() {
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "node_class"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_node_class), "get_node_class", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_node_class), "set_node_class", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "indent_level"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_indent_level), "get_indent_level", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_indent_level), "set_indent_level", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "tmp_flags"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_tmp_flags), "get_tmp_flags", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_tmp_flags), "set_tmp_flags", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "flags"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_flags), "get_flags", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_flags), "set_flags", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "data"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_data), "get_data", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_data), "set_data", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "comments_before"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_comments_before), "get_comments_before", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_comments_before), "set_comments_before", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "comments_after"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_comments_after), "get_comments_after", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_comments_after), "set_comments_after", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "c"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_c), "get_c", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_c), "set_c", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "s"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_s), "get_s", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_s), "set_s", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "p"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_p), "get_p", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_p), "set_p", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_DefinePropertyGetSet(jsctx, ama::g_node_proto, JS_NewAtom(jsctx, "v"), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeGet_v), "get_v", 0, JS_CFUNC_getter, 0), JS_NewCFunction2(jsctx, (JSCFunction*)(NodeSet_v), "set_v", 1, JS_CFUNC_setter, 0), JS_PROP_C_W_E);
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "setData", JS_NewCFunction(jsctx, (NodeCall_setData), "Node.setData", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "setFlags", JS_NewCFunction(jsctx, (NodeCall_setFlags), "Node.setFlags", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "setCommentsBefore", JS_NewCFunction(jsctx, (NodeCall_setCommentsBefore), "Node.setCommentsBefore", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "setCommentsAfter", JS_NewCFunction(jsctx, (NodeCall_setCommentsAfter), "Node.setCommentsAfter", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "setIndent", JS_NewCFunction(jsctx, (NodeCall_setIndent), "Node.setIndent", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "Clone", JS_NewCFunction(jsctx, (NodeCall_Clone), "Node.Clone", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "ReplaceWith", JS_NewCFunction(jsctx, (NodeCall_ReplaceWith), "Node.ReplaceWith", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "Unlink", JS_NewCFunction(jsctx, (NodeCall_Unlink), "Node.Unlink", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "Insert", JS_NewCFunction(jsctx, (NodeCall_Insert), "Node.Insert", 2));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "Root", JS_NewCFunction(jsctx, (NodeCall_Root), "Node.Root", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "RootStatement", JS_NewCFunction(jsctx, (NodeCall_RootStatement), "Node.RootStatement", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "isAncestorOf", JS_NewCFunction(jsctx, (NodeCall_isAncestorOf), "Node.isAncestorOf", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "Owning", JS_NewCFunction(jsctx, (NodeCall_Owning), "Node.Owning", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "Owner", JS_NewCFunction(jsctx, (NodeCall_Owner), "Node.Owner", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "LastChildSP", JS_NewCFunction(jsctx, (NodeCall_LastChildSP), "Node.LastChildSP", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "LastChild", JS_NewCFunction(jsctx, (NodeCall_LastChild), "Node.LastChild", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "CommonAncestor", JS_NewCFunction(jsctx, (NodeCall_CommonAncestor), "Node.CommonAncestor", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "GetStringValue", JS_NewCFunction(jsctx, (NodeCall_GetStringValue), "Node.GetStringValue", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "dot", JS_NewCFunction(jsctx, (NodeCall_dot), "Node.dot", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "FreeASTStorage", JS_NewCFunction(jsctx, (NodeCall_FreeASTStorage), "Node.FreeASTStorage", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "Find", JS_NewCFunction(jsctx, (NodeCall_Find), "Node.Find", 2));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "FindAll", JS_NewCFunction(jsctx, (NodeCall_FindAll), "Node.FindAll", 2));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "FindAllWithin", JS_NewCFunction(jsctx, (NodeCall_FindAllWithin), "Node.FindAllWithin", 3));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "FindAllBefore", JS_NewCFunction(jsctx, (NodeCall_FindAllBefore), "Node.FindAllBefore", 4));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "isRawNode", JS_NewCFunction(jsctx, (NodeCall_isRawNode), "Node.isRawNode", 2));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "GetName", JS_NewCFunction(jsctx, (NodeCall_GetName), "Node.GetName", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "toSource", JS_NewCFunction(jsctx, (NodeCall_toSource), "Node.toSource", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "isMethodCall", JS_NewCFunction(jsctx, (NodeCall_isMethodCall), "Node.isMethodCall", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "InsertDependency", JS_NewCFunction(jsctx, (NodeCall_InsertDependency), "Node.InsertDependency", 2));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "InsertCommentBefore", JS_NewCFunction(jsctx, (NodeCall_InsertCommentBefore), "Node.InsertCommentBefore", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "MergeCommentsBefore", JS_NewCFunction(jsctx, (NodeCall_MergeCommentsBefore), "Node.MergeCommentsBefore", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "MergeCommentsAfter", JS_NewCFunction(jsctx, (NodeCall_MergeCommentsAfter), "Node.MergeCommentsAfter", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "MergeCommentsAndIndentAfter", JS_NewCFunction(jsctx, (NodeCall_MergeCommentsAndIndentAfter), "Node.MergeCommentsAndIndentAfter", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "DestroyForSymbol", JS_NewCFunction(jsctx, (NodeCall_DestroyForSymbol), "Node.DestroyForSymbol", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "isSymbol", JS_NewCFunction(jsctx, (NodeCall_isSymbol), "Node.isSymbol", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "isRef", JS_NewCFunction(jsctx, (NodeCall_isRef), "Node.isRef", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "Validate", JS_NewCFunction(jsctx, (NodeCall_Validate), "Node.Validate", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "ValidateEx", JS_NewCFunction(jsctx, (NodeCall_ValidateEx), "Node.ValidateEx", 2));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "NeedTrailingSemicolon", JS_NewCFunction(jsctx, (NodeCall_NeedTrailingSemicolon), "Node.NeedTrailingSemicolon", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "GetCommentedIndentLevel", JS_NewCFunction(jsctx, (NodeCall_GetCommentedIndentLevel), "Node.GetCommentedIndentLevel", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "ParentStatement", JS_NewCFunction(jsctx, (NodeCall_ParentStatement), "Node.ParentStatement", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "Prev", JS_NewCFunction(jsctx, (NodeCall_Prev), "Node.Prev", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "BreakSibling", JS_NewCFunction(jsctx, (NodeCall_BreakSibling), "Node.BreakSibling", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "BreakChild", JS_NewCFunction(jsctx, (NodeCall_BreakChild), "Node.BreakChild", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "BreakSelf", JS_NewCFunction(jsctx, (NodeCall_BreakSelf), "Node.BreakSelf", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "ReplaceUpto", JS_NewCFunction(jsctx, (NodeCall_ReplaceUpto), "Node.ReplaceUpto", 2));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "ValidateChildCount", JS_NewCFunction(jsctx, (NodeCall_ValidateChildCount), "Node.ValidateChildCount", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "AdjustIndentLevel", JS_NewCFunction(jsctx, (NodeCall_AdjustIndentLevel), "Node.AdjustIndentLevel", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "PreorderNext", JS_NewCFunction(jsctx, (NodeCall_PreorderNext), "Node.PreorderNext", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "PreorderSkipChildren", JS_NewCFunction(jsctx, (NodeCall_PreorderSkipChildren), "Node.PreorderSkipChildren", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "PreorderLastInside", JS_NewCFunction(jsctx, (NodeCall_PreorderLastInside), "Node.PreorderLastInside", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "PostorderFirst", JS_NewCFunction(jsctx, (NodeCall_PostorderFirst), "Node.PostorderFirst", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "PostorderNext", JS_NewCFunction(jsctx, (NodeCall_PostorderNext), "Node.PostorderNext", 1));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "toSingleNode", JS_NewCFunction(jsctx, (NodeCall_toSingleNode), "Node.toSingleNode", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "Unparse", JS_NewCFunction(jsctx, (NodeCall_Unparse), "Node.Unparse", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "GetCFGRole", JS_NewCFunction(jsctx, (NodeCall_GetCFGRole), "Node.GetCFGRole", 0));
		JS_SetPropertyStr(jsctx, ama::g_node_proto, "isChildCFGDependent", JS_NewCFunction(jsctx, (NodeCall_isChildCFGDependent), "Node.isChildCFGDependent", 1));
		ama::g_node_class_names.resize(31);
		ama::g_builder_names.resize(31);
		JSValueConst global = JS_GetGlobalObject(ama::jsctx);
		ama::g_node_class_names[N_NONE] = "N_NONE";
		ama::g_builder_names[N_NONE] = "nNone";
		ama::g_node_class_names[N_RAW] = "N_RAW";
		ama::g_builder_names[N_RAW] = "nRaw";
		ama::g_node_class_names[N_SYMBOL] = "N_SYMBOL";
		ama::g_builder_names[N_SYMBOL] = "nSymbol";
		ama::g_node_class_names[N_REF] = "N_REF";
		ama::g_builder_names[N_REF] = "nRef";
		ama::g_node_class_names[N_NUMBER] = "N_NUMBER";
		ama::g_builder_names[N_NUMBER] = "nNumber";
		ama::g_node_class_names[N_STRING] = "N_STRING";
		ama::g_builder_names[N_STRING] = "nString";
		ama::g_node_class_names[N_NODEOF] = "N_NODEOF";
		ama::g_builder_names[N_NODEOF] = "nNodeof";
		ama::g_node_class_names[N_SCOPE] = "N_SCOPE";
		ama::g_builder_names[N_SCOPE] = "nScope";
		ama::g_node_class_names[N_FUNCTION] = "N_FUNCTION";
		ama::g_builder_names[N_FUNCTION] = "nFunction";
		ama::g_node_class_names[N_CLASS] = "N_CLASS";
		ama::g_builder_names[N_CLASS] = "nClass";
		ama::g_node_class_names[N_POSTFIX] = "N_POSTFIX";
		ama::g_builder_names[N_POSTFIX] = "nPostfix";
		ama::g_node_class_names[N_DOT] = "N_DOT";
		ama::g_builder_names[N_DOT] = "nDot";
		ama::g_node_class_names[N_ITEM] = "N_ITEM";
		ama::g_builder_names[N_ITEM] = "nItem";
		ama::g_node_class_names[N_CALL] = "N_CALL";
		ama::g_builder_names[N_CALL] = "nCall";
		ama::g_node_class_names[N_CALL_TEMPLATE] = "N_CALL_TEMPLATE";
		ama::g_builder_names[N_CALL_TEMPLATE] = "nCallTemplate";
		ama::g_node_class_names[N_CALL_CUDA_KERNEL] = "N_CALL_CUDA_KERNEL";
		ama::g_builder_names[N_CALL_CUDA_KERNEL] = "nCallCudaKernel";
		ama::g_node_class_names[N_DEPENDENCY] = "N_DEPENDENCY";
		ama::g_builder_names[N_DEPENDENCY] = "nDependency";
		ama::g_node_class_names[N_BINOP] = "N_BINOP";
		ama::g_builder_names[N_BINOP] = "nBinop";
		ama::g_node_class_names[N_PREFIX] = "N_PREFIX";
		ama::g_builder_names[N_PREFIX] = "nPrefix";
		ama::g_node_class_names[N_ASSIGNMENT] = "N_ASSIGNMENT";
		ama::g_builder_names[N_ASSIGNMENT] = "nAssignment";
		ama::g_node_class_names[N_SCOPED_STATEMENT] = "N_SCOPED_STATEMENT";
		ama::g_builder_names[N_SCOPED_STATEMENT] = "nScopedStatement";
		ama::g_node_class_names[N_EXTENSION_CLAUSE] = "N_EXTENSION_CLAUSE";
		ama::g_builder_names[N_EXTENSION_CLAUSE] = "nExtensionClause";
		ama::g_node_class_names[N_PARAMETER_LIST] = "N_PARAMETER_LIST";
		ama::g_builder_names[N_PARAMETER_LIST] = "nParameterList";
		ama::g_node_class_names[N_CONDITIONAL] = "N_CONDITIONAL";
		ama::g_builder_names[N_CONDITIONAL] = "nConditional";
		ama::g_node_class_names[N_LABELED] = "N_LABELED";
		ama::g_builder_names[N_LABELED] = "nLabeled";
		ama::g_node_class_names[N_AIR] = "N_AIR";
		ama::g_builder_names[N_AIR] = "nAir";
		ama::g_node_class_names[N_FILE] = "N_FILE";
		ama::g_builder_names[N_FILE] = "nFile";
		ama::g_node_class_names[N_SEMICOLON] = "N_SEMICOLON";
		ama::g_builder_names[N_SEMICOLON] = "nSemicolon";
		ama::g_node_class_names[N_PAREN] = "N_PAREN";
		ama::g_builder_names[N_PAREN] = "nParen";
		ama::g_node_class_names[N_KEYWORD_STATEMENT] = "N_KEYWORD_STATEMENT";
		ama::g_builder_names[N_KEYWORD_STATEMENT] = "nKeywordStatement";
		ama::g_node_class_names[N_JS_REGEXP] = "N_JS_REGEXP";
		ama::g_builder_names[N_JS_REGEXP] = "nJsRegexp";
		JS_SetPropertyStr(ama::jsctx, global, "TMPF_IS_NODE", JS_NewInt32(ama::jsctx, TMPF_IS_NODE));
		JS_SetPropertyStr(ama::jsctx, global, "TMPF_GC_MARKED", JS_NewInt32(ama::jsctx, TMPF_GC_MARKED));
		JS_SetPropertyStr(ama::jsctx, global, "FILE_SPACE_INDENT", JS_NewInt32(ama::jsctx, FILE_SPACE_INDENT));
		JS_SetPropertyStr(ama::jsctx, global, "REF_WRITTEN", JS_NewInt32(ama::jsctx, REF_WRITTEN));
		JS_SetPropertyStr(ama::jsctx, global, "REF_RW", JS_NewInt32(ama::jsctx, REF_RW));
		JS_SetPropertyStr(ama::jsctx, global, "REF_DECLARED", JS_NewInt32(ama::jsctx, REF_DECLARED));
		JS_SetPropertyStr(ama::jsctx, global, "LITERAL_PARSED", JS_NewInt32(ama::jsctx, LITERAL_PARSED));
		JS_SetPropertyStr(ama::jsctx, global, "STRING_SINGLE_QUOTED", JS_NewInt32(ama::jsctx, STRING_SINGLE_QUOTED));
		JS_SetPropertyStr(ama::jsctx, global, "DOT_PTR", JS_NewInt32(ama::jsctx, DOT_PTR));
		JS_SetPropertyStr(ama::jsctx, global, "DOT_CLASS", JS_NewInt32(ama::jsctx, DOT_CLASS));
		JS_SetPropertyStr(ama::jsctx, global, "DEP_C_INCLUDE", JS_NewInt32(ama::jsctx, DEP_C_INCLUDE));
		JS_SetPropertyStr(ama::jsctx, global, "DEP_JS_REQUIRE", JS_NewInt32(ama::jsctx, DEP_JS_REQUIRE));
		JS_SetPropertyStr(ama::jsctx, global, "DEP_TYPE_MASK", JS_NewInt32(ama::jsctx, DEP_TYPE_MASK));
		JS_SetPropertyStr(ama::jsctx, global, "DEPF_C_INCLUDE_NONSTR", JS_NewInt32(ama::jsctx, DEPF_C_INCLUDE_NONSTR));
		JS_SetPropertyStr(ama::jsctx, global, "MAX_INDENT", JS_NewInt32(ama::jsctx, MAX_INDENT));
		JS_SetPropertyStr(ama::jsctx, global, "POS_BEFORE", JS_NewInt32(ama::jsctx, POS_BEFORE));
		JS_SetPropertyStr(ama::jsctx, global, "POS_AFTER", JS_NewInt32(ama::jsctx, POS_AFTER));
		JS_SetPropertyStr(ama::jsctx, global, "POS_FRONT", JS_NewInt32(ama::jsctx, POS_FRONT));
		JS_SetPropertyStr(ama::jsctx, global, "POS_BACK", JS_NewInt32(ama::jsctx, POS_BACK));
		JS_SetPropertyStr(ama::jsctx, global, "POS_REPLACE", JS_NewInt32(ama::jsctx, POS_REPLACE));
		JS_SetPropertyStr(ama::jsctx, global, "BOUNDARY_FUNCTION", JS_NewInt32(ama::jsctx, BOUNDARY_FUNCTION));
		JS_SetPropertyStr(ama::jsctx, global, "BOUNDARY_CLASS", JS_NewInt32(ama::jsctx, BOUNDARY_CLASS));
		JS_SetPropertyStr(ama::jsctx, global, "BOUNDARY_NODEOF", JS_NewInt32(ama::jsctx, BOUNDARY_NODEOF));
		JS_SetPropertyStr(ama::jsctx, global, "BOUNDARY_SCOPE", JS_NewInt32(ama::jsctx, BOUNDARY_SCOPE));
		JS_SetPropertyStr(ama::jsctx, global, "BOUNDARY_MATCH", JS_NewInt32(ama::jsctx, BOUNDARY_MATCH));
		JS_SetPropertyStr(ama::jsctx, global, "BOUNDARY_ONE_LEVEL", JS_NewInt32(ama::jsctx, BOUNDARY_ONE_LEVEL));
		JS_SetPropertyStr(ama::jsctx, global, "BOUNDARY_ANY", JS_NewInt32(ama::jsctx, BOUNDARY_ANY));
		JS_SetPropertyStr(ama::jsctx, global, "BOUNDARY_DEFAULT", JS_NewInt32(ama::jsctx, BOUNDARY_DEFAULT));
		JS_SetPropertyStr(ama::jsctx, global, "CFG_BASIC", JS_NewInt32(ama::jsctx, CFG_BASIC));
		JS_SetPropertyStr(ama::jsctx, global, "CFG_BRANCH", JS_NewInt32(ama::jsctx, CFG_BRANCH));
		JS_SetPropertyStr(ama::jsctx, global, "CFG_LOOP", JS_NewInt32(ama::jsctx, CFG_LOOP));
		JS_SetPropertyStr(ama::jsctx, global, "CFG_JUMP", JS_NewInt32(ama::jsctx, CFG_JUMP));
		JS_SetPropertyStr(ama::jsctx, global, "CFG_DECL", JS_NewInt32(ama::jsctx, CFG_DECL));
	}
}
#pragma gen_end(js_bindings)
