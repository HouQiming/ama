'use strict'
const path = require('path');
const fs = require('fs');
let pipe = module.exports;

pipe.run = __system;
pipe.runPiped = function(cmd){
	let tmp_name = path.join(process.platform==='win32'?process.env.TEMP:'/tmp', 'ama_'+Date.now().toString()+'.txt');
	let exitCode = __system([cmd, ' > ', tmp_name].join(''));
	return {stdout:fs.existsSync(tmp_name)?fs.readFileSync(tmp_name).toString():'', exitCode:exitCode};
}
