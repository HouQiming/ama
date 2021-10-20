#ifndef __SPAN_POLYFILL_H
#define __SPAN_POLYFILL_H
//std::span polyfill
//we don't support static Extents
#if __cplusplus>=202002L
	#error do not include this in C++20
#endif
#include <stddef.h>
namespace std{
	template<typename T> 
	class span{
	public:
		T* m_data;
		size_t m_size;
		span():m_data(NULL),m_size(0){}
		span(const std::vector<T> &a):m_data((T*)a.data()),m_size(a.size()){}
		//span(const std::shared_ptr<std::vector<T>> &a):m_data((T*)a->data()),m_size(a->size()){}
		//span(const std::unique_ptr<std::vector<T>> &a):m_data((T*)a->data()),m_size(a->size()){}
		template<size_t U>
		span(const std::array<T,U> &a):m_data((T*)a.data()),m_size(U){}
		span(const std::string &a):m_data((T*)a.data()),m_size(a.size()/sizeof(T)){}
		//for string literals only
		template<size_t size>
		span(T const (&data)[size]):m_data((T*)data),m_size((size-1)/sizeof(T)){}
		span(const T* data,size_t size):m_data((T*)data),m_size(size){}
		span(const T* data,const T* data_end):m_data((T*)data),m_size(data_end-data){}
		//template<typename const_char_ptr,typename test=typename std::enable_if<
		//	std::is_same<const_char_ptr,const char*>::value&&std::is_same<T,char>::value
		//>::type>
		//span(const_char_ptr data):m_data((char*)data),m_size(strlen(data)){}
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
}
#endif
