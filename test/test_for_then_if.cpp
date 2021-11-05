/*
@ama
const amacpp=require('cpp/amacpp');
let nd_root=ParseCurrentFile({parse_indent_as_scope:1});
console.log(JSON.stringify(nd_root,null,1));
console.log(nd_root.then(amacpp).toSource({auto_space:0}));
*/
int operator==(){
	for i<this->core->config.vocab_size
		toks.push_back(TokenWithProb{
			token:i,
			prob:p_token_probs[i]
		})
	toks--->sort();
	if this->core->config.sampling_strategy==SAMPLE_BEST
		toks.resize(1)
	else if this->core->config.sampling_strategy==SAMPLE_TOP5
		toks.resize(5)
	else if this->core->config.sampling_strategy==SAMPLE_95P
		prob_popped=0.f;
		while toks.size()>1&&prob_popped+toks.back()<0.05f:
			prob_popped+=toks--->pop();
	else
		assert(0)
}
