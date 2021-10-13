'use strict';
//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)
let g_templates = {
	array: {from: .(.(Node.MatchAny('TElement'))[]), to: .(std::vector<.(Node.MatchAny('TElement'))>)},
	view: {from: .(.(Node.MatchAny('TElement'))[:]), to: .(JC::array_base<.(Node.MatchAny('TElement'))>)},
	fixed_array: {from: .(.(Node.MatchAny('TElement'))[.(Node.MatchAny('size'))]), to: .(std::array<.(Node.MatchAny('TElement')), .(Node.MatchAny('size'))>)},
	map: {from: .(Map<.(Node.MatchAny('TKey')), .(Node.MatchAny('TValue'))>), to: .(std::unordered_map<.(Node.MatchAny('TKey')), .(Node.MatchAny('TValue'))>)},
}
function TranslateTemplates(nd_root, alt_templates, is_forward) {
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
	if (is_forward) {
		for (let key in templates) {
			match_jobs.push(templates[key]);
		}
		for (let nd_tv of nd_root.FindAll(N_TYPED_VAR, null)) {
			let nd_type = nd_tv.c;
			for (let ndi = nd_type; ndi; ndi = ndi.PreorderNext(nd_tv.c)) {
				for (let job of match_jobs) {
					let match = ndi.Match(job.from);
					if (match) {
						for (let param in match) {
							if (param != 'nd' && match[param].s) {match[param].BreakSibling();}
						}
						ndi = ndi.ReplaceWith(job.to.Subst(match));
						break;
					}
				}
			}
		}
	} else {
		for (let key in templates) {
			match_jobs.push({from: templates[key].to,to: templates[key].from});
		}
		for (let ndi = nd_root; ndi; ndi = ndi.PreorderNext(nd_root)) {
			for (let job of match_jobs) {
				let match = ndi.Match(job.from);
				if (match) {
					for (let param in match) {
						if (param != 'nd' && match[param].s) {match[param].BreakSibling();}
					}
					ndi = ndi.ReplaceWith(job.to.Subst(match));
					break;
				}
			}
		}
	}
	return nd_root;
};

function TranslateSaneTypeNames(nd_root, alt_templates) {
	return TranslateTemplates(nd_root, alt_templates, 1);
}

function UntranslateSaneTypeNames(nd_root, alt_templates) {
	return TranslateTemplates(nd_root, alt_templates, 0);
}

TranslateSaneTypeNames.inverse = UntranslateSaneTypeNames;
module.exports = TranslateSaneTypeNames;
