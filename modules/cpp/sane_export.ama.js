'use strict';
//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)
const path = require('path');
require('class');

function BidirTransform(nd_root, is_forward) {
	if (path.extname(nd_root.data).startsWith('.h')) {return nd_root;}
	let from = is_forward ? 'public' : 'static';
	let to = is_forward ? 'static' : 'public';
	for (let nd_func of nd_root.FindAll(N_FUNCTION, null)) {
		let nd_owner = nd_func.Owner();
		if (!(nd_owner.node_class == N_CLASS && nd_owner.data == 'namespace' || nd_owner.node_class == N_FILE)) {
			continue;
		}
		if (!(nd_func.c.node_class == N_RAW && nd_func.c.c && nd_func.c.LastChild().node_class == N_REF)) {
			//don't do the static magic on method implementations
			continue;
		}
		if (nd_func.LastChild().node_class != N_SCOPE) {
			//don't do it on forward declarations
			continue;
		}
		let nd_before = nd_func.c;
		let nd_kw = nd_before.Find(N_REF, from);
		if (nd_kw) {
			let nd_next = nd_kw.s;
			nd_kw.Unlink();
			if (nd_next) {
				nd_next.comments_before = nd_next.comments_before.replace(/^[ \t\r\n]+/, '');
			}
		} else if (nd_before.node_class == N_RAW) {
			//inverse it
			if (!nd_before.Find(N_REF, to)) {
				let nd_1st = nd_before.c;
				if (nd_1st && nd_1st.node_class == N_CALL_TEMPLATE && nd_1st.GetName() == 'template') {
					nd_1st.Insert(POS_AFTER, nRef(to).setCommentsAfter(' '));
				} else {
					nd_before.Insert(POS_FRONT, nRef(to).setCommentsAfter(' '));
				}
			}
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
