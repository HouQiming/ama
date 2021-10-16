'use strict';
//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)
require('class');

function BidirTransform(nd_root, is_forward) {
	let from = is_forward ? 'public' : 'static';
	let to = is_forward ? 'static' : 'public';
	for (let nd_func of nd_root.FindAll(N_FUNCTION, null)) {
		let nd_owner = nd_func.Owner();
		if (!(nd_owner.node_class == N_CLASS || nd_owner.node_class == N_FILE)) {
			continue;
		}
		if (!(nd_func.c.node_class == N_RAW && nd_func.c.c && nd_func.c.LastChild().node_class == N_REF)) {
			//don't do the static magic on method implementations
			continue;
		}
		let nd_kw = nd_func.c.Find(N_REF, from);
		if (nd_kw) {
			let nd_next = nd_kw.s;
			nd_kw.Unlink();
			if (nd_next) {
				nd_next.comments_before = nd_next.comments_before.replace(/^[ \t\r\n]+/, '');
			}
		} else if(nd_func.c.node_class == N_RAW) {
			//inverse it
			nd_func.c.Insert(POS_FRONT, nRef(to).setCommentsAfter(' '));
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
