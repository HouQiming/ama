'use strict';
//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)
function isTypeLike(nd_type) {
	return nd_type.node_class == N_REF || nd_type.node_class == N_DOT || nd_type.node_class == N_CALL_TEMPLATE;
}
let g_obj_init_transform = {from: .(.(Node.MatchAny('key')):.(Node.MatchAny('value'))),to: .(.(Node.MatchDot(nAir(), 'key'))=.(Node.MatchAny('value')))}
function BidirTransform(nd_root, is_forward) {
	//`type foo{};` as the default
	for (let nd_ref of nd_root.FindAll(N_REF, null)) {
		if (!(nd_ref.flags & REF_DECLARED)) {continue;}
		if (nd_ref.p.node_class != N_RAW) {continue;}
		let nd_stmt = nd_ref.ParentStatement();
		if (nd_stmt.node_class != N_SEMICOLON || nd_stmt.c != nd_ref.p) {continue;}
		if (is_forward) {
			if (!nd_ref.s) {nd_ref.Insert(POS_AFTER, nScope());}
		} else {
			if (nd_ref.s && nd_ref.s.node_class == N_SCOPE&&!nd_ref.s.c) {nd_ref.s.Unlink();}
		}
	}
	for (let nd_scope of nd_root.FindAll(N_SCOPE, null)) {
		//`type{foo:bar}` <=> `type{.foo=bar}`
		if (!nd_scope.c) {continue;}
		if (!(nd_scope.p.node_class == N_RAW && nd_scope.Prev() && isTypeLike(nd_scope.Prev()))) {continue;}
		//no ;
		if (nd_scope.Find(N_SEMICOLON, null)) {continue;}
		if (!is_forward) {
			//console.log(nd_scope.toSource());
			//TODO: query the type / declared type
			for (let ndi = nd_scope.c; ndi; ndi = ndi.s) {
				if (ndi.node_class != N_ASSIGNMENT) {
					//name it
					//TODO
				}
				//TODO: name the unnamed initializers
			}
		}
		nd_scope.TranslateTemplates([g_obj_init_transform], is_forward);
	}
}

function Translate(nd_root) {
	return BidirTransform(nd_root, 1);
}

function Untranslate(nd_root) {
	return BidirTransform(nd_root, 0);
}

Translate.inverse = Untranslate;
module.exports = Translate;
