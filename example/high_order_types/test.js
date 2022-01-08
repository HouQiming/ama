data Compose<A,B,C>=Compose(A<B<C>>);

function someMaybeListInt():Compose<Maybe,Array,number>{
	return Compose<Maybe,Array,number>(Just<Array<number>>([1.0,2.0,3.0]));
}

function extractMaybeListInt(c:Compose<Maybe,Array,number>){
	let [content]=c;
	return content||[];
}

console.log(extractMaybeListInt(someMaybeListInt()));
