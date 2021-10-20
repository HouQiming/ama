#include <vector>
#include <unordered_map>
#include "../util/jc_array.h"
#include "../util/jc_unique_string.h"
#include "../../modules/cpp/json/json.h"
#include "../ast/node.hpp"
#include "../script/jsenv.hpp"
#include "scoping.hpp"
namespace ama {
	void ConvertToScope(ama::Node* nd_raw) {
		nd_raw->node_class = ama::N_SCOPE;
		if ( nd_raw->p ) {
			nd_raw->flags = 0;
		} else {
			nd_raw->flags &= 0xffff0000u;
		}
	}
	static std::vector<ama::Node*> MergeScopesIntoStatements(std::unordered_map<JC::unique_string, int> const& keywords_extension_clause, std::span<ama::Node*> lines_out) {
		std::vector<ama::Node*> line_group{};
		std::vector<ama::Node*> line_out_final{};
		for (int i0 = 0; i0 < lines_out.size();) {
			int i1 = i0 + 1;
			if ( !(lines_out[i0]->c && lines_out[i0]->LastChild()->isSymbol(";")) ) {
				while ( i1 < lines_out.size() && 
				(lines_out[i1]->node_class == ama::N_SCOPE || lines_out[i1]->isRawNode('{', '}') || 
				(lines_out[i1]->isRawNode(0, 0) && lines_out[i1]->c && (lines_out[i1]->c->node_class == ama::N_SCOPE || lines_out[i1]->c->isRawNode('{', '}'))) || 
				((lines_out[i1 - 1]->node_class == ama::N_SCOPE || lines_out[i1 - 1]->isRawNode('{', '}')) && 
				((lines_out[i1]->node_class == ama::N_REF && keywords_extension_clause--->get(lines_out[i1]->data)) || 
				(lines_out[i1]->node_class == ama::N_RAW && lines_out[i1]->c && lines_out[i1]->c->node_class == ama::N_REF && keywords_extension_clause--->get(lines_out[i1]->c->data))))) ) {
					if ( (lines_out[i1 - 1]->node_class == ama::N_SCOPE || lines_out[i1 - 1]->isRawNode('{', '}')) && lines_out[i1]->comments_before--->startsWith('\n') ) {
						lines_out[i1]->comments_before = JC::array_cast<JC::unique_string>(lines_out[i1]->comments_before--->subarray(1));
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
							ndj->c->comments_before = JC::array_cast<JC::unique_string>(JC::string_concat(ndj->comments_before, ndj->c->comments_before));
							ama::Node* ndj_last = ndj->LastChild();
							ndj_last->comments_after = JC::array_cast<JC::unique_string>(JC::string_concat(ndj_last->comments_after, ndj->comments_after));
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
				for ( ama::Node* const &ndj: line_group ) {
					ndj->AdjustIndentLevel(-nd_raw->indent_level);
				}
				line_out_final.push_back(nd_raw);
			}
			i0 = i1;
		}
		return std::move(line_out_final);
	}
	static void FoldIndentGroup(
	std::unordered_map<JC::unique_string, int> const& keywords_extension_clause,
	std::vector<ama::Node*>& lines_out, int32_t level, intptr_t lineno, ama::Node* nd_nextline) {
		for (intptr_t i = lineno; i < lines_out.size(); i += 1) {
			lines_out[i]->AdjustIndentLevel(-level);
		}
		ama::Node* nd_new_scope = ama::CreateNode(ama::N_SCOPE, ama::InsertMany(MergeScopesIntoStatements(keywords_extension_clause, lines_out--->subarray(lineno))));
		nd_new_scope->indent_level = level;
		if ( nd_nextline ) {
			intptr_t p_newline = nd_nextline->comments_before--->indexOf('\n');
			if ( p_newline >= 0 ) {
				if ( nd_new_scope->c ) {
					ama::Node* nd_last = nd_new_scope->LastChild();
					if ( nd_last->comments_after--->indexOf('\n') < intptr_t(0L) ) {
						nd_last->comments_after = JC::array_cast<JC::unique_string>(JC::string_concat(nd_last->comments_after, "\n"));
					}
				}
				nd_new_scope->comments_after = JC::array_cast<JC::unique_string>(JC::string_concat(nd_new_scope->comments_after, nd_nextline->comments_before--->subarray(0, p_newline)));
				nd_nextline->comments_before = JC::array_cast<JC::unique_string>(nd_nextline->comments_before--->subarray(p_newline));
			}
			//MergeCommentsAfter(nd_nextline);
			//console.log(JSON.stringify(nd_new_scope.comments_after))
		} else {
			if ( nd_new_scope->c ) {
				ama::Node* nd_last = nd_new_scope->LastChild();
				if ( nd_last->comments_after--->indexOf('\n') < intptr_t(0L) ) {
					nd_last->comments_after = JC::array_cast<JC::unique_string>(JC::string_concat(nd_last->comments_after, "\n"));
				}
			}
			nd_new_scope->comments_after = "\n";
		}
		lines_out.resize(lineno);
		lines_out.push_back(nd_new_scope);
	}
	ama::Node* DelimitCLikeStatements(ama::Node* nd_root, JSValue options) {
		std::unordered_map<JC::unique_string, int> keywords_extension_clause = ama::GetPrioritizedList(options, "keywords_extension_clause");
		{
			std::vector<ama::Node*> a = nd_root->FindAllWithin(0, ama::N_RAW, nullptr);
			for (intptr_t i = intptr_t(a.size()) - 1; i >= 0; --i) {
				{
					if ( !(a[i]->flags & 0xffff) && a[i]->p ) {
						//move out trailing ;
						//need to do it in reverse order so that we kick out ; before getting to the parent
						ama::Node* nd_last = a[i]->LastChild();
						if ( nd_last && nd_last->isSymbol(";") && a[i]->p && (a[i]->p->node_class == ama::N_SCOPE || a[i]->p->node_class == ama::N_RAW) ) {
							nd_last->comments_after = JC::array_cast<JC::unique_string>(JC::string_concat(nd_last->comments_after, a[i]->comments_after));
							nd_last->AdjustIndentLevel(a[i]->indent_level);
							a[i]->comments_after = "";
							nd_last->Unlink();
							a[i]->Insert(ama::POS_AFTER, nd_last);
						}
						continue;
					}
					std::vector<ama::Node*> new_children{};
					ama::Node* ndi = a[i]->c;
					new_children.push_back(ndi);
					char changed = 0;
					int after_cpp_macro = 0;
					while ( ndi ) {
						ama::Node* ndi_next = ndi->s;
						if ( ndi->isRawNode(0, 0) || (ndi_next && ndi_next->isRawNode(0, 0)) ) {
							//already delimited, keep it
							if ( !changed ) { changed = ','; }
							ndi->BreakSibling();
							if ( ndi_next ) {
								new_children.push_back(ndi_next);
							}
						} else if ( (ndi->isSymbol(";") && !
						(ndi_next && ndi_next->node_class == ama::N_REF && keywords_extension_clause--->get(ndi_next->data))) || 
						(ndi_next && ndi_next->isSymbol(";") && !
						(ndi_next->s && ndi_next->s->node_class == ama::N_REF && keywords_extension_clause--->get(ndi_next->s->data))) ) {
							//after_cpp_macro=0
							//separate ; into its own statement
							changed = ';';
							ndi->BreakSibling();
							if ( ndi_next ) {
								new_children.push_back(ndi_next);
							}
						} else if ( ndi->isRawNode('{', '}') || ndi->node_class == ama::N_SCOPE ) {
							if ( ndi->s && ((ndi->s->node_class == ama::N_REF && keywords_extension_clause--->get(ndi->s->data)) || ndi->s->isSymbol("=")) ) {
								ndi = ndi_next;
								continue;
							}
							//after_cpp_macro=0
							if ( !changed ) { changed = ','; }
							ndi->BreakSibling();
							if ( ndi_next ) {
								new_children.push_back(ndi_next);
							}
						} else if(ndi->node_class == ama::N_REF && ndi->data--->startsWith('#') ) {
							changed = ';';
							if ( !new_children.size() && new_children.back() == ndi ) {
								ndi->BreakSelf();
								new_children.push_back(ndi);
							}
							after_cpp_macro = 1;
						}
						if ( after_cpp_macro && ndi->s != nullptr && ndi->s->comments_before--->indexOf('\n') >= 0 ) {
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
					if ( changed != ';' ) {
						std::vector<ama::Node*> comma_children{};
						for ( ama::Node* ndj: new_children ) {
							ndi = ndj;
							comma_children.push_back(ndi);
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
					}
					if ( changed || (!a[i]->isRawNode('{', '}') && a[i]->c && a[i]->c->s) ) {
						for (int i = 0; i < new_children.size(); i += 1) {
							ama::Node* nd_inner = ama::toSingleNode(new_children[i]);
							new_children[i] = nd_inner;
						}
						a[i]->c = nullptr;
						a[i]->Insert(ama::POS_FRONT, ama::InsertMany(new_children));
					}
					//if( nd_raw.isRawNode('{', '}') && (changed == ';' || !nd_raw.c) ) {
					if ( a[i]->isRawNode('{', '}') ) {
						ConvertToScope(a[i]);
					}
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
		std::unordered_map<JC::unique_string, int> keywords_extension_clause = ama::GetPrioritizedList(options, "keywords_extension_clause");
		std::vector<ama::Node*> scopes = nd_root->FindAllWithin(0, ama::N_SCOPE, nullptr);
		for ( ama::Node* const &nd_raw: nd_root->FindAllWithin(0, ama::N_RAW, nullptr) ) {
			if ( nd_raw->isRawNode('{', '}') ) {
				scopes--->push(nd_raw);
			}
		}
		scopes.push_back(nd_root);
		for ( ama::Node* const &nd_scope: scopes ) {
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
						FoldIndentGroup(keywords_extension_clause, lines_out, istk[istk.size() - 2].level, istk.back().lineno, lines[i]);
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
				FoldIndentGroup(keywords_extension_clause, lines_out, istk[istk.size() - 2].level, istk.back().lineno, nullptr);
				istk--->pop();
			}
			//merge scopes into surrounding raws
			std::vector<ama::Node*> line_out_final = MergeScopesIntoStatements(keywords_extension_clause, lines_out);
			//replace old children
			for ( ama::Node* const &ndi: line_out_final ) {
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
