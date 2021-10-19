#include <string>
#include "../util/jc_array.h"
#include "../../modules/cpp/json/json.h"
#include "../ast/node.hpp"
#include "../script/jsenv.hpp"
#include "../parser/literal.hpp"
#include "../parser/charset.hpp"
#include "gen.hpp"
namespace ama {
	void ama::CodeGenerator::GenerateSpaceBefore(ama::Node* nd_next) {
		if ( this->auto_space && !(nd_next->comments_before.size() && uint8_t(nd_next->comments_before[0]) <= uint8_t(' ')) ) {
			this->code.push_back(' ');
		}
	}
	void ama::CodeGenerator::GenerateSpaceAfter(ama::Node* nd_last) {
		if ( this->auto_space && !(nd_last->comments_after.size() && uint8_t(nd_last->comments_after[nd_last->comments_after.size() - 1]) <= uint8_t(' ')) ) {
			this->code.push_back(' ');
		}
	}
	void ama::CodeGenerator::GenerateSpaceBetween(ama::Node* nd_last, ama::Node* nd_next) {
		if ( this->auto_space && !(nd_next->comments_before.size() && uint8_t(nd_next->comments_before[0]) <= uint8_t(' ')) ) {
			this->GenerateSpaceAfter(nd_last);
		}
	}
	void ama::CodeGenerator::GenerateDefault(ama::Node* nd) {
		switch ( nd->node_class ) {
			default: {
					
					this->code--->push("/* unsupported node class "
					, JSON::stringify(nd->node_class), " */ ", JSON::stringify(nd)
					, " /* with children */ (");
				for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
					this->Generate(ndi);
					this->code--->push(' ');
				}
				this->code--->push(')');
				break;
			}
			case ama::N_FILE: case ama::N_RAW: {
				char ch_opening = char(nd->flags & 0xff);
				if ( nd->node_class == ama::N_RAW && ch_opening ) {
					this->code.push_back(ch_opening);
				}
				for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
					this->Generate(ndi);
					if ( ndi->s && (ndi->isSymbol(",") || ndi->isSymbol(";")) ) {
						this->GenerateSpaceBetween(ndi, ndi->s);
					}
				}
				char ch_closing = char(nd->flags >> 8 & 0xff);
				if ( nd->node_class == ama::N_RAW && ch_closing ) {
					this->code.push_back(ch_closing);
				}
				break;
			}
			case ama::N_SCOPE: {
				this->code.push_back('{');
				for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
					this->Generate(ndi);
				}
				this->code.push_back('}');
				break;
			}
			case ama::N_STRING: {
				if ( nd->flags & ama::LITERAL_PARSED ) {
					if ( nd->flags & ama::STRING_SINGLE_QUOTED ) {
						this->code--->push(ama::escapeJSString(nd->data));
					} else {
						this->code--->push(JSON::stringify(nd->data));
					}
					break;
				}
			}
			case ama::N_JS_REGEXP: case ama::N_SYMBOL: case ama::N_REF: case ama::N_NUMBER: {
				this->code--->push(nd->data);
				break;
			}
			case ama::N_DOT: {
				//this->RebuildOperand(nd, nd.c, ama::OPERAND_POSTFIX);
				this->Generate(nd->c);
				if ( nd->flags == ama::DOT_CLASS ) {
					this->code--->push("::");
				} else if ( nd->flags == ama::DOT_PTR ) {
					this->code--->push("->");
				} else {
					this->code--->push('.');
				}
				this->code--->push(nd->data);
				break;
			}
			case ama::N_CALL: case ama::N_CALL_TEMPLATE: case ama::N_CALL_CUDA_KERNEL: case ama::N_ITEM: {
				if ( nd->c ) {
					this->Generate(nd->c);
				} else {
					this->code--->push("/* missing callee */");
				}
				if ( nd->node_class == ama::N_CALL_CUDA_KERNEL ) {
					this->code--->push("<<<");
				} else if ( nd->node_class == ama::N_CALL_TEMPLATE ) {
					this->code.push_back('<');
				} else if ( nd->node_class == ama::N_ITEM ) {
					this->code.push_back('[');
				} else {
					//if( nd.flags & ama::CALL_IS_UNARY_OPERATOR ) {
					//	if( nd.c.s ) {
					//		this.GenerateSpaceBefore(nd.c.s);
					//	}
					//} else {
					this->code.push_back('(');
					//}
				}
				if ( nd->c ) {
					for (ama::Node* ndi = nd->c->s; ndi; ndi = ndi->s) {
						this->Generate(ndi);
						if ( ndi->s ) {
							this->code.push_back(',');
							this->GenerateSpaceBefore(ndi->s);
						}
					}
				}
				if ( nd->node_class == ama::N_CALL_CUDA_KERNEL ) {
					this->code--->push(">>>");
				} else if ( nd->node_class == ama::N_CALL_TEMPLATE ) {
					this->code.push_back('>');
				} else if ( nd->node_class == ama::N_ITEM ) {
					this->code.push_back(']');
				} else {
					//if( nd.flags & ama::CALL_IS_UNARY_OPERATOR ) {
					//	//do nothing
					//} else {
					this->code.push_back(')');
					//}
				}
				break;
			}
			case ama::N_PARAMETER_LIST: {
				this->code.push_back('(');
				for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
					this->Generate(ndi);
					if ( ndi->s ) {
						this->code.push_back(',');
						this->GenerateSpaceBefore(ndi->s);
					}
				}
				this->code.push_back(')');
				break;
			}
			//case ama::N_RAW_DECLARATION: {
			//	for(ama::Node*+! ndi : nd) {
			//		this->Generate(ndi);
			//	}
			//	break;
			//}
			case ama::N_DEPENDENCY: {
				if ( (nd->flags & ama::DEP_TYPE_MASK) == ama::DEP_C_INCLUDE ) {
					this->code--->push("#include");
					this->GenerateSpaceBefore(nd->c);
					if ( nd->flags & ama::DEPF_C_INCLUDE_NONSTR ) {
						this->GenerateComment(0, this->scope_indent_level + intptr_t(nd->c->indent_level), nd->c->comments_before);
						this->code--->push(nd->c->GetStringValue());
						this->GenerateComment(1, this->scope_indent_level + (nd->s ? intptr_t(nd->s->indent_level) : intptr_t(0L)), nd->c->comments_after);
					} else {
						this->Generate(nd->c);
					}
					if ( this->auto_space && nd->comments_after--->indexOf('\n') < 0 && !(nd->s && nd->s->comments_before--->indexOf('\n') >= 0) ) {
						this->code--->push('\n');
					}
				} else if ( (nd->flags & ama::DEP_TYPE_MASK) == ama::DEP_JS_REQUIRE ) {
					this->code--->push("require(");
					this->Generate(nd->c);
					this->code--->push(')');
				}
				break;
			}
			case ama::N_BINOP: {
				this->Generate(nd->c);
				this->GenerateSpaceAfter(nd->c);
				this->code--->push(nd->data);
				this->GenerateSpaceBefore(nd->c->s);
				this->Generate(nd->c->s);
				break;
			}
			case ama::N_POSTFIX: {
				this->Generate(nd->c);
				this->code--->push(nd->data);
				break;
			}
			case ama::N_PREFIX: {
				this->code--->push(nd->data);
				this->Generate(nd->c);
				break;
			}
			case ama::N_ASSIGNMENT: {
				this->Generate(nd->c);
				if ( nd->c->s->node_class == ama::N_AIR ) {
					//do nothing
				} else {
					this->GenerateSpaceAfter(nd->c);
					this->code--->push(nd->data);
					this->code--->push('=');
					this->GenerateSpaceBefore(nd->c->s);
					this->Generate(nd->c->s);
				}
				break;
			}
			case ama::N_CLASS: case ama::N_KEYWORD_STATEMENT: case ama::N_SCOPED_STATEMENT: case ama::N_EXTENSION_CLAUSE: {
				this->code--->push(nd->data);
				if ( nd->c && !(nd->c->node_class == ama::N_EXTENSION_CLAUSE || nd->c->node_class == ama::N_SCOPE || nd->c->node_class == ama::N_AIR) ) {
					this->GenerateSpaceBefore(nd->c);
				}
				//*** PASS THROUGH ***
			}
			case ama::N_FUNCTION: {
				//N_CLASS: before, name, after, body
				//N_FUNCTION: before, paramlist, after, body
				//N_SCOPED_STATEMENT, N_EXTENSION_CLAUSE: arg, body
				ama::Node* ndi_last{};
				for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
					if ( ndi->node_class == ama::N_EXTENSION_CLAUSE || ndi->node_class == ama::N_SCOPE ) {
						if ( ndi_last ) {
							this->GenerateSpaceBetween(ndi_last, ndi);
						} else {
							this->GenerateSpaceBefore(ndi);
						}
					}
					this->Generate(ndi);
					if ( ndi->node_class != ama::N_AIR ) {
						ndi_last = ndi;
					}
				}
				break;
			}
			case ama::N_NODEOF: {
				this->code--->push('.');
				this->Generate(nd->c);
				break;
			}
			case ama::N_CONDITIONAL: {
				this->Generate(nd->c);
				this->GenerateSpaceAfter(nd->c);
				this->code--->push('?');
				this->GenerateSpaceBefore(nd->c->s);
				this->Generate(nd->c->s);
				this->GenerateSpaceAfter(nd->c->s);
				this->code--->push(':');
				this->GenerateSpaceBefore(nd->c->s->s);
				this->Generate(nd->c->s->s);
				break;
			}
			case ama::N_LABELED: {
				this->Generate(nd->c);
				this->code--->push(':');
				this->GenerateSpaceBefore(nd->c->s);
				this->Generate(nd->c->s);
				break;
			}
			case ama::N_AIR: {
				//nothing
				break;
			}
			case ama::N_SEMICOLON: {
				this->Generate(nd->c);
				this->code--->push(';');
				break;
			}
			case ama::N_PAREN: {
				this->code--->push('(');
				this->Generate(nd->c);
				this->code--->push(')');
				break;
			}
		}
		//case N_ITEM:
	}
	void ama::CodeGenerator::GenerateIndent(intptr_t expected_indent_level) {
		intptr_t current_indent_level = intptr_t(0L);
		if ( current_indent_level >= expected_indent_level ) {
			this->p_last_indent = this->code--->lastIndexOf('\n') + intptr_t(1L);
			return;
		}
		intptr_t p_last_line = this->p_last_indent;
		int end_last_indent = p_last_line;
		while ( end_last_indent < this->code.size() && (this->code[end_last_indent] == ' ' || this->code[end_last_indent] == '\t') ) {
			end_last_indent += 1;
		}
		while ( current_indent_level < expected_indent_level ) {
			if ( (current_indent_level % this->tab_width) == 0 && (current_indent_level + this->tab_width) <= expected_indent_level && !(p_last_line < end_last_indent && this->code[p_last_line] == ' ') && this->tab_indent ) {
				if ( p_last_line < end_last_indent ) {
					assert(this->code[p_last_line] == '\t');
					p_last_line += 1;
				}
				this->code.push_back('\t');
				current_indent_level /= this->tab_width;
				current_indent_level += 1;
				current_indent_level *= this->tab_width;
			} else {
				if ( p_last_line < end_last_indent ) {
					p_last_line += 1;
				}
				this->code.push_back(' ');
				current_indent_level += intptr_t(1L);
			}
		}
		this->p_last_indent = this->code--->lastIndexOf('\n') + intptr_t(1L);
	}
	void ama::CodeGenerator::GenerateCommentDefault(int is_after, intptr_t expected_indent_level, std::span<char> comment) {
		//if( comment.indexOf('\n') < 0 ) {
		//	this.code.push(comment);
		//	return;
		//}
		for (intptr_t i = 0; i < comment.size(); i++) {
			char ch = comment[i];
			if ( ch == '\n' ) {
				this->code--->push(ch);
				//unify in-comment indent
				intptr_t indent_level = expected_indent_level;
				i += 1;
				while ( i < comment.size() ) {
					if ( comment[i] == ' ' ) {
						indent_level += 1;
					} else if ( comment[i] == '\t' ) {
						indent_level /= this->tab_width;
						indent_level += 1;
						indent_level *= this->tab_width;
					} else {
						break;
					}
					i += 1;
				}
				i -= 1;
				this->GenerateIndent(indent_level);
				continue;
			}
			this->code--->push(ch);
		}
	}
	void ama::CodeGenerator::GenerateComment(int is_after, intptr_t expected_indent_level, std::span<char> comment) {
		if ( this->hook_comment != nullptr ) {
			if ( this->hook_comment(this, comment.data(), comment.size(), is_after, expected_indent_level) != 0 ) { return; }
		}
		this->GenerateCommentDefault(is_after, expected_indent_level, comment);
	}
	void ama::CodeGenerator::Generate(ama::Node* nd) {
		if ( !nd ) {
			this->code--->push("/*NULL*/");
			return;
		}
		ama::Node* bk = this->nd_current;
		this->nd_current = nd;
		this->scope_indent_level += intptr_t(nd->indent_level);
		this->GenerateComment(0, this->scope_indent_level, nd->comments_before);
		if ( this->hook != nullptr && this->hook(this, nd) != 0 ) {
			//the hook generated it
		} else {
			this->GenerateDefault(nd);
		}
		//if( nd.p && (nd.p.node_class == ama::N_SCOPE || nd.p.node_class == ama::N_FILE) && 
		//nd.NeedTrailingSemicolon() ) {
		//	this->code.push_back(';');
		//}
		//the "normal" content of a node should not contain newline
		this->scope_indent_level -= intptr_t(nd->indent_level);
		this->GenerateComment(1, this->scope_indent_level + (nd->s ? intptr_t(nd->s->indent_level) : intptr_t(0L)), nd->comments_after);
		this->nd_current = bk;
	}
	//this is exposed to JS with a non-standard wrapper, so make it a non-method
	std::string GenerateCode(ama::Node* nd, JSValueConst options) {
		options = ama::InheritOptions(options);
		ama::CodeGenerator ctx{.code = {}, .nd_current = nullptr, .opaque = nullptr, .hook = nullptr, .hook_comment = nullptr, .scope_indent_level = intptr_t(0L), .p_last_indent = intptr_t(0L), .tab_width = 4, .auto_space = 1};
		ctx.tab_width = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "tab_width"), 4);
		ctx.auto_space = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "auto_space"), 1);
		ctx.tab_indent = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "tab_indent"), 2);
		if ( ctx.tab_indent == 2 ) {
			ama::Node* nd_root = nd->Root();
			if ( nd_root->node_class == ama::N_FILE && (nd_root->flags & ama::FILE_SPACE_INDENT) ) {
				ctx.tab_indent = 0;
			}
		}
		ctx.Generate(nd);
		return std::move(ctx.code);
	}
	std::string ama::Node::toSource() const {
		return GenerateCode((ama::Node*)(this), JS_NULL);
	}
};
namespace JSON {
	template<>
	struct StringifyToImpl<ama::Node> {
		typedef void type;
		template<typename T = ama::Node>
		static void stringifyTo(std::string& buf, ama::Node const& a) {
			buf += "{";
			buf += "\"node_class\":";
			JSON::stringifyTo(buf, a.node_class);
			buf += ",\"indent_level\":";
			JSON::stringifyTo(buf, a.indent_level);
			buf += ",\"tmp_flags\":";
			JSON::stringifyTo(buf, a.tmp_flags);
			buf += ",\"flags\":";
			JSON::stringifyTo(buf, a.flags);
			buf += ",\"data\":";
			JSON::stringifyTo(buf, a.data);
			buf += ",\"comments_before\":";
			JSON::stringifyTo(buf, a.comments_before);
			buf += ",\"comments_after\":";
			JSON::stringifyTo(buf, a.comments_after);
			buf += "}";
		}
	};
};
