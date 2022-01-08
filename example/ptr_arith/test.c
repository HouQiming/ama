int main(){
	int a=3;
	int *p=&a;
	p+=1;
	int b=*(p-1);
	return 0;
}
