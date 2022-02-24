#include <vector>
#include <unordered_map>
#include "../util/jc_array.h"
#include "../../modules/cpp/json/json.h"
#include "../ast/node.hpp"
#include "../script/jsenv.hpp"
#include "scoping.hpp"
namespace ama {
	void ConvertToScope(ama::Node* nd_raw) {
		if (nd_raw->node_class == ama::N_SCOPE) {return;}
		nd_raw->node_class = ama::N_SCOPE;
		if ( nd_raw->p ) {
			nd_raw->flags = 0;
		} else {
			//it will end up as N_FILE instead
			nd_raw->flags &= 0xffff0000u;
		}
	}
	static void ConvertToOA(ama::Node* nd_raw, uint8_t new_node_class) {
		if (nd_raw->node_class != ama::N_RAW) {return;}
		nd_raw->node_class = new_node_class;
		nd_raw->flags = 0;
	}
	static std::vector<ama::Node*> MergeScopesIntoStatements(std::unordered_map<ama::gcstring, int> const& keywords_extension_clause, int32_t but_merge_cpp_ctor_lines, std::span<ama::Node*> lines_out) {
		std::vector<ama::Node*> line_group{};
		std::vector<ama::Node*> line_out_final{};
		int could_be_in_ctor = 0;
		for (int i0 = 0; i0 < lines_out.size();) {
			int i1 = i0 + 1;
			if ( !(lines_out[i0]->c && lines_out[i0]->LastChild()->isSymbol(";")) ) {
				while ( i1 < lines_out.size() && 
				(lines_out[i1]->node_class == ama::N_SCOPE || lines_out[i1]->isRawNode('{', '}') || 
				(lines_out[i1]->isRawNode(0, 0) && lines_out[i1]->c && (lines_out[i1]->c->node_class == ama::N_SCOPE || lines_out[i1]->c->isRawNode('{', '}'))) || 
				((lines_out[i1 - 1]->node_class == ama::N_SCOPE || lines_out[i1 - 1]->isRawNode('{', '}')) && 
				((lines_out[i1]->node_class == ama::N_REF && keywords_extension_clause--->get(lines_out[i1]->data)) || 
				(lines_out[i1]->node_class == ama::N_RAW && lines_out[i1]->c && lines_out[i1]->c->node_class == ama::N_REF && keywords_extension_clause--->get(lines_out[i1]->c->data)))) ||
				but_merge_cpp_ctor_lines && (lines_out[i1 - 1]->node_class == ama::N_RAW && lines_out[i1 - 1]->c && lines_out[i1 - 1]->LastChild()->isSymbol(":")) ||
				but_merge_cpp_ctor_lines && could_be_in_ctor && (lines_out[i1 - 1]->node_class == ama::N_RAW && lines_out[i1 - 1]->c && lines_out[i1 - 1]->LastChild()->isSymbol(","))) ||
				but_merge_cpp_ctor_lines && (lines_out[i1 - 1]->node_class == ama::N_RAW && lines_out[i1 - 1]->c && lines_out[i1 - 1]->c->isRef("template") && (lines_out[i1 - 1]->LastChild()->isSymbol(">") || lines_out[i1 - 1]->LastChild()->isSymbol(">>")))
				) {
					if ( (lines_out[i1 - 1]->node_class == ama::N_SCOPE || lines_out[i1 - 1]->isRawNode('{', '}')) && lines_out[i1]->comments_before--->startsWith('\n') ) {
						lines_out[i1]->comments_before = ama::gcstring(lines_out[i1]->comments_before--->subarray(1));
					}
					if (but_merge_cpp_ctor_lines && (lines_out[i1 - 1]->node_class == ama::N_RAW && lines_out[i1 - 1]->c && lines_out[i1 - 1]->LastChild()->isSymbol(":"))) {
						could_be_in_ctor = 1;
					}
					i1 += 1;
				}
			}
			if ( (i1 - i0) <= 1 ) {
				line_out_final.push_back(lines_out[i0]);
			} else {
				line_group.clear();
				for (int j = i0; j < i1; j += 1) {
					ama::Node* ndj = lines_out[j];
					if ( ndj->isRawNode(0, 0) ) {
						if ( ndj->c ) {
							ndj->c->comments_before = (ndj->comments_before + ndj->c->comments_before);
							ama::Node* ndj_last = ndj->LastChild();
							ndj_last->comments_after = (ndj_last->comments_after + ndj->comments_after);
						}
						for (ama::Node* ndi = ndj->c; ndi; ndi = ndi->s) {
							{
								ndi->AdjustIndentLevel(ndj->indent_level);
								line_group.push_back(ndi);
							}
						}
					} else {
						line_group.push_back(ndj);
					}
				}
				assert(line_group.size() > 1);
				ama::Node* nd_raw = ama::CreateNodeFromChildren(ama::N_RAW, line_group);
				nd_raw->indent_level = line_group[0]->indent_level;
				for ( ama::Node * ndj: line_group ) {
					ndj->AdjustIndentLevel(-nd_raw->indent_level);
				}
				line_out_final.push_back(nd_raw);
			}
			i0 = i1;
		}
		return std::move(line_out_final);
	}
	static void FoldIndentGroup(
	std::unordered_map<ama::gcstring, int> const& keywords_extension_clause,
	std::vector<ama::Node*>& lines_out, int32_t level, intptr_t lineno, int32_t auto_curly_bracket, 
	int32_t but_merge_cpp_ctor_lines, ama::Node* nd_nextline) {
		for (intptr_t i = lineno; i < intptr_t(lines_out.size()); i += 1) {
			lines_out[i]->AdjustIndentLevel(-level);
		}
		ama::Node* nd_new_scope = ama::CreateNode(ama::N_SCOPE, ama::InsertMany(MergeScopesIntoStatements(keywords_extension_clause, but_merge_cpp_ctor_lines, lines_out--->subarray(lineno))));
		if (!auto_curly_bracket) {
			nd_new_scope->flags = ama::SCOPE_FROM_INDENT;
		}
		nd_new_scope->indent_level = level;
		if ( nd_nextline ) {
			intptr_t p_newline = nd_nextline->comments_before--->indexOf('\n');
			if ( p_newline >= 0 && auto_curly_bracket) {
				if ( nd_new_scope->c ) {
					ama::Node* nd_last = nd_new_scope->LastChild();
					if ( nd_last->comments_after--->indexOf('\n') < intptr_t(0L) ) {
						nd_last->comments_after = (ama::gcscat(nd_last->comments_after, "\n"));
					}
				}
				nd_new_scope->comments_after = (ama::gcscat(nd_new_scope->comments_after, nd_nextline->comments_before--->subarray(0, p_newline)));
				nd_nextline->comments_before = ama::gcstring(nd_nextline->comments_before--->subarray(p_newline));
			}
			//MergeCommentsAfter(nd_nextline);
			//console.log(JSON.stringify(nd_new_scope.comments_after))
		} else {
			if ( nd_new_scope->c ) {
				ama::Node* nd_last = nd_new_scope->LastChild();
				if ( nd_last->comments_after--->indexOf('\n') < intptr_t(0L) ) {
					nd_last->comments_after = (ama::gcscat(nd_last->comments_after, "\n"));
				}
			}
			nd_new_scope->comments_after = "\n";
		}
		lines_out.resize(lineno);
		lines_out.push_back(nd_new_scope);
	}
	//before DelimitCLikeStatements
	static bool hasTrailingNewline(std::span<char> s) {
		for (intptr_t i = s.size() - 1; i >= 0; i--) {
			if (s[i] == '\n') {return 1;}
			if (uint8_t(s[i]) > uint8_t(' ')) {break;}
		}
		return 0;
	}
	static bool hasLeadingNewline(std::span<char> s) {
		for (intptr_t i = 0; i < s.size(); i++) {
			if (s[i] == '\n') {return 1;}
			if (uint8_t(s[i]) > uint8_t(' ')) {
				//treat // as newline
				if (s--->subarray(i)--->startsWith("//")) {return 1;}
				break;
			}
		}
		return 0;
	}
	ama::Node* InsertJSSemicolons(ama::Node* nd_root) {
		//just insert ';', leave the original RAW structure untouched
		for (ama::Node* nd_raw: nd_root->FindAllWithin(0, ama::N_RAW)) {
			//only in scopes / statements
			if (!(nd_raw->isRawNode(0, 0) || nd_raw->isRawNode('{', '}'))) { continue;}
			ama::Node* nd_last = nullptr;
			for (ama::Node* ndi = nd_raw->c; ndi; ndi = ndi->s) {
				nd_last = ndi->Prev();
				//search for newline in-between
				if (!(nd_last && (hasTrailingNewline(nd_last->comments_after) || hasLeadingNewline(ndi->comments_before)))) { continue;}
				//allow operators to cross line boundary
				if (nd_last->node_class == ama::N_SYMBOL && nd_last->data != "--" && nd_last->data != "++" ||
				ndi->node_class == ama::N_SYMBOL && ndi->data != "--" && ndi->data != "++") {
					continue;
				}
				//never after <> / {}
				if (nd_last->isRawNode('<', '>') ) {continue;}
				if (nd_last->isRawNode('{', '}')) {
					if (ndi->node_class != ama::N_REF) {continue;}
					bool is_scope = false;
					bool has_comma = false;
					for (ama::Node* ndj = nd_last->c; ndj; ndj = ndj->s) {
						if (ndj->isSymbol(";")) {
							is_scope = true;
							break;
						} else if (ndj->isSymbol(",")) {
							has_comma = true;
						}
					}
					if (is_scope || !has_comma) {continue;}
				}
				//allow () [] {} on the next line
				if (ndi->isRawNode('(', ')') || ndi->isRawNode('[', ']') || ndi->isRawNode('{', '}')) {continue;}
				//certain keywords are valid at eol
				if (nd_last->isRef("function") || nd_last->isRef("class") || nd_last->isRef("async") || 
				nd_last->isRef("await") || nd_last->isRef("case") || nd_last->isRef("else") ||
				nd_last->isRef("try") || nd_last->isRef("import")) {
					continue;
				}
				//insert ';'
				ndi->Insert(ama::POS_BEFORE, ama::nSymbol(";"));
			}
		}
		return nd_root;
	}
	ama::Node* DelimitCLikeStatements(ama::Node* nd_root, JSValue options) {
		std::unordered_map<ama::gcstring, int> keywords_extension_clause = ama::GetPrioritizedList(options, "keywords_extension_clause");
		std::unordered_map<ama::gcstring, int> named_binary_operators = ama::GetPrioritizedList(options, "binary_operators");
		std::unordered_map<ama::gcstring, int> named_operators = ama::GetPrioritizedList(options, "named_operators");
		for (auto &it: named_binary_operators) {
			if (!named_operators--->get(it.first)) {
				it.second = 0;
			}
		}
		std::vector<ama::Node*> all_raws = nd_root->FindAllWithin(0, ama::N_RAW);
		for (intptr_t i = intptr_t(all_raws.size()) - 1; i >= 0; --i) {
			ama::Node* nd_raw = all_raws[i];
			if ( !(nd_raw->flags & 0xffff) && nd_raw->p ) {
				//move out trailing ;
				//need to do it in reverse order so that we kick out ; before getting to the parent
				ama::Node* nd_last = nd_raw->LastChild();
				if ( nd_last && nd_last->isSymbol(";") && nd_raw->p && (nd_raw->p->node_class == ama::N_SCOPE || nd_raw->p->node_class == ama::N_RAW) ) {
					nd_last->comments_after = (nd_last->comments_after + nd_raw->comments_after);
					nd_last->AdjustIndentLevel(nd_raw->indent_level);
					nd_raw->comments_after = "";
					nd_last->Unlink();
					nd_raw->Insert(ama::POS_AFTER, nd_last);
				}
				continue;
			}
			if (nd_raw->isRawNode('`', '`')) {continue;}
			std::vector<ama::Node*> new_children{};
			ama::Node* ndi = nd_raw->c;
			if (ndi) {new_children.push_back(ndi);}
			char changed = 0;
			bool has_semicolon = false;
			int after_cpp_macro = 0;
			while ( ndi ) {
				ama::Node* ndi_next = ndi->s;
				if (ndi->isSymbol(";")) {has_semicolon = true;}
				if ( ndi->isRawNode(0, 0) || (ndi_next && ndi_next->isRawNode(0, 0)) ) {
					//already delimited, keep it
					if ( !changed ) { changed = ','; }
					ndi->BreakSibling();
					if ( ndi_next ) {
						new_children.push_back(ndi_next);
					}
				} else if ( (ndi->isSymbol(";") && !(ndi_next && ndi_next->node_class == ama::N_REF && keywords_extension_clause--->get(ndi_next->data))) || 
				(ndi_next && ndi_next->isSymbol(";") && !(ndi_next->s && ndi_next->s->node_class == ama::N_REF && keywords_extension_clause--->get(ndi_next->s->data))) ) {
					//after_cpp_macro=0
					//separate ; into its own statement
					changed = ';';
					ndi->BreakSibling();
					if ( ndi_next ) {
						new_children.push_back(ndi_next);
					}
				} else if ( ndi->isRawNode('{', '}') || ndi->node_class == ama::N_SCOPE ) {
					if ( ndi->s && ((ndi->s->node_class == ama::N_REF && (keywords_extension_clause--->get(ndi->s->data) || named_binary_operators--->get(ndi->s->data, 0))) || 
					ndi->s->isSymbol("=") || ndi->s->isSymbol(",") || ndi->s->isSymbol(":") || ndi->s->isRawNode('(', ')') || ndi->s->isSymbol(".")) ) {
						ndi = ndi_next;
						continue;
					}
					//after_cpp_macro=0
					if ( !changed ) { changed = ','; }
					ndi->BreakSibling();
					if ( ndi_next ) {
						new_children.push_back(ndi_next);
					}
				} else if (ndi->node_class == ama::N_REF && ndi->data--->startsWith('#')) {
					changed = ';';
					if ( !new_children.size() && new_children.back() == ndi ) {
						ndi->BreakSelf();
						new_children.push_back(ndi);
					}
					after_cpp_macro = 1;
				}
				if ( after_cpp_macro && (ndi->comments_after--->indexOf('\n') >= 0 || ndi->s != nullptr && ndi->s->comments_before--->indexOf('\n') >= 0) ) {
					after_cpp_macro = 0;
					changed = ';';
					ndi->BreakSibling();
					if ( ndi_next ) {
						new_children.push_back(ndi_next);
					}
				}
				//COULDDO: case:, default:
				ndi = ndi_next;
			}
			//no , delimiting in root
			if ( !has_semicolon && changed != ';' && nd_raw->p ) {
				std::vector<ama::Node*> comma_children{};
				changed = ' ';
				for ( ama::Node * ndj: new_children ) {
					ndi = ndj;
					comma_children.push_back(ndi);
					if (ndi->node_class == ama::N_RAW && ndi->c && ndi->LastChild()->isSymbol(",")) {changed = ',';}
					while ( ndi ) {
						ama::Node* ndi_next = ndi->s;
						if ( ndi->isSymbol(",") ) {
							//after_cpp_macro=0
							changed = ',';
							ndi->BreakSibling();
							ndi->BreakSelf();
							if ( ndi != comma_children.back() ) {
								comma_children.push_back(ndi);
							}
							if ( ndi_next ) {
								comma_children.push_back(ndi_next);
							}
						}
						ndi = ndi_next;
					}
				}
				std::swap(new_children, comma_children);
				if ( changed == ',' && nd_raw->isRawNode('{', '}') ) {
					ConvertToOA(nd_raw, ama::N_OBJECT);
				} else if ( nd_raw->isRawNode('[', ']') ) {
					ConvertToOA(nd_raw, ama::N_ARRAY);
				}
			}
			if ( changed || (!nd_raw->isRawNode('{', '}') && nd_raw->c && nd_raw->c->s) ) {
				for (int i = 0; i < new_children.size(); i += 1) {
					ama::Node* nd_inner = ama::toSingleNode(new_children[i]);
					new_children[i] = nd_inner;
				}
				nd_raw->c = nullptr;
				if (new_children.size()) {
					nd_raw->Insert(ama::POS_FRONT, ama::InsertMany(new_children));
				}
			}
			//if( nd_raw.isRawNode('{', '}') && (changed == ';' || !nd_raw.c) ) {
			if ( nd_raw->isRawNode('{', '}') ) {
				ConvertToScope(nd_raw);
				if (!changed && nd_raw->c) {
					ama::Node* nd_child = nd_raw->BreakChild();
					nd_raw->Insert(ama::POS_BACK, nd_child->toSingleNode());
				}
			}
		}
		return nd_root;
	}
	//do this before DelimitCLikeStatements
	struct IndentStackItem {
		int32_t level{};
		int32_t lineno{};
	};
	ama::Node* ConvertIndentToScope(ama::Node* nd_root, JSValue options) {
		std::unordered_map<ama::gcstring, int> keywords_extension_clause = ama::GetPrioritizedList(options, "keywords_extension_clause");
		int32_t auto_curly_bracket = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "auto_curly_bracket"), 0);
		int32_t but_merge_cpp_ctor_lines = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_indent_as_scope_but_merge_cpp_ctor_lines"), 0);
		std::vector<ama::Node*> scopes = nd_root->FindAllWithin(0, ama::N_SCOPE);
		for ( ama::Node * nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
			if ( nd_raw->isRawNode('{', '}') ) {
				scopes--->push(nd_raw);
			}
		}
		scopes.push_back(nd_root);
		for ( ama::Node * nd_scope: scopes ) {
			if (nd_scope->p && nd_scope->p->isRawNode('(', ')')) {
				ama::Node* nd_prev = nd_scope->Prev();
				if (!nd_prev || nd_prev->isSymbol(",")) {
					//likely C++ array const, ignore
					continue;
				}
			}
			std::vector<ama::Node*> lines{};
			int passed_newline = 1;
			lines.push_back(nd_scope->c);
			//delimit first
			for (ama::Node* ndi = nd_scope->c; ndi; ndi = ndi->s) {
				if ( ndi->comments_before--->indexOf('\n') >= 0 ) {
					passed_newline = 1;
				}
				if ( passed_newline ) {
					if ( ndi != lines.back() ) {
						lines.push_back(ndi->BreakSelf());
					}
					passed_newline = 0;
				}
				if (ndi->s && (ndi->s->comments_before--->startsWith("\n ") || ndi->s->comments_before--->startsWith("\n\t"))) {
					//last comment in an indent group, it should have been our comment
					ndi->MergeCommentsAfter(ndi->s);
					//if (ndi->comments_after--->endsWith('\n')) {
					//ndi->comments_after = ndi->comments_after--->subarray(0, ndi->comments_after.size() - 1);
					//leave them a newline for sanity
					ndi->s->comments_before = "\n";
					//}
				}
				if ( ndi->comments_after--->indexOf('\n') >= 0 ) {
					passed_newline = 1;
				}
			}
			if ( lines.size() <= 1 ) { continue; }
			for (int i = 0; i < lines.size(); i += 1) {
				lines[i] = ama::toSingleNode(lines[i]);
				//console.log(i, lines[i].toSource());
			}
			//check indent levels after
			std::vector<IndentStackItem> istk{};
			istk--->push(IndentStackItem{.level = lines[0]->indent_level, .lineno = 0});
			std::vector<ama::Node*> lines_out{};
			for (int i = 0; i < lines.size(); i += 1) {
				while ( lines[i]->indent_level < istk.back().level ) {
					if ( istk.size() <= 1 || lines[i]->indent_level > istk[istk.size() - 2].level ) {
						//bad indent
						istk.back().level = lines[i]->indent_level;
					} else {
						FoldIndentGroup(keywords_extension_clause, lines_out, istk[istk.size() - 2].level, istk.back().lineno, auto_curly_bracket, but_merge_cpp_ctor_lines, lines[i]);
						istk--->pop();
					}
				}
				if ( lines[i]->indent_level > istk.back().level ) {
					istk--->push(IndentStackItem{.level = lines[i]->indent_level, .lineno = int32_t(lines_out.size())});
				}
				assert(!lines[i]->s);
				lines_out.push_back(lines[i]);
			}
			while ( istk.size() > 1 ) {
				FoldIndentGroup(keywords_extension_clause, lines_out, istk[istk.size() - 2].level, istk.back().lineno, auto_curly_bracket, but_merge_cpp_ctor_lines, nullptr);
				istk--->pop();
			}
			//merge scopes into surrounding raws
			std::vector<ama::Node*> line_out_final = MergeScopesIntoStatements(keywords_extension_clause, but_merge_cpp_ctor_lines, lines_out);
			//replace old children
			for ( ama::Node * ndi: line_out_final ) {
				ndi->p = nd_scope;
				//if( ndi.s ) {
				//	console.log('has s', ndi.toSource());
				//}
				assert(!ndi->s);
			}
			nd_scope->c = nullptr;
			nd_scope->Insert(ama::POS_FRONT, ama::InsertMany(line_out_final));
		}
		//flag the root as a scope to prevent DelimitCLikeStatements from merging its children like it were a raw statement
		//the flag will be reset when converting to N_FILE
		nd_root->flags = (nd_root->flags & 0xffff0000u) | uint32_t('{') | uint32_t('}') << 8;
		return nd_root;
	}
};
