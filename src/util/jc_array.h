#ifndef __JC_ARRAY_H
#define __JC_ARRAY_H
#include <vector>
#include <string>
#include <memory>
#include <type_traits>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <memory.h>
#include <stddef.h>
#include <unordered_map>
#include <algorithm>
#include <utility>
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

template<typename T>
static inline void array_concat_copy_into(std::vector<T> &dest){}

template<typename T, typename U, typename... Types>
static inline void array_concat_copy_into(std::vector<T> &dest, U&& first, Types&&... rest){
	const T* data = array_data(first);
	dest.insert(dest.end(), data, data+array_size(first));
	array_concat_copy_into(dest, std::forward<Types>(rest)...);
}

template<typename T>
static inline void AppendArray(std::vector<T> &a,std::span<T> b){
	a.insert(a.end(),b.begin(),b.end());
}

static inline void AppendArray(std::string &a,std::span<char> b){
	a.append(b.data(),b.size());
}

#ifdef _MSC_VER
#define memmem fallback_memmem

//two-way memmem fallback, use template hack to avoid code duplication
//http://www-igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260
template<class T=void>
size_t maxSuf(const char *x, size_t m, size_t *p, int32_t mask) {
	size_t ms=0, j=0, k=0;
	int32_t a=0, b=0;

	ms = -1;
	j = 0;
	k = *p = 1;
	while (j + k < m) {
		a = (int32_t)(uint32_t)(uint8_t)x[j + k]^mask;
		b = (int32_t)(uint32_t)(uint8_t)x[ms + k]^mask;
		if (a < b) {
			j += k;
			k = 1;
			*p = j - ms;
		}
		else
			if (a == b)
				 if (k != *p)
					 ++k;
				 else {
					 j += *p;
					 k = 1;
				 }
			else { /* a > b */
				 ms = j;
				 j = ms + 1;
				 k = *p = 1;
			}
	}
	return(ms);
}

static inline size_t max_iptr(size_t a,size_t b){return a>b?a:b;}

