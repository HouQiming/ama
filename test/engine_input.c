
int main(){
	char* y;
	y="before";
	if(y>2){
		x=y;
	}else{
		y="else";
	}
	if(x>2){
		y='unrelated then'
	}else{
		y="unrelated else";
	}
	x="after";
	console.log(x);
	return 0;
}
