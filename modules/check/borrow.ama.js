module.exports = {
	//TODO: address-taking vs const address taking
	//need to track *outstanding pointers*
	//it's a value-carried *relationship* that should be tracked from both ends
	//and the relationship can *die*: when a pointer gets overwritten
	//needs different codegen mechanism: death tracking requires RC
	//we can manually generate the scope-killing and assign-killing... but we can't really kill sth in a fork
	//rely on gc? we want to know "at point A, is there any B alive which has a relationship R with C", A could be in a fork
	//could solve with active context enumeration? scrub context in A's fork, don't store relationship on C, store on B
	//B could be buried in a field... well, we try our best to find
	//FindValue
	//should improve checker QoL: use QuickJS toString() to move code into the sandboxed checker
};
