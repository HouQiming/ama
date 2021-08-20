/*
@ama
let nd_root=ParseCurrentFile();
nd_root
	.NodeofToASTExpression()
	.Save('.audit.js')
*/

function CheckWrap(nd){
	return .(CHECK(.(nString(nd.toSource())),.(nd)));
}
