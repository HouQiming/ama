#ifndef __UNIQUE_STRING_H
#define __UNIQUE_STRING_H
#include "jc_array.h"

namespace JC {

///////////////////////
//unique_string
//the idiosyncratic way: std::unordered_map<array_base<char>,std::shared_ptr<std::string>>
//casting to std::string isn't the best idea: unique_string content is immutable
//just port the old string_interned
//use C++17 string_view for hashing: __cplusplus>=201703L
class string_interned: public array_base<const char> {public:
	size_t m_hash;
	//rc is hardcoded as the last field in object.jch
	intptr_t rc;
	string_interned(): array_base<const char>(NULL, (size_t)0) {}
	//the string data is stored just after this thing
	//the actual allocation is length+1 aligned to sizeof(intptr_t)
	////
	//const char* data(){
	//	return (const char*)(this+1);
	//}
	operator array_base<char>&() {
		return *(array_base<char>*)this;
	}
};

//increases rc
string_interned* intern_string(const char* data, size_t size);
void free_interned_string(string_interned* p);

struct unique_string {
public:
	string_interned * p;
	unique_string(): p(NULL) {}
	unique_string(string_interned* p0): p(p0) {if (p0) {p0->rc += 1;}}
	unique_string(const char* data, size_t size): p(intern_string(data, size)) {}
	unique_string(const char* data, const char* data_end): p(intern_string(data, data_end - data)) {}
	template<size_t size>
	unique_string(char const (&data)[size]): p(intern_string(data, size - 1)) {}
	unique_string(const unique_string &a): p(a.p) {
		if (p) {p->rc += 1;}
	}
	unique_string(const unique_string &&a): p(a.p) {
		((unique_string&)a).p = NULL;
	}
	//unique_string(char a):p(intern_string(&a,1)){}
	template < typename T, typename test = typename std::enable_if <
		std::is_same<typename std::remove_cv<typename std::remove_reference<decltype((new typename std::remove_reference<T>::type[1])[0][0])>::type>::type, char>::value && !std::is_same<typename std::remove_cv<typename std::remove_reference<T>::type>::type, unique_string>::value
	> ::type >
	unique_string(T&& a): p(intern_string(JC::array_data(a), JC::array_size(a))) {}
	unique_string(std::nullptr_t): p(nullptr) {}
	~unique_string() {
		if (p && --p->rc <= 0) {
			free_interned_string(p);
		}
	}
	//you cannot change the characters of a unique string
	const char operator[](intptr_t index)const {
		assert(size_t(index) < p->m_size);
		return p->data()[index];
	}
	const char* c_str()const {return p->data();}
	const char* data()const {return p->data();}
	size_t size()const {return p->size();}
	char* begin()const {return (char*)p->begin();}
	char* end()const {return (char*)p->end();}
	operator array_base<char>&()const {
		return *(array_base<char>*)p;
	}
	template<typename T>
	typename std::enable_if<
			std::is_same<typename std::remove_cv<typename std::remove_reference<T>::type>::type, unique_string>::value, int>::type operator==(const T &b)const {
		return this->p == b.p;
	}
	template<typename T>
	typename std::enable_if<
			std::is_same<typename std::remove_cv<typename std::remove_reference<T>::type>::type, unique_string>::value, int>::type operator!=(const T &b)const {
		return this->p != b.p;
	}
	int operator==(std::nullptr_t)const {
		return this->p == NULL;
	}
	int operator!=(std::nullptr_t)const {
		return this->p != NULL;
	}
	template<size_t size>
	int operator==(char const (&data)[size])const {
		return p->size() == size - 1 && memcmp(p->data(), data, size - 1) == 0;
	}
	template<size_t size>
	int operator!=(char const (&data)[size])const {
		return p->size() != size - 1 || memcmp(p->data(), data, size - 1) != 0;
	}
	//operator const char*(){
	//	return this->c_str();
	//}
	//operator bool(){
	//	return p!=NULL;
	//}
	unique_string & operator=(const unique_string &b) {
		//they could be the same string
		if (b.p) {b.p->rc += 1;}
		if (p && --p->rc <= 0) {
			free_interned_string(p);
		}
		p = b.p;
		return *this;
	}
	unique_string & operator=(unique_string &&b) {
		if (p && --p->rc <= 0) {
			free_interned_string(p);
		}
		p = b.p;
		b.p = NULL;
		return *this;
	}
	unique_string & operator=(const std::nullptr_t) {
		if (p && --p->rc <= 0) {
			free_interned_string(p);
		}
		p = NULL;
		return *this;
	}
	//const unique_string* operator->()const{
	//	return this;
	//}
};

template<>
struct GetArrayElementType<unique_string> {
	typedef char type;
};

static inline unique_string operator""_u(const char* data, size_t size) {
	return unique_string(data, size);
}

static inline char* array_data(const JC::unique_string &a) {return (char*)a.data();}
static inline size_t array_size(const JC::unique_string &a) {return a.size();}

}

namespace std {
	template<> struct hash<JC::string_interned> {
		std::size_t operator()(JC::string_interned const& s) const noexcept {
			return s.m_hash;
		}
	};
	template<> struct hash<JC::string_interned*> {
		std::size_t operator()(JC::string_interned* s) const noexcept {
			return s->m_hash;
		}
	};
	template<> struct hash<JC::unique_string> {
		std::size_t operator()(JC::unique_string const& s) const noexcept {
			return s.p ? s.p->m_hash : 0;
		}
	};
}
#endif
