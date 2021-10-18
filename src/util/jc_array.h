#ifndef __JC_ARRAY_H
#define __JC_ARRAY_H
#include "jc_stdinc.h"

typedef const char const_char;
namespace JC{

///////////////////////
//base array for unified parameter passing
template<typename T>
struct array_base{
	T* m_data;
	size_t m_size;
	array_base():m_data(NULL),m_size(0){}
	array_base(const std::vector<T> &a):m_data((T*)a.data()),m_size(a.size()){}
	array_base(const std::shared_ptr<std::vector<T>> &a):m_data((T*)a->data()),m_size(a->size()){}
	array_base(const std::unique_ptr<std::vector<T>> &a):m_data((T*)a->data()),m_size(a->size()){}
	template<size_t U>
	array_base(const std::array<T,U> &a):m_data((T*)a.data()),m_size(U){}
	array_base(const std::string &a):m_data((T*)a.data()),m_size(a.size()/sizeof(T)){}
	array_base(const std::shared_ptr<std::string> &a):m_data((T*)a->data()),m_size(a->size()/sizeof(T)){}
	array_base(const std::unique_ptr<std::string> &a):m_data((T*)a->data()),m_size(a->size()/sizeof(T)){}
	#if __cplusplus>=201103L
		#if __cplusplus&&!defined(__clang__)
		#pragma GCC diagnostic ignored "-Winit-list-lifetime"
		#endif
		array_base(std::initializer_list<T> a):m_data((T*)a.begin()),m_size(a.size()){}
	#endif
	//template<typename U = T, typename std::enable_if<std::is_same<U,char>::value>::type* = nullptr>
	//array_base(const char* s):m_data((char*)s),m_size(strlen(s)){}
	//for string literals only
	template<size_t size>
	array_base(T const (&data)[size]):m_data((T*)data),m_size((size-1)/sizeof(T)){}
	array_base(const T* data,size_t size):m_data((T*)data),m_size(size){}
	array_base(const T* data,const T* data_end):m_data((T*)data),m_size(data_end-data){}
	template<typename const_char_ptr,typename test=typename std::enable_if<
		std::is_same<const_char_ptr,const char*>::value&&std::is_same<T,char>::value
	>::type>
	array_base(const_char_ptr data):m_data((char*)data),m_size(strlen(data)){}
	T& operator[](intptr_t index)const{
		assert(size_t(index)<m_size);
		return m_data[index];
	}
	const T* c_str()const{return (const T*)m_data;}
	T* data()const{return m_data;}
	size_t size()const{return m_size;}
	T* begin()const{return m_data;}
	T* end()const{return m_data+m_size;}
	T& back(){
		assert(m_size);
		return m_data[m_size-1];
	}
};

struct unique_string;

template<typename T>static inline T* array_data(const std::vector<T> &a){return (T*)a.data();}
template<typename T>static inline T* array_data(array_base<T> a){return a.data();}
template<typename T,size_t lg>static inline T* array_data(const std::array<T,lg> &a){return (T*)a.data();}
static inline char* array_data(const std::string &a){return (char*)a.data();}
template<typename T>static inline decltype(array_data(T())) array_data(const std::shared_ptr<T> &a){return array_data(*a);}
template<typename T>static inline decltype(array_data(T())) array_data(const std::unique_ptr<T> &a){return array_data(*a);}
static inline char* array_data(const char* a){return (char*)a;}
static inline char* array_data(const JC::unique_string &a);

//template<typename T>static inline size_t array_size(const std::vector<T> &a){return a.size();}
//template<typename T>static inline size_t array_size(array_base<T> a){return a.size();}
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
	template<typename T>static inline size_t call(array_base<T> a){return a.size();}
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
struct GetArrayElementType<JC::array_base<T>>{
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
//note that array_base fits in two registers and by-value passing is faster than a reference
template<typename Array>
static inline array_base<typename GetArrayElementType<Array>::type> subarray(const Array &a, intptr_t start, intptr_t size){
	assert(size_t(start)<=a.size());
	assert(size_t(start+size)<=a.size());
	assert(size_t(size)<=a.size());
	return array_base<typename GetArrayElementType<Array>::type>(a.data()+start,size);
}

template<typename Array>
static inline array_base<typename GetArrayElementType<Array>::type> subarray(const Array &a, intptr_t start){
	assert(size_t(start)<=a.size());
	return array_base<typename GetArrayElementType<Array>::type>(a.data()+start,a.size()-start);
}

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
static inline array_base<typename GetArrayElementType<typename std::remove_reference<Array>::type>::type> array_cast_base(Array&& a){
	return std::forward<Array>(a);
}

template<typename T,typename U>
static inline typename std::enable_if<std::is_convertible<U,array_base<T>>::value,std::vector<T>>::type& push(std::vector<T>& a, const U &b){
	auto data=array_data(b);
	a.insert(a.end(),data,data+array_size(b));
	return a;
}

template<typename T,typename U>
static inline typename std::enable_if<std::is_convertible<U,T>::value&&!std::is_convertible<U,array_base<T>>::value,std::vector<T>>::type& push(std::vector<T>& a, const U &b){
	a.push_back(b);
	return a;
}

static inline std::string& push(std::string& a, array_base<char> b){
	a.append(b.data(),b.size());
	return a;
}

static inline std::string& push(std::string& a, char b){
	a.push_back(b);
	return a;
}

template<typename T>
static inline T pop(std::vector<T>& a){
	T ret=a.back();
	a.pop_back();
	return ret;
}

static inline char pop(std::string& a){
	char ret=a.back();
	a.pop_back();
	return ret;
}

static inline char* data(array_base<char> a){
	return (char*)a.data();
}

template<typename T,typename U>
static inline typename std::enable_if<std::is_same<decltype(*array_data(typename std::remove_reference<T>::type())),decltype(*array_data(typename std::remove_reference<U>::type()))>::value,void>::type set(T&& a, U&& b){
	auto* tar = array_data(a);
	auto* src = array_data(b);
	size_t size_tar = array_size(a);
	size_t size_src = array_size(b);
	std::copy(src, src+std::min(size_tar, size_src), tar);
}

//string comparison
static inline int operator==(array_base<char> a, array_base<char> b){
	return a.size()==b.size()&&memcmp(a.data(),b.data(),b.size())==0;
}

static inline int operator!=(array_base<char> a, array_base<char> b){
	return a.size()!=b.size()||memcmp(a.data(),b.data(),b.size())!=0;
}

static inline int operator<(array_base<char> a, array_base<char> b){
	size_t size=a.size()<b.size()?a.size():b.size();
	int diff=memcmp(a.data(),b.data(),size);
	return diff<0||(diff==0&&a.size()<b.size());
}

static inline int operator>(array_base<char> a, array_base<char> b){
	size_t size=a.size()<b.size()?a.size():b.size();
	int diff=memcmp(a.data(),b.data(),size);
	return diff>0||(diff==0&&a.size()>b.size());
}

static inline int operator<=(array_base<char> a, array_base<char> b){
	size_t size=a.size()<b.size()?a.size():b.size();
	int diff=memcmp(a.data(),b.data(),size);
	return diff<0||(diff==0&&a.size()<=b.size());
}

static inline int operator>=(array_base<char> a, array_base<char> b){
	size_t size=a.size()<b.size()?a.size():b.size();
	int diff=memcmp(a.data(),b.data(),size);
	return diff>0||(diff==0&&a.size()>=b.size());
}

static inline int startsWith(array_base<char> a, array_base<char> b){
	return a.size()>=b.size()&&memcmp(a.data(),b.data(),b.size())==0;
}

static inline int endsWith(array_base<char> a, array_base<char> b){
	return a.size()>=b.size()&&memcmp(a.data()+(a.size()-b.size()),b.data(),b.size())==0;
}

static inline int startsWith(array_base<char> a, char b){
	return a.size()>=1&&a[0]==b;
}

static inline int endsWith(array_base<char> a, char b){
	return a.size()>=1&&a[a.size()-1]==b;
}

template<typename T, typename Array>
static inline array_base<T> array_as(const Array &a){
	return array_base<T>((T*)a.data(),a.size()*sizeof(typename GetArrayElementType<Array>::type)/sizeof(T));
}

template<typename T,typename U>
static inline void fill(T &&a, U&& value){
	std::fill(a.begin(),a.end(),std::forward<U>(value));
}

template<typename T>
static inline void array_concat_copy_into(std::vector<T> &dest){}

template<typename T, typename U, typename... Types>
static inline void array_concat_copy_into(std::vector<T> &dest, U&& first, Types&&... rest){
	const T* data = array_data(first);
	dest.insert(dest.end(), data, data+array_size(first));
	array_concat_copy_into(dest, std::forward<Types>(rest)...);
}

template<typename T,typename... Types>
static inline std::vector<T> concat(const std::vector<T> &first, Types&&... args){
	std::vector<T> ret;
	array_concat_copy_into(ret, first, std::forward<Types>(args)...);
	return std::move(ret);
}

////////////////////////////
template<typename T>
static inline void* sharedToVoid(const std::shared_ptr<T>& a){
	//static_assert(sizeof(std::shared_ptr<T>)==sizeof(void*),"shared_ptr has to be a single pointer");
	//void* volatile p=NULL;
	//*(std::shared_ptr<T>*)&p=a;
	//return (void*)p;
	return a?(void*)(new std::shared_ptr<T>(a)):NULL;
}

template<typename T>
static inline std::shared_ptr<T> voidToShared(void* p0){
	//static_assert(sizeof(std::shared_ptr<T>)==sizeof(void*),"shared_ptr has to be a single pointer");
	//void* volatile p=p0;
	//return *(std::shared_ptr<T>*)&p;
	return p0?*(std::shared_ptr<T>*)p0:std::shared_ptr<T>(nullptr);
}

}

namespace std{
template<typename T>
static inline decltype(T().begin()) begin(const std::shared_ptr<T> &a){
	return a->begin();
}

template<typename T>
static inline decltype(T().end()) end(const std::shared_ptr<T> &a){
	return a->end();
}

template<typename T>
static inline decltype(T().begin()) begin(const std::unique_ptr<T> &a){
	return a->begin();
}

template<typename T>
static inline decltype(T().end()) end(const std::unique_ptr<T> &a){
	return a->end();
}
}

#endif
