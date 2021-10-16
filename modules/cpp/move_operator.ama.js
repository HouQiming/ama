'use strict';
//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)

//COULDDO: auto-move last ref
default_options.prefix_operators = '<< ' + default_options.prefix_operators;

function BidirTransform(nd_root, is_forward) {
	if (is_forward) {
		for (let nd of nd_root.FindAll(N_PREFIX, '<<')) {
			nd.ReplaceWith(.(std::move(.(nd.c.node_class==N_PAREN?nd.c.c:nd.c))));
		}
	} else {
		for (let match of nd_root.MatchAll(.(std::move(.(Node.MatchAny('opr')))))) {
			let nd_opr = match.opr;
			if (nd_opr.node_class != N_REF && nd_opr.node_class != N_DOT && nd_opr.node_class != N_CALL && nd_opr.node_class != N_ITEM) {
				nd_opr = nParen(nd_opr);
			}
			match.nd.ReplaceWith(nPrefix('<<', nd_opr));
		}
	}
	return nd_root;
}

function Translate(nd_root) {
	return BidirTransform(nd_root, 1);
}

function Untranslate(nd_root) {
	return BidirTransform(nd_root, 0);
}

Translate.inverse = Untranslate;
module.exports = Translate;
