'use strict'
let _cmdline = module.exports;

_cmdline.help = function(argv) {
	console.log("  ama [-s script] [files]  = process [files] with [script]");
	for (let key in _cmdline) {
		if (_cmdline[key].usage) {
			console.log([
				'  ama --', key, _cmdline[key].usage,
			].join(''));
		}
	}
};
_cmdline.help.usage = '               = print this help';

_cmdline.init = function(argv) {
	//TODO
};
_cmdline.init.usage = ' [dir]         = setup ama for a project, defaults to ./';

_cmdline.build = function(argv) {
	//TODO
};
_cmdline.build.usage = ' [dir]        = build a project, defaults to ./';
