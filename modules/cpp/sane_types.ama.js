'use strict';
let g_templates = {
	array: {from: @(@(Node.MatchAny('TElement'))[]), to: @(std::vector<@(Node.MatchAny('TElement'))>)},
	view: {from: @(@(Node.MatchAny('TElement'))[:]), to: @(std::span<@(Node.MatchAny('TElement'))>)},
	fixed_array: {from: nPostfix(@(@(Node.MatchAny('TElement'))[@(Node.MatchAny('size'))]), '!'), to: @(std::array<@(Node.MatchAny('TElement')), @(Node.MatchAny('size'))>)},
	map: {from: @(Map<@(Node.MatchAny('TKey')), @(Node.MatchAny('TValue'))>), to: @(std::unordered_map<@(Node.MatchAny('TKey')), @(Node.MatchAny('TValue'))>)},
	unique_ptr: {from: @(@(Node.MatchAny('TElement'))^), to: @(std::unique_ptr<@(Node.MatchAny('TElement'))>)},
};

/*
#filter Mark `[]` as a type suffix, a required setup step for `require("sane_types")`
The filter itself has no visible effect and must be used before `require("sane_types")`.
*/
function FixArrayTypes(nd_root) {
	for (let nd_mul of nd_root.FindAll(N_BINOP)) {
		if (nd_mul.data == '*' || nd_mul.data == '^') {
			//[]
			let nd_postfix_core = nd_mul.c.s;
			while (nd_postfix_core.node_class == N_POSTFIX) {
				nd_postfix_core = nd_postfix_core.c;
			}
			if (nd_postfix_core.isRawNode('[', ']') || nd_postfix_core.node_class == N_ARRAY) {
				let nd_their_operand = nd_mul.c.BreakSibling();
				let nd_subscripts = nd_postfix_core.c;
				let nd_item = nItem(nPostfix(nd_mul.BreakChild(), nd_mul.data));
				if (nd_subscripts) {
					nd_item.Insert(POS_BACK, nd_subscripts);
					for (let ndi = nd_subscripts; ndi; ndi = ndi.s) {
						while (ndi.node_class == N_SEMICOLON) {
							ndi = ndi.ReplaceWith(ndi.BreakChild());
						}
					}
				}
				if (nd_their_operand.node_class == N_POSTFIX) {
					nd_postfix_core.ReplaceWith(nd_item);
					nd_item = nd_their_operand;
				}
				nd_mul.ReplaceWith(nd_item);
			}
		}
	}
}

function BidirTransform(nd_root, alt_templates, is_forward) {
	let templates = g_templates;
	if (alt_templates) {
		templates = Object.create(g_templates);
		for (let key in alt_templates) {
			if (typeof(alt_templates[key]) != 'object') {continue;}
			//if (!((alt_templates[key].from) && (alt_templates[key].to))) {continue;}
			templates[key] = Object.create(templates[key] || null);
			for (let key2 in alt_templates[key]) {
				templates[key][key2] = alt_templates[key][key2];
			}
		};
	}
	let match_jobs = [];
	for (let key in templates) {
		match_jobs.push(templates[key]);
	}
	if (is_forward) {
		FixArrayTypes(nd_root);
	}
	return nd_root.TranslateTemplates(match_jobs, is_forward);
};

/*
#filter Short names for C++ template types
Correspondence:
- `foo[]` => `std::vector<foo>`
- `foo[:]` => `std::span<foo>`
- `foo[size]!` => `std::array<foo, size>`
- `Map<foo, bar>` => `std::unordered_map<foo, bar>`
Before:
```C++
int main() {
	int[] a;
	a.push_back(10);
	return 0;
}
```
*/
function Translate(nd_root, alt_templates) {
	return BidirTransform(nd_root, alt_templates, 1);
}

function UntranslateSaneTypeNames(nd_root, alt_templates) {
	return BidirTransform(nd_root, alt_templates, 0);
}

Translate.inverse = UntranslateSaneTypeNames;
Translate.setup = function(code, options) {
	options.postfix_operators = '! ' + options.postfix_operators;
};
module.exports = Translate;
module.exports.FixArrayTypes = FixArrayTypes;
