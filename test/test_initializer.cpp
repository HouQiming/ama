/*
@ama
const amacpp=require('cpp/amacpp');
let nd_root=ParseCurrentFile({parse_indent_as_scope:1});
console.log(JSON.stringify(nd_root,null,1));
console.log(nd_root.then(amacpp).toSource({auto_space:0}));
*/
int main(){
	unsigned int a=PredictionResult{
		items: {
			PredictionItem{text:"if"},
			PredictionItem{text:"for"},
			PredictionItem{text:"return"},
			PredictionItem{text:"switch"},
			PredictionItem{text:"case"},
			PredictionItem{text:"while"},
			PredictionItem{text:"do"},
			PredictionItem{
				text:"goto"
			},
			PredictionItem{text:"throw"},
			PredictionItem{text:"try"}
		}
	};
	return(a);
	return -a;
	return PredictionResult{
		error:"not implemented"
	}
}
