'use strict';
const assert = require('assert');
const depends = require('depends');
//this is a language support module, we mainly extend Node
let classes = module.exports;

let g_not_class = new Set(['static', 'final', 'const', 'public', 'private', 'protected', 'extends', 'implements']);
let g_protections = ['public', 'private', 'protected'];

function ListOwnProperties(properties, nd_class) {
	let nd_scope = nd_class.LastChild();
	let last_appearance = {};
	let lingering_protection = nd_class.data == 'class' ? 'private' : 'public';
	for (let ndi = nd_scope; ndi; ndi = ndi.PreorderNext(nd_scope)) {
		if (ndi.node_class == N_REF && g_not_class.has(ndi.data)) {
			if (ndi.p && ndi.p.node_class == N_LABELED && ndi.p.c == ndi) {
				//it's lingering
				lingering_protection = ndi.data;
			} else {
				last_appearance[ndi.data] = ndi.ParentStatement();
			}
		}
		let protection = lingering_protection;
		for (let prot of g_protections) {
			if (last_appearance[prot] && last_appearance[prot].isAncestorOf(ndi)) {
				protection = prot;
				break;
			}
		}
		//if (ndi.node_class == N_REF){
		//	console.log(ndi.data,ndi.flags);
		//}
		if (ndi.node_class == N_REF && (ndi.flags & REF_DECLARED)) {
			let is_static = last_appearance['static'] && last_appearance['static'].isAncestorOf(ndi) || nd_class.data == 'namespace';
			properties.push({
				enumerable: !is_static | 0,
				writable: !(last_appearance['final'] && last_appearance['final'].isAncestorOf(ndi) || last_appearance['const'] && last_appearance['const'].isAncestorOf(ndi)) | 0,
				own: 1,
				shadowed: 0,
				static: is_static | 0,
				protection: protection,
				kind: 'variable',
				node: ndi,
				name: ndi.data
			});
		} else if (ndi.node_class == N_FUNCTION) {
			if (ndi.data) {
				properties.push({
					enumerable: 0,
					writable: 0,
					own: 1,
					shadowed: 0,
					static: 0 | (last_appearance['static'] && last_appearance['static'].isAncestorOf(ndi)),
					protection: protection,
					kind: 'method',
					node: ndi,
					name: ndi.data
				});
			}
			ndi = ndi.PreorderSkip();
			continue;
		} else if (ndi.node_class == N_CLASS) {
			properties.push({
				enumerable: 0,
				writable: 0,
				own: 1,
				shadowed: 0,
				static: 1,
				protection: protection,
				kind: 'class',
				node: ndi,
				name: ndi.GetName()
			});
			ndi = ndi.PreorderSkip();
			continue;
		}
	}
}

///figure out inheritance and property list without caching: nodes could change later
///often we aren't reusing the result anyway
Node.ParseClass = function() {
	if (this.node_class != N_CLASS) {throw new Error('ParseClass is only valid on a class node');}
	//children: before, name, after, scope
	//base classes
	let nd_after = this.c.s.s;
	let base_class_set = new Set();
	for (let ndi = nd_after; ndi; ndi = ndi.PreorderNext(nd_after)) {
		if ((ndi.node_class == N_REF || ndi.node_class == N_DOT) && !g_not_class.has(ndi.data)) {
			//COULDDO: use typing: let type_obj = typing.TryGettingClass(typing.ComputeType(ndi))
			for (let nd_class of ndi.LookupClass()) {
				if (nd_class) {base_class_set.add(nd_class);}
			}
		}
		if (ndi.node_class == N_CALL || ndi.node_class == N_CALL_TEMPLATE ||
		ndi.node_class == N_RAW && (ndi.flags & 0xffff) || ndi.node_class == N_SCOPE ||
		ndi.node_class == N_DOT || ndi.node_class == N_ITEM) {
			ndi = ndi.PreorderSkip();
		}
	}
	let base_classes = [];
	base_class_set.forEach(nd_class => {
		base_classes.push(nd_class);
	});
	//properties: enum base_classes first
	let properties = [];
	for (let nd_class of base_classes) {
		ListOwnProperties(properties, nd_class);
	}
	for (let ppt of properties) {
		ppt.own = 0;
	}
	ListOwnProperties(properties, this);
	//shadowing deduction
	let shadows = new Set();
	for (let i = properties.length - 1; i >= 0; i--) {
		if (shadows.has(properties[i].name)) {
			properties[i].enumerable = 0;
			properties[i].shadowed = 1;
		}
		shadows.add(properties[i].name);
	}
	return {
		nd: this,
		base_classes: base_classes,
		properties: properties,
	}
};

function LookupClassInFile(ret, nd_root, nd_name) {
	//COULDDO: match-ness scoring + high score picking, but conservative listing could work better
	let name = nd_name.GetName();
	for (let nd_class of nd_root.FindAll(N_CLASS, name)) {
		//skip forwards
		if (nd_class.LastChild().node_class == N_SCOPE) {
			ret.push(nd_class);
		}
	}
}

//look up a ref / dot node
Node.LookupClass = function() {
	let ret = [];
	for (let nd_root of depends.ListAllDependency(this.Root(), true)) {
		if (nd_root) {
			LookupClassInFile(ret, nd_root, this);
		}
	}
	return ret;
};
