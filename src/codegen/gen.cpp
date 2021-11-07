#include <string>
#include "../util/jc_array.h"
#include "../util/unicode.hpp"
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
				this->code--->push("/* unsupported node class ", JSON::stringify(nd->node_class), " with children */ (");
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
				this->code.push_back(nd->flags & ama::PARAMLIST_TEMPLATE ? '<' : '(');
				for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
					this->Generate(ndi);
					if ( ndi->s ) {
						this->code.push_back(',');
						this->GenerateSpaceBefore(ndi->s);
					}
				}
				this->code.push_back(nd->flags & ama::PARAMLIST_TEMPLATE ? '>' : ')');
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
				if (nd->data.size() > 0 && nd->data.back() >= 'a' && nd->data.back() <= 'z') {
					//unsigned and stuff
					this->GenerateSpaceBefore(nd->c);
				}
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
				if ( nd->c && !(
					nd->c->node_class == ama::N_EXTENSION_CLAUSE || nd->c->node_class == ama::N_SCOPE || 
					nd->c->node_class == ama::N_AIR || nd->c->node_class == ama::N_PARAMETER_LIST
				) ) {
					this->GenerateSpaceBefore(nd->c);
				}
				//*** PASS THROUGH ***
			}
			case ama::N_FUNCTION: {
				//N_CLASS: before, name, after, body
				//N_FUNCTION: before, paramlist, after, body
				//N_SCOPED_STATEMENT, N_EXTENSION_CLAUSE: arg, body
				ama::Node* ndi_last = nullptr;
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
				if (nd->c->s->node_class != ama::N_AIR) {
					this->GenerateSpaceBefore(nd->c->s);
				}
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
		int end_last_indent = p_last_line;while ( end_last_indent < this->code.size() && (this->code[end_last_indent] == ' ' || this->code[end_last_indent] == '\t') ) {
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
		ama::CodeGenerator ctx{};
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
static int DumpingHook(ama::CodeGenerator* ctx, ama::Node* nd) {
	if (nd != (ama::Node*)ctx->opaque && nd->node_class == ama::N_SCOPE) {
		ctx->code--->push("{...}");
		return 1;
	}
	return 0;
}

std::string ama::Node::dump() const {
	ama::CodeGenerator ctx{};
	ctx.tab_width = 4;
	ctx.auto_space = 1;
	ctx.tab_indent = 1;
	ctx.hook = DumpingHook;
	ctx.opaque = this;
	ctx.Generate((ama::Node*)this);
	return std::move(ctx.code);
}

/////////////
struct FancyDumpContext {
	ama::Node* nd{};
	intptr_t ofs0{};
	intptr_t ofs1{};
};

static int FancyHook(ama::CodeGenerator* ctx, ama::Node* nd) {
	FancyDumpContext* fdctx = (FancyDumpContext*)ctx->opaque;
	if (fdctx->nd == nd) {
		fdctx->ofs0 = ctx->code.size();
		ctx->GenerateDefault(nd);
		fdctx->ofs1 = ctx->code.size();
		return 1;
	}
	if (nd != (ama::Node*)ctx->opaque && nd->node_class == ama::N_SCOPE) {
		ctx->code--->push("{...}");
		return 1;
	}
	return 0;
}

static int FancyHookComment(ama::CodeGenerator* ctx, const char* data, intptr_t size, int is_after, intptr_t expected_indent_level) {
	//FancyDumpContext* fdctx = (FancyDumpContext*)ctx->opaque;
	if (!ctx->code.size()) {
		//drop the comment
		return 1;
	} else {
		return 0;
	}
}

static const std::array<int32_t, 284> COMBINING{0x0300, 0x036F, 0x0483, 0x0486, 0x0488, 0x0489, 0x0591, 0x05BD, 0x05BF, 0x05BF, 0x05C1, 0x05C2, 0x05C4, 0x05C5, 0x05C7, 0x05C7, 0x0600, 0x0603, 0x0610, 0x0615, 0x064B, 0x065E, 0x0670, 0x0670, 0x06D6, 0x06E4, 0x06E7, 0x06E8, 0x06EA, 0x06ED, 0x070F, 0x070F, 0x0711, 0x0711, 0x0730, 0x074A, 0x07A6, 0x07B0, 0x07EB, 0x07F3, 0x0901, 0x0902, 0x093C, 0x093C, 0x0941, 0x0948, 0x094D, 0x094D, 0x0951, 0x0954, 0x0962, 0x0963, 0x0981, 0x0981, 0x09BC, 0x09BC, 0x09C1, 0x09C4, 0x09CD, 0x09CD, 0x09E2, 0x09E3, 0x0A01, 0x0A02, 0x0A3C, 0x0A3C, 0x0A41, 0x0A42, 0x0A47, 0x0A48, 0x0A4B, 0x0A4D, 0x0A70, 0x0A71, 0x0A81, 0x0A82, 0x0ABC, 0x0ABC, 0x0AC1, 0x0AC5, 0x0AC7, 0x0AC8, 0x0ACD, 0x0ACD, 0x0AE2, 0x0AE3, 0x0B01, 0x0B01, 0x0B3C, 0x0B3C, 0x0B3F, 0x0B3F, 0x0B41, 0x0B43, 0x0B4D, 0x0B4D, 0x0B56, 0x0B56, 0x0B82, 0x0B82, 0x0BC0, 0x0BC0, 0x0BCD, 0x0BCD, 0x0C3E, 0x0C40, 0x0C46, 0x0C48, 0x0C4A, 0x0C4D, 0x0C55, 0x0C56, 0x0CBC, 0x0CBC, 0x0CBF, 0x0CBF, 0x0CC6, 0x0CC6, 0x0CCC, 0x0CCD, 0x0CE2, 0x0CE3, 0x0D41, 0x0D43, 0x0D4D, 0x0D4D, 0x0DCA, 0x0DCA, 0x0DD2, 0x0DD4, 0x0DD6, 0x0DD6, 0x0E31, 0x0E31, 0x0E34, 0x0E3A, 0x0E47, 0x0E4E, 0x0EB1, 0x0EB1, 0x0EB4, 0x0EB9, 0x0EBB, 0x0EBC, 0x0EC8, 0x0ECD, 0x0F18, 0x0F19, 0x0F35, 0x0F35, 0x0F37, 0x0F37, 0x0F39, 0x0F39, 0x0F71, 0x0F7E, 0x0F80, 0x0F84, 0x0F86, 0x0F87, 0x0F90, 0x0F97, 0x0F99, 0x0FBC, 0x0FC6, 0x0FC6, 0x102D, 0x1030, 0x1032, 0x1032, 0x1036, 0x1037, 0x1039, 0x1039, 0x1058, 0x1059, 0x1160, 0x11FF, 0x135F, 0x135F, 0x1712, 0x1714, 0x1732, 0x1734, 0x1752, 0x1753, 0x1772, 0x1773, 0x17B4, 0x17B5, 0x17B7, 0x17BD, 0x17C6, 0x17C6, 0x17C9, 0x17D3, 0x17DD, 0x17DD, 0x180B, 0x180D, 0x18A9, 0x18A9, 0x1920, 0x1922, 0x1927, 0x1928, 0x1932, 0x1932, 0x1939, 0x193B, 0x1A17, 0x1A18, 0x1B00, 0x1B03, 0x1B34, 0x1B34, 0x1B36, 0x1B3A, 0x1B3C, 0x1B3C, 0x1B42, 0x1B42, 0x1B6B, 0x1B73, 0x1DC0, 0x1DCA, 0x1DFE, 0x1DFF, 0x200B, 0x200F, 0x202A, 0x202E, 0x2060, 0x2063, 0x206A, 0x206F, 0x20D0, 0x20EF, 0x302A, 0x302F, 0x3099, 0x309A, 0xA806, 0xA806, 0xA80B, 0xA80B, 0xA825, 0xA826, 0xFB1E, 0xFB1E, 0xFE00, 0xFE0F, 0xFE20, 0xFE23, 0xFEFF, 0xFEFF, 0xFFF9, 0xFFFB, 0x10A01, 0x10A03, 0x10A05, 0x10A06, 0x10A0C, 0x10A0F, 0x10A38, 0x10A3A, 0x10A3F, 0x10A3F, 0x1D167, 0x1D169, 0x1D173, 0x1D182, 0x1D185, 0x1D18B, 0x1D1AA, 0x1D1AD, 0x1D242, 0x1D244, 0xE0001, 0xE0001, 0xE0020, 0xE007F, 0xE0100, 0xE01EF};
static inline int bisearch(int ucs) {
	int min = 0;
	intptr_t max = (COMBINING.size() >> 1) - intptr_t(1L);
	intptr_t mid = intptr_t(0L);
	if ( ucs < COMBINING[intptr_t(0L) * intptr_t(2L) + intptr_t(0L)] || ucs > COMBINING[max * intptr_t(2L) + intptr_t(1L)] ) {
		return 0;
	}
	while ( max >= intptr_t(min) ) {
		mid = (intptr_t(min) + max) >> 1;
		if ( ucs > COMBINING[mid * intptr_t(2L) + intptr_t(1L)] ) {
			min = int(mid + intptr_t(1L));
		} else if ( ucs < COMBINING[mid * intptr_t(2L) + intptr_t(0L)] ) {
			max = mid - intptr_t(1L);
		} else {
			return 1;
		}
	}
	return 0;
}
static inline int wcwidth1(int ucs) {
	return 1 + (ucs >= 0x1100 && (ucs <= 0x115f || // Hangul Jamo init. consonants
	ucs == 0x2329 || ucs == 0x232a || (ucs >= 0x2e80 && ucs <= 0xa4cf && ucs != 0x303f) || (// CJK..Yi
	ucs >= 0xac00 && ucs <= 0xd7a3) || (// Hangul Syllables
	ucs >= 0xf900 && ucs <= 0xfaff) || (// CJK Compat Ideographs
	ucs >= 0xfe10 && ucs <= 0xfe19) || (// Vertical forms
	ucs >= 0xfe30 && ucs <= 0xfe6f) || (// CJK Compat Forms
	ucs >= 0xff00 && ucs <= 0xff60) || (// Fullwidth Forms
	ucs >= 0xffe0 && ucs <= 0xffe6) || (ucs >= 0x1F300 && ucs <= 0x2fffd) || (ucs >= 0x30000 && ucs <= 0x3fffd)));
}
// binary search
static int wcwidth(int ucs) {
	// test for 8-bit control characters
	if ( ucs == 0 ) {
		return 0;
	}
	if ( ucs < 32 || (ucs >= 0x7f && ucs < 0xa0) ) {
		return 0;
	}
	// binary search in table of non-spacing characters
	if ( bisearch(ucs) ) {
		return 0;
	}
	// if we arrive here, ucs is not a combining or C0/C1 control character
	// replace 0x20000 with 0x1F300 to classify emoji as double-width
	return wcwidth1(ucs);
}

static void FlushTilde(std::string &ret, intptr_t ofs_newline, intptr_t ofs0_tilde, intptr_t ofs1_tilde, int colored) {
	intptr_t ofs_eol = ret.size();
	unicode::TWTF8Filter filter{};
	char ch_current = ' ';
	for (intptr_t i = ofs_newline; i < ofs_eol; i++) {
		int ch = filter.NextByte(i, ret[i]);
		int w = ch >= 0 ? wcwidth(ch) : 0;
		if (i == ofs0_tilde) {
			if (ofs0_tilde == ofs1_tilde) {
				ch_current = '^';
			} else {
				ch_current = '~';
			}
			if (colored) {
				ret--->push("\033[32;1m");
			}
		} else if (i == ofs1_tilde) {
			if (colored) {
				ret--->push("\033[0m");
			}
			break;
		}
		if (w >= 1) {ret.push_back(ch_current);}
		if (w >= 2) {ret.push_back(ch_current);}
	}
	ret.push_back('\n');
}

std::string ama::Node::FormatFancyMessage(std::span<char> msg, int flags)const {
	std::string ret{};
	ama::Node* nd_root = this->Root();
	ret--->push(
		(flags & ama::MSG_COLORED) ? "\033[1m" : "",
		nd_root->data, ':', JSON::stringify(this->ComputeLineNumber() + 1),
		": ",
		(flags & (ama::MSG_WARNING + ama::MSG_COLORED)) == (ama::MSG_WARNING + ama::MSG_COLORED) ? "\033[32;1m" : "",
		(flags & ama::MSG_WARNING) ? "warning" : "note",
		(flags & (ama::MSG_WARNING + ama::MSG_COLORED)) == (ama::MSG_WARNING + ama::MSG_COLORED) ? "\033[0;1m" : "",
		": ", msg,
		(flags & ama::MSG_COLORED) ? "\033[0m" : "",
		'\n'
	);
	//citation with fancy tilde thing
	FancyDumpContext fdctx{};
	ama::CodeGenerator ctx{};
	ctx.tab_width = 4;
	ctx.auto_space = 1;
	ctx.tab_indent = 0;
	ctx.hook_comment = FancyHookComment;
	ctx.hook = FancyHook;
	ctx.opaque = &fdctx;
	fdctx.nd = (ama::Node*)this;
	ctx.Generate(((ama::Node*)this)->ParentStatement());
	intptr_t ofs_newline = ret.size();
	intptr_t ofs0_tilde = -1L;
	intptr_t ofs1_tilde = -1L;
	ret--->push("    ");
	for (intptr_t ofs = 0; ofs < ctx.code.size(); ofs++) {
		auto ch = ctx.code[ofs];
		if (ofs == fdctx.ofs0) {
			ofs0_tilde = ret.size();
		}
		if (ofs == fdctx.ofs1) {
			ofs1_tilde = ret.size();
		}
		if (ch == '\t') {
			ret--->push("    ");
		} else {
			ret.push_back(ch);
			if (ch == '\n') {
				if (ofs0_tilde >= 0L && ofs1_tilde >= 0L) {
					FlushTilde(ret, ofs_newline, ofs0_tilde, ofs1_tilde, (flags & ama::MSG_COLORED));
					ofs0_tilde = -1L;
					ofs1_tilde = -1L;
				}
				ofs_newline = ret.size();
				ret--->push("    ");
			}
		
		}
	}
	if (ret.back() != '\n') {
		ret.push_back('\n');
	}
	if (ofs0_tilde >= 0L && ofs1_tilde >= 0L) {
		FlushTilde(ret, ofs_newline, ofs0_tilde, ofs1_tilde, (flags & ama::MSG_COLORED));
	}
	return std::move(ret);
}
