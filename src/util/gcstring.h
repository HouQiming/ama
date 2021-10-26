#ifndef __GCSTRING_H
#define __GCSTRING_H
#include <stddef.h>
#include <stdint.h>
#include <type_traits>
#include "jc_array.h"

namespace ama{
	//little-endian
	static const uint64_t GCS_PTR_TAG=1uLL<<63;
	//big endian
	//static const uint64_t GCS_PTR_TAG=1uLL;
	struct gcstring_long{
		size_t length;
		gcstring_long* next;
		uint64_t hash;
		char* data()const{
			return (char*)(this+1);
		}
	};
	//we do enough concatenations to warrant an interface like this
	gcstring_long* toGCStringLongCat(char const* data0, size_t length0,char const* data1, size_t length1);
	static inline uint64_t toGCStringValue(char const* data, size_t length){
		if(length<=7){
			union{
				uint64_t v;
				char s[8];
			}u;
			u.v=0;
			memcpy(u.s,data,length);
			return u.v;
		}else{
			return ama::GCS_PTR_TAG^uint64_t(toGCStringLongCat(data,length,"",0));
		}
	}
	struct raw_construction;
	//ama::gc() can free gcstring instances
	//the only recognized root references are those in ama::Node
	struct gcstring{
		//val could pack a 7- byte zero-terminated string 
		uint64_t val=0;
		//////////////
		gcstring():val(0){}
		gcstring(char const* data, size_t size): val(toGCStringValue(data, size)) {}
		gcstring(char const* data, char const* data_end): val(toGCStringValue(data, data_end - data)) {}
		template<size_t size>
		gcstring(char const (&data)[size]): val(toGCStringValue(data, size - 1)) {}
		//gcstring(gcstring &&a): val(a.val) {}
		//gcstring(gcstring const& a): val(a.val) {}
		gcstring(std::string const& a): val(toGCStringValue(a.data(), a.size())) {}
		gcstring(std::span<char> a): val(toGCStringValue(a.data(), a.size())) {}
		//for hacky construction from raw val
		gcstring(uint64_t val,raw_construction*):val(val){}
		gcstring(gcstring_long const* s,raw_construction*):val(GCS_PTR_TAG|uint64_t(s)){}
		//////////////
		gcstring_long const* _as_long()const{
			return (gcstring_long const*)(val^GCS_PTR_TAG);
		}
		char const* data()const{
			return val&GCS_PTR_TAG?this->_as_long()->data():(char*)this;
		}
		char const* c_str()const{
			return this->data();
		}
		size_t size()const{
			if(this->val&GCS_PTR_TAG){
				return this->_as_long()->length;
			}else{
				size_t ret=0;
				uint64_t s=this->val;
				if(s&0xffffffff00000000uLL){s>>=32;ret+=4;}
				if(s&0xffff0000uLL){s>>=16;ret+=2;}
				if(s&0xff00uLL){ret+=1;}
				if(s&0xffuLL){ret+=1;}
				return ret;
			}
		}
		bool empty()const{
			return !this->val;
		}
		char operator[](size_t index)const{
			assert(index < this->size());
			return this->data()[index];
		}
		char back(){
			return this->data()[this->size()-1];
		}
		char const* begin()const{return this->data();}
		char const* end()const{return this->data()+this->size();}
		//////////////
		void gcmark()const{
			if(val&GCS_PTR_TAG){
				((gcstring_long*)(val^GCS_PTR_TAG))->length|=(1uLL<<63);
			}
		}
		//////////////
		//yes, we are copying the argument, it's free anyway
		gcstring operator+(gcstring b){
			size_t length_a=this->size();
			size_t length=length_a+b.size();
			uint64_t val_new{};
			if(length<=7){
				val_new=val|(b.val<<(length_a*8));
			}else{
				val_new=ama::GCS_PTR_TAG^uint64_t(toGCStringLongCat(
					this->data(),length_a,
					b.data(),b.size()
				));
			}
			return gcstring(val_new, (raw_construction*)nullptr);
		}
		//////////////
		operator std::span<char>()const {
			return std::span<char>(this->data(),this->size());
		}
		template<typename T>
		typename std::enable_if<
			std::is_same<typename std::remove_cv<typename std::remove_reference<T>::type>::type, gcstring>::value, 
			bool
		>::type operator==(const T &b)const {
			return this->val == b.val;
		}
		template<typename T>
		typename std::enable_if<
			std::is_same<typename std::remove_cv<typename std::remove_reference<T>::type>::type, gcstring>::value, 
			bool
		>::type operator!=(const T &b)const {
			return this->val != b.val;
		}
		template<size_t size>
		bool operator==(char const (&data)[size])const {
			return this->size() == size - 1 && memcmp(this->data(), data, size - 1) == 0;
		}
		template<size_t size>
		bool operator!=(char const (&data)[size])const {
			return this->size() != size - 1 || memcmp(this->data(), data, size - 1) != 0;
		}
	};
	void gcstring_gcsweep();
	ama::gcstring gcscat(std::span<char> a,std::span<char> b);
}
namespace JC{
	template<>
	struct GetArrayElementType<ama::gcstring> {
		typedef char type;
	};
	static inline char* array_data(const ama::gcstring &a){return (char*)a.data();}
	static inline size_t array_size(const ama::gcstring &a){return a.size();}
}
namespace std {
	template<> struct hash<ama::gcstring> {
		std::size_t operator()(ama::gcstring const& s) const noexcept {
			return hash<uint64_t>()(s.val);
		}
	};
}
static inline ama::gcstring operator""_gc(const char* data, size_t size) {
	return ama::gcstring(data, size);
}
#endif
