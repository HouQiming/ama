'use strict';

function BidirTransform(nd_root, is_forward) {
	if (is_forward) {
		for (let nd of nd_root.FindAll(N_REF, 'NULL')) {
			nd.data = 'nullptr';
		}
	} else {
		for (let nd of nd_root.FindAll(N_REF, 'nullptr')) {
			nd.data = 'NULL';
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
