function Translate(nd_root) {
	for (let nd_line of nd_root.FindAll(N_KEYWORD_STATEMENT, '#line')) {
		nd_line.c.ReplaceWith(nNumber((nd_line.ComputeLineNumber() + 2).toString()));
	}
	return nd_root;
}

function Untranslate(nd_root) {
	for (let nd_line of nd_root.FindAll(N_KEYWORD_STATEMENT, '#line')) {
		nd_line.c.ReplaceWith(nRef('__AMA_LINE__'));
	}
	return nd_root;
}

Translate.inverse = Untranslate;
module.exports = Translate;
