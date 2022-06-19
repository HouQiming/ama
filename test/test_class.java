////////////
class B{int something;};
class C{int something;};
interface D{int something;};
@Author(
   name = "Benjamin Franklin",
   date = "3/27/2003"
)
class A extends B,C implements D{
	static public final int a;
};
class A:B,C,D{
	static public final int a=(foo,bar)->{return foo+bar;};
};
//typescript mis-named as .js
declare function max<T>(a:Array<T>|{[key:any]:T}):T;
