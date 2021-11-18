'use strict';
default_options.postfix_operators = '! ' + default_options.postfix_operators;
let g_templates = {
	array: {from: @(@(Node.MatchAny('TElement'))[]), to: @(std::vector<@(Node.MatchAny('TElement'))>)},
	view: {from: @(@(Node.MatchAny('TElement'))[:]), to: @(std::span<@(Node.MatchAny('TElement'))>)},
	fixed_array: {from: nPostfix(@(@(Node.MatchAny('TElement'))[@(Node.MatchAny('size'))]), '!'), to: @(std::array<@(Node.MatchAny('TElement')), @(Node.MatchAny('size'))>)},
	map: {from: @(Map<@(Node.MatchAny('TKey')), @(Node.MatchAny('TValue'))>), to: @(std::unordered_map<@(Node.MatchAny('TKey')), @(Node.MatchAny('TValue'))>)},
};

function FixArrayTypes(nd_root) {
	for (let nd_mul of nd_root.FindAll(N_BINOP, '*')) {
		//[]
		if (nd_mul.c.s.isRawNode('[', ']')) {
			let nd_subscripts = nd_mul.c.BreakSibling().c;
			let nd_item = nItem(nPostfix(nd_mul.BreakChild(), '*'));
			if (nd_subscripts) {
				nd_item.Insert(POS_BACK, nd_subscripts);
			}
			nd_mul.ReplaceWith(nd_item);
		}
	}
}

function BidirTransform(nd_root, alt_templates, is_forward) {
	let templates = g_templates;
	if (alt_templates) {
		templates = Object.create(g_templates)
		for (let key in alt_templates) {
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

function TranslateSaneTypeNames(nd_root, alt_templates) {
	return BidirTransform(nd_root, alt_templates, 1);
}

function UntranslateSaneTypeNames(nd_root, alt_templates) {
	return BidirTransform(nd_root, alt_templates, 0);
}

TranslateSaneTypeNames.inverse = UntranslateSaneTypeNames;
module.exports = TranslateSaneTypeNames;
module.exports.FixArrayTypes = FixArrayTypes;
