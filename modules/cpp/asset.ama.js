'use strict'
const fs = require('fs');
const path = require('path');
const gentag = require('cpp/gentag');

module.exports = function Transform(nd_root) {
	for (let match of gentag.FindAllGenTags(nd_root, .(asset(.(Node.MatchAny(N_STRING, 'file')))))) {
		let file = match.file.GetStringValue();
		let name = path.parse(file).name.replace(/[^a-zA-Z0-9]+/g, '_').toLowerCase();
		let data = new Uint8Array(fs.readFileSync(file));
		let ret = ['\nstatic const uint8_t ', name, '[]={'];
		for (let i = 0; i < data.length; i++) {
			if (!(i & 127)) {
				ret.push('\n\t');
			}
			ret.push(data[i].toString(), ',');
		}
		ret.push('0\n};\n');
		gentag.UpdateGenTagContent(match.gentag, nAir().setCommentsBefore(ret.join('')));
	}
};
