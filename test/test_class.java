////////////
class B{int something;};
class C{int something;};
interface D{int something;};
class A extends B,C implements D{
	static public final int a;
};
class A:B,C,D{
	static public final int a;
};
//typescript mis-named as .js
declare function max<T>(a:Array<T>|{[key:any]:T}):T;
