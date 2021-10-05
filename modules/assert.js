'use strict'
module.exports = function(cond){
	if(!cond){
		throw new Error('assertion failure')
	}
}
