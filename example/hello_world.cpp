//use #if 0 to syntax-highlight the script code
#if 0
	@ama
	let nd_root=ParseCurrentFile();
	//dump the AST
	console.log(JSON.stringify(nd_root,null,2));
	//search the `hello(world)` calls
	let counter=0;
	for(let match of nd_root.MatchAll(.(hello(world)))){
		//replace them with some counter-printing
		match.nd.ReplaceWith(.(std::cout<<.(nNumber(counter.toString()))<<std::endl));
		counter+=1;
	}
	//insert #include <iostream>
	nd_root.Insert(POS_FRONT,.(#include <iostream>));
	//save the result
	nd_root.Save('.audit.cpp');
	console.log('Saved hello_world.audit.cpp');
#endif

int main(){
	hello(world);
	hello(world);
	hello(world);
	return 0;
}
