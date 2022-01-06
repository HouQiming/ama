#include <string>
#include "../../modules/cpp/json/json.h"
#include "jsenv.hpp"
#include "../ast/node.hpp"
#include "jsenv.hpp"
#include "nodeof_to_ast.hpp"
namespace ama {
	static void dfsTranslate(std::string& ret, ama::Node* nd);
};
namespace ama {
	ama::Node* DefaultParseCode(char const* code);
	static void dfsTranslateList(std::string& ret, ama::Node* nd) {
		if ( nd->s ) {
			ret--->push("cons(");
		}
		dfsTranslate(ret, nd);
		if ( nd->s ) {
			ret--->push(", ");
			dfsTranslateList(ret, nd->s);
			ret--->push(')');
		}
	}
	static void dfsTranslate(std::string& ret, ama::Node* nd) {
		if ( nd->node_class == ama::N_NODEOF ) {
			for ( ama::Node* const & nd_nodeof: nd->c->FindAllWithin(ama::BOUNDARY_MATCH, ama::N_NODEOF) ) {
				std::string ret2{};
				dfsTranslateList(ret2, nd_nodeof->c->c);
				nd_nodeof->ReplaceWith(DefaultParseCode(ret2.c_str()));
			}
			ret--->push('(');
			for (ama::Node* ndi_0 = nd->c->c; ndi_0; ndi_0 = ndi_0->s) {
				{
					ret--->push(ndi_0->toSource());
				}
			}
			ret--->push(')');
		} else {
			ret--->push(ama::g_builder_names[nd->node_class], '(');
			if ( !nd->data.empty() ) {
				if ( nd->node_class == ama::N_STRING ) {
					//parse it
					nd->GetStringValue();
				}
				ret--->push(JSON::stringify(nd->data));
				if ( nd->c ) {
					ret--->push(", ");
				}
			}
			for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
				dfsTranslate(ret, ndi);
				if ( ndi->s ) {
					ret--->push(", ");
				}
			}
			ret--->push(')');
		}
		if ( nd->node_class == ama::N_STRING ) {
			if ( nd->flags != ama::LITERAL_PARSED ) {
				ret--->push(".setFlags(", JSON::stringify(nd->flags), ')');
			}
		} else {
			if ( nd->flags ) {
				ret--->push(".setFlags(", JSON::stringify(nd->flags), ')');
			}
		}
		if ( nd->comments_before.size() ) {
			ret--->push(".setCommentsBefore(", JSON::stringify(nd->comments_before), ')');
		}
		if ( nd->comments_after.size() ) {
			ret--->push(".setCommentsAfter(", JSON::stringify(nd->comments_after), ')');
		}
		if ( nd->indent_level ) {
			ret--->push(".setIndent(", JSON::stringify(nd->indent_level), ')');
		}
	}
	ama::Node* NodeofToASTExpression(ama::Node* nd_root) {
		for ( ama::Node* const & nd_nodeof: nd_root->FindAllWithin(ama::BOUNDARY_MATCH, ama::N_NODEOF) ) {
			std::string ret{};
			ama::Node* nd_tmp = nd_nodeof->c->c;
			int8_t bk_ind = nd_tmp->indent_level;
			ama::gcstring bk_cmta = nd_tmp->comments_after;
			nd_tmp->comments_after = "";
			nd_tmp->indent_level = 0;
			dfsTranslateList(ret, nd_tmp);
			nd_tmp->indent_level = bk_ind;
			nd_tmp->comments_after = bk_cmta;
			//preserve line numbers
			int comment_pushed = 0;
			for ( char const & ch: nd_nodeof->toSource() ) {
				if ( ch == '\n' ) {
					if ( !comment_pushed ) {
						comment_pushed = 1;
						ret--->push("/*");
					}
					ret--->push("\n*");
				}
			}
			if ( comment_pushed ) {
				ret--->push('/');
			}
			nd_nodeof->ReplaceWith(DefaultParseCode(ret.c_str())->setIndent(nd_nodeof->indent_level));
		}
		return nd_root;
	}
};
