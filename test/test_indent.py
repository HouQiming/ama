#@ama
#let nd_root=ParseCurrentFile();
#console.log(JSON.stringify(nd_root,null,2))
#console.log(nd_root.toSource())
import torch

def main(
	argc,
	argv
):
	normal_indent=1

	after_blank_line=2
	if true:
		indented=0
		
		indented_after_blank_line=1
	else:
			bad_indent=0
		normal_indent=1
	long_string='''
		comment
			not a scope
	this is entirely valid
								so is this''' + (
'inside_expression' +
				'so this is OK too'
	)
	#random comment
	return

import again
