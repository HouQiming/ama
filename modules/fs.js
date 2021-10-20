//DO NOT use ama features: this is a "chicken" file which gets called when formatting other JS files
'use strict'
let fs = module.exports;

let Buffer = {
	toString:__buffer_toString
};
Buffer.__proto__ = ArrayBuffer;

fs.readFileSync = function(fn) {
	let ret = __readFileSync(fn);
	if (ret) {
		ret.__proto__ = Buffer;
	}
	return ret;
}
fs.existsSync = __existsSync;
fs.writeFileSync = __writeFileSync;
fs.statSync = __statSync;
