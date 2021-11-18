//@ama ParseCurrentFile().NodeofToASTExpression().Save('.audit.js')

function CheckWrap(nd){
	return @(CHECK(@(nString(nd.toSource())),@(nd)));
}
