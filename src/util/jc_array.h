#ifndef __JC_ARRAY_H
#define __JC_ARRAY_H
#include "jc_stdinc.h"
#if __cplusplus>=202002L
	#include <span>
#else
	#include "./span_polyfill.h"
#endif

typedef const char const_char;
namespace JC{

template<typename T>
using array_base = std::span<T>;

struct unique_string;

template<typename T>static inline T* array_data(const std::vector<T> &a){return (T*)a.data();}
template<typename T>static inline T* array_data(std::span<T> a){return a.data();}
template<typename T,size_t lg>static inline T* array_data(const std::array<T,lg> &a){return (T*)a.data();}
static inline char* array_data(const std::string &a){return (char*)a.data();}
template<typename T>static inline decltype(array_data(T())) array_data(const std::shared_ptr<T> &a){return array_data(*a);}
template<typename T>static inline decltype(array_data(T())) array_data(const std::unique_ptr<T> &a){return array_data(*a);}
static inline char* array_data(const char* a){return (char*)a;}
static inline char* array_data(const JC::unique_string &a);

//template<typename T>static inline size_t array_size(const std::vector<T> &a){return a.size();}
//template<typename T>static inline size_t array_size(std::span<T> a){return a.size();}
//template<typename T,size_t lg>static inline size_t array_size(const std::array<T,lg> &a){return a.size();}
//static inline size_t array_size(const std::string &a){return a.size();}
//template<typename T>static inline size_t array_size(const std::shared_ptr<T> &a){return array_size(*a);}
template<size_t lg>static inline size_t array_size(char const (&a)[lg]){return lg-1;/*remove the NUL*/}
static inline size_t array_size(const JC::unique_string &a);

template<typename T,bool is_charptr=std::is_same<typename std::remove_reference<typename std::remove_cv<T>::type>::type,char*>::value>
static inline typename std::enable_if<!std::is_same<typename std::remove_reference<typename std::remove_cv<T>::type>::type,JC::unique_string>::value,size_t>::type array_size(T&& a);

template<bool is_charptr>
struct array_size_helper{
	template<typename T>static inline size_t call(const std::vector<T> &a){return a.size();}
	template<typename T>static inline size_t call(std::span<T> a){return a.size();}
	template<typename T,size_t lg>static inline size_t call(const std::array<T,lg> &a){return a.size();}
	static inline size_t call(const std::string &a){return a.size();}
	template<typename T>static inline size_t call(const std::shared_ptr<T> &a){return array_size(*a);}
	template<typename T>static inline size_t call(const std::unique_ptr<T> &a){return array_size(*a);}
};

template<>
struct array_size_helper<true>{
	static inline size_t call(const char* a){
		return strlen(a);
	}
};

template<typename T,bool is_charptr>
static inline typename std::enable_if<!std::is_same<typename std::remove_reference<typename std::remove_cv<T>::type>::type,JC::unique_string>::value,size_t>::type array_size(T&& a){
	return array_size_helper<is_charptr>::call(std::forward<T>(a));
}

//also use this for SFINAE
template<typename Array>
struct GetArrayElementType{};

template<typename T>
struct GetArrayElementType<std::vector<T>>{
	typedef T type;
};

template<typename T,size_t size>
struct GetArrayElementType<std::array<T,size>>{
	typedef T type;
};

template<typename T>
struct GetArrayElementType<std::span<T>>{
	typedef T type;
};

template<>
struct GetArrayElementType<std::string>{
	typedef char type;
};

///////////////////////
//C++11 string concatenation for std container / string literal
static inline size_t concat_total_length(){return 0;}

#ifndef _MSC_VER
template<size_t lg, typename... Types>
static inline size_t concat_total_length(char const (&first)[lg], Types&&... rest);
template<typename... Types>
static inline size_t concat_total_length(const char first, Types&&... rest);
#endif

template<typename T, typename... Types>
static inline size_t concat_total_length(T&& first, Types&&... rest){
	return array_size(first)+concat_total_length(std::forward<Types>(rest)...);
}

template<size_t lg, typename... Types>
static inline size_t concat_total_length(char const (&first)[lg], Types&&... rest){
	return (lg-1)+concat_total_length(std::forward<Types>(rest)...);
}

template<typename... Types>
static inline size_t concat_total_length(const char first, Types&&... rest){
	return 1+concat_total_length(std::forward<Types>(rest)...);
}

static inline void concat_copy_into(char* dest){}

#ifndef _MSC_VER
template<size_t lg, typename... Types>
static inline void concat_copy_into(char* dest, char const (&first)[lg], Types&&... rest);
template<typename... Types>
static inline void concat_copy_into(char* dest, const char first, Types&&... rest);
#endif

template<typename T, typename... Types>
static inline void concat_copy_into(char* dest, T&& first, Types&&... rest){
	size_t first_size = array_size(first);
	memcpy(dest,array_data(first),first_size);
	concat_copy_into(dest+first_size, std::forward<Types>(rest)...);
}

template<size_t lg, typename... Types>
static inline void concat_copy_into(char* dest, char const (&first)[lg], Types&&... rest){
	size_t first_size=lg-1;
	memcpy(dest,first,first_size);
	concat_copy_into(dest+first_size, std::forward<Types>(rest)...);
}

template<typename... Types>
static inline void concat_copy_into(char* dest, const char first, Types&&... rest){
	dest[0]=first;
	concat_copy_into(dest+1, std::forward<Types>(rest)...);
}

template<typename... Types>
static inline std::string string_concat(Types&&... args){
	size_t size=concat_total_length(std::forward<Types>(args)...);
	std::string ret(size,'\0');
	concat_copy_into((char*)ret.data(),std::forward<Types>(args)...);
	return ret;
}

///////////////////////
//JC array methods
//note that std::span fits in two registers and by-value passing is faster than a reference
template<typename T> struct isResizable:std::false_type{};
template<typename T> struct isResizable<std::vector<T>>:std::true_type{};
template<>struct isResizable<std::string>:std::true_type{};
template<typename T> struct isVector:std::false_type{};
template<typename T> struct isVector<std::vector<T>>:std::true_type{};

template<typename T,typename U>
static inline T array_cast(U&& a){
	auto data=array_data(std::forward<U>(a));
	return T(data,data+array_size(std::forward<U>(a)));
}

template<typename T,typename U>
static inline std::shared_ptr<T> array_cast_shared(U&& a){
	auto data=array_data(std::forward<U>(a));
	return std::make_shared<T>(data,data+array_size(std::forward<U>(a)));
}

template<typename T,typename U>
static inline std::unique_ptr<T> array_cast_unique(U&& a){
	auto data=array_data(std::forward<U>(a));
	#if __cplusplus>=201402L
		return std::make_unique<T>(data,data+array_size(std::forward<U>(a)));
	#else
		return std::unique_ptr<T>(new T(data,data+array_size(std::forward<U>(a))));
	#endif
}

template<typename Array>
static inline std::span<typename GetArrayElementType<typename std::remove_reference<Array>::type>::type> array_cast_base(Array&& a){
	return std::forward<Array>(a);
}

//string comparison
static inline int operator==(std::span<char> a, std::span<char> b){
	return a.size()==b.size()&&memcmp(a.data(),b.data(),b.size())==0;
}

static inline int operator!=(std::span<char> a, std::span<char> b){
	return a.size()!=b.size()||memcmp(a.data(),b.data(),b.size())!=0;
}

static inline int operator<(std::span<char> a, std::span<char> b){
	size_t size=a.size()<b.size()?a.size():b.size();
	int diff=memcmp(a.data(),b.data(),size);
	return diff<0||(diff==0&&a.size()<b.size());
}

static inline int operator>(std::span<char> a, std::span<char> b){
	size_t size=a.size()<b.size()?a.size():b.size();
	int diff=memcmp(a.data(),b.data(),size);
	return diff>0||(diff==0&&a.size()>b.size());
}

static inline int operator<=(std::span<char> a, std::span<char> b){
	size_t size=a.size()<b.size()?a.size():b.size();
	int diff=memcmp(a.data(),b.data(),size);
	return diff<0||(diff==0&&a.size()<=b.size());
}

static inline int operator>=(std::span<char> a, std::span<char> b){
	size_t size=a.size()<b.size()?a.size():b.size();
	int diff=memcmp(a.data(),b.data(),size);
	return diff>0||(diff==0&&a.size()>=b.size());
}

template<typename T>
static inline void array_concat_copy_into(std::vector<T> &dest){}

template<typename T, typename U, typename... Types>
static inline void array_concat_copy_into(std::vector<T> &dest, U&& first, Types&&... rest){
	const T* data = array_data(first);
	dest.insert(dest.end(), data, data+array_size(first));
	array_concat_copy_into(dest, std::forward<Types>(rest)...);
}

template<typename Base,typename T>
class ArrayExtension:public Base{
public:
	//typedef typename GetArrayElementType<Base>::type T;
	inline std::span<T> subarray(intptr_t start, intptr_t size)const{
		assert(size_t(start)<=this->size());
		assert(size_t(start+size)<=this->size());
		assert(size_t(size)<=this->size());
		return std::span<T>(this->data()+start,size);
	}
	inline std::span<T> subarray(intptr_t start)const{
		assert(size_t(start)<=this->size());
		return std::span<T>(this->data()+start,this->size()-start);
	}
	template<typename _Base=Base>
	inline typename std::enable_if<isVector<_Base>::value,ArrayExtension<_Base,T>&>::type push(std::span<T> b){
		this->insert(this->end(),b.begin(),b.end());
		return *this;
	}
	template<typename U>
	inline typename std::enable_if<
		isResizable<Base>::value&&std::is_convertible<U,T>::value&&!std::is_convertible<U,std::span<T>>::value,
		ArrayExtension<Base,T>&
	>::type push(U const &b){
		this->push_back(b);
		return *this;
	}
	template<typename _Base>
	inline typename std::enable_if<
		std::is_same<_Base,std::string>::value,
		ArrayExtension<Base,T>&
	>::type push(std::span<char> b){
		this->insert(this->end(),b.begin(),b.end());
		return *this;
	}
	template<typename _Base=Base>
	inline typename std::enable_if<
		isResizable<_Base>::value&&std::is_move_constructible<T>::value,
		T&
	>::type pop(){
		T ret=std::move(this->back());
		this->pop_back();
		return std::move(ret);
	}
	inline ArrayExtension<Base,T>& set(std::span<T> b){
		T* src=this->data();
		T const* tar=b.data();
		size_t size_tar = this->size();
		size_t size_src = b.size();
		std::copy(src, src+std::min(size_tar, size_src), tar);
		return *this;
	}
	template<typename _Base=Base>
	inline typename std::enable_if<
		std::is_same<_Base,std::string>::value,
		int
	>::type& startsWith(std::span<char> b)const{
		return this->size()>=b.size()&&memcmp(this->data(),b.data(),b.size())==0;
	}
	template<typename _Base=Base>
	inline typename std::enable_if<
		std::is_same<_Base,std::string>::value,
		int
	>::type& endsWith(std::span<char> a, std::span<char> b){
		return this->size()>=b.size()&&memcmp(this->data()+(this->size()-b.size()),b.data(),b.size())==0;
	}
	template<typename _Base=Base>
	inline typename std::enable_if<
		std::is_same<_Base,std::string>::value,
		int
	>::type& startsWith(std::span<char> a, char b){
		return this->size()>=1&&a[0]==b;
	}
	template<typename _Base=Base>
	inline typename std::enable_if<
		std::is_same<_Base,std::string>::value,
		int
	>::type& endsWith(std::span<char> a, char b){
		return this->size()>=1&&a.back()==b;
	}
	template<typename U>
	inline std::span<U> as(){
		return std::span<U>(reinterpret_cast<U*>(this->data()),this->size()*sizeof(T)/sizeof(U));
	}
	template<typename U>
	inline void fill(U&& value){
		std::fill(this->begin(),this->end(),std::forward<U>(value));
	}
	template<typename... Types>
	inline std::vector<T> concat(Types&&... args){
		std::vector<T> ret;
		array_concat_copy_into(ret, *this, std::forward<Types>(args)...);
		return std::move(ret);
	}
};
}

//JC::GetArrayElementType serves as the SFINAE gimmick, so we can't `auto` the return type
template<typename Array>
JC::ArrayExtension<Array,typename JC::GetArrayElementType<Array>::type>* operator--(Array &a,int){
	return reinterpret_cast<JC::ArrayExtension<Array,typename JC::GetArrayElementType<Array>::type>*>(&a);
}

template<typename Array>
JC::ArrayExtension<Array,typename JC::GetArrayElementType<Array>::type>* operator--(Array const &a,int){
	return reinterpret_cast<JC::ArrayExtension<Array,typename JC::GetArrayElementType<Array>::type>const*>(&a);
}
#endif