template<class T=void>
void* fallback_memmem(const char* haystack,size_t haystack_size, const char* needle,size_t needle_size){
	size_t i=0, j=0, ell=0, memory=0, p=0, per=0, q=0;
	/* Preprocessing */
	i = maxSuf(needle, needle_size, &p, 0);
	j = maxSuf(needle, needle_size, &q, -1);
	if (i > j) {
		ell = i;
		per = p;
	} else {
		ell = j;
		per = q;
	}
	/* Searching */
	if (memcmp(needle, needle + per, ell + 1) == 0) {
		j = 0;
		memory = -1;
		while (j <= haystack_size - needle_size) {
			i = max_iptr(ell, memory) + 1;
			while (i < needle_size && needle[i] == haystack[i + j])
				++i;
			if (i >= needle_size) {
				i = ell;
				while (i > memory && needle[i] == haystack[i + j])
					--i;
				if (i <= memory){
					return (void*)(haystack+(j));
				}
				j += per;
				memory = needle_size - per - 1;
			}
			else {
				j += (i - ell);
				memory = -1;
			}
		}
	} else {
		per = max_iptr(ell + 1, needle_size - ell - 1) + 1;
		j = 0;
		while (j <= haystack_size - needle_size) {
			i = ell + 1;
			while (i < needle_size && needle[i] == haystack[i + j])
				++i;
			if (i >= needle_size) {
				i = ell;
				while (i >= 0 && needle[i] == haystack[i + j])
					--i;
				if (i < 0){
					return (void*)(haystack+(j));
				}
				j += per;
			}
			else
				j += (i - ell);
		}
	}
	return NULL;
}
#endif

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
	inline typename std::enable_if<
		isResizable<_Base>::value,
		ArrayExtension<_Base,T>&
	>::type push(std::span<T> b){
		AppendArray(*this,b);
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
	template<typename _Base=Base>
	inline typename std::enable_if<
		isResizable<_Base>::value&&std::is_move_constructible<T>::value,
		T
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
	inline int startsWith(std::span<T> b)const{
		return this->size()>=b.size()&&memcmp(this->data(),b.data(),b.size())==0;
	}
	inline int endsWith(std::span<T> b)const{
		return this->size()>=b.size()&&memcmp(this->data()+(this->size()-b.size()),b.data(),b.size())==0;
	}
	template<typename _T=T>
	inline typename std::enable_if<
		std::is_same<_T,char>::value,
		int
	>::type startsWith(char b)const{
		return this->size()>=1&&(*this)[0]==b;
	}
	template<typename _T=T>
	inline typename std::enable_if<
		std::is_same<_T,char>::value,
		int
	>::type endsWith(char b)const{
		return this->size()>=1&&(*this)[this->size()-1]==b;
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
	inline std::vector<T> concat(Types&&... args)const{
		std::vector<T> ret;
		array_concat_copy_into(ret, *this, std::forward<Types>(args)...);
		return std::move(ret);
	}
	template<typename _Base=Base,typename T1,typename T2,typename... Types>
	inline typename std::enable_if<
		isResizable<_Base>::value,
		ArrayExtension<Base,T>&
	>::type push(T1&& first, T2&& second, Types&&... args){
		this->push(std::forward<T1>(first));
		this->push(std::forward<T2>(second), std::forward<Types>(args)...);
		return *this;
	}
	void sort(){
		std::sort(this->begin(),this->end());
	}
	template<typename F>
	void sort(F const &less){
		std::sort(this->begin(),this->end(),less);
	}
	template<typename F>
	void sortby(F const &toKey){
		struct LessByKey{
			F f;
			inline int operator()(T const &s,T const &t){
				return f(s)<f(t);
			}
		};
		std::sort(this->begin(),this->end(),LessByKey{toKey});
	}
	void unique(){
		auto new_end=std::unique(this->begin(),this->end());
		this->resize(new_end-this->begin());
	}
	template<typename F>
	void unique(F const &equal){
		auto new_end=std::unique(this->begin(),this->end(),equal);
		this->resize(new_end-this->begin());
	}
	template<typename F>
	inline intptr_t bisect(F const &tooSmall){
		intptr_t l=0L;
		intptr_t r=(intptr_t)(this->size()-1);
		while(l<=r){
			intptr_t m=(l+r)>>1;
			if(tooSmall((*this)[m])){
				l=m+1;
			}else{
				r=m-1;
			}
		}
		return l;
	}
	inline intptr_t indexOf(std::span<char> b){
		void* p=memmem(this->data(),this->size(),b.data(),b.size());
		if(p){
			return (char*)p-this->data();
		}else{
			return (intptr_t)(-1);
		}
	}
	#ifdef _MSC_VER
		#undef memmem 
	#endif
	inline intptr_t indexOf(char b)const{
		void* p=(void*)memchr(this->data(),b,this->size());
		if(p){
			return (char*)p-this->data();
		}else{
			return (intptr_t)(-1);
		}
	}
	inline intptr_t lastIndexOf(char b)const{
		#ifdef _MSC_VER
			for(intptr_t i=(intptr_t)this->size()-1;i>=0;--i){
				if(a[i]==b){return i;}
			}
			return -1L;
		#else
			void* p=(void*)memrchr(this->data(),b,this->size());
			if(p){
				return (char*)p-this->data();
			}else{
				return (intptr_t)(-1);
			}
		#endif
	}
};

template<typename Key,typename Value>
class MapExtension:public std::unordered_map<Key,Value>{
public:
	template<typename NewKey>
	inline bool has(NewKey &&k){
		return this->find(std::forward<NewKey>(k))!=this->end();
	}
	template<typename NewKey>
	inline Value const &get(NewKey &&k, Value const& default_value = Value())const{
		auto p_slot=this->find(std::forward<NewKey>(k));
		if(p_slot==this->end()){
			return default_value;
		}else{
			return p_slot->second;
		}
	}
	template<typename NewKey,typename NewValue>
	inline void set(NewKey &&k, NewValue&& v){
		#if __cplusplus>=201703L
			this->insert_or_assign(std::forward<NewKey>(k),std::forward<NewValue>(v));
		#else
			(*this)[std::forward<NewKey>(k)]=std::forward<NewValue>(v);
		#endif
	}
};

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

//example: `std::string("foobar")--->startsWith("foo")`
//JC::GetArrayElementType serves as the SFINAE gimmick, so we can't `auto` the return type
template<typename Array>
JC::ArrayExtension<Array,typename JC::GetArrayElementType<Array>::type>* operator--(Array &a,int){
	return reinterpret_cast<JC::ArrayExtension<Array,typename JC::GetArrayElementType<Array>::type>*>(&a);
}

template<typename Array>
JC::ArrayExtension<Array,typename JC::GetArrayElementType<Array>::type>const* operator--(Array const &a,int){
	return reinterpret_cast<JC::ArrayExtension<Array,typename JC::GetArrayElementType<Array>::type>const*>(&a);
}

template<typename Key,typename Value>
auto operator--(std::unordered_map<Key,Value> &a,int){
	return reinterpret_cast<JC::MapExtension<Key,Value>*>(&a);
}

template<typename Key,typename Value>
auto operator--(std::unordered_map<Key,Value> const&a,int){
	return reinterpret_cast<JC::MapExtension<Key,Value>const*>(&a);
}
#endif
