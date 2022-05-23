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
