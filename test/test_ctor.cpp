/*
@ama
const amacpp=require('cpp/amacpp');
let nd_root=ParseCurrentFile({parse_indent_as_scope:1,parse_indent_as_scope_but_merge_cpp_ctor_lines:1});
console.log(JSON.stringify(nd_root,null,1));
console.log(nd_root.then(amacpp).toSource().split('\n').join('\\\n'));
*/
struct FooImpl: public torch::nn::Module {
	torch::Tensor w_k;
	torch::Tensor b_k;
	FooImpl(NetConfig const& config):
	w_k(torch::zeros({config.d_model, config.heads * config.attn_features})),
	b_k(torch::zeros({config.heads * config.attn_features})) {
		register_parameter("w_k", this->w_k);
		register_parameter("b_k", this->b_k);
	}
};
