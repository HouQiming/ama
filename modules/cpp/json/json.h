#ifndef __JSON_CPP_HPP
#define __JSON_CPP_HPP
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cmath>
#include <cstdlib>
#include "../../../src/util/jc_array.h"
#include "../../../src/util/jc_unique_string.h"
#ifndef NDEBUG
#include <stdio.h>
#endif

namespace JSON{
	/////////////
	//general version
	template<typename T>
	struct StringifyToImpl{
		//nothing
	};
	template<typename T>
	static inline typename JSON::StringifyToImpl<T>::type stringifyTo(std::string& buf, T const& a){
		JSON::StringifyToImpl<T>::stringifyTo(buf,a);
	}
	void stringifyTo(std::string &buf,std::span<char> a);
	void stringifyTo(std::string &buf,int64_t a);
	void stringifyTo(std::string &buf,uint64_t a);
	void stringifyTo(std::string &buf,double a);
	//reduce the priority of bool: too many other types can cast to it
	template<typename T>
	typename std::enable_if<std::is_same<T,bool>::value,void>::type stringifyTo(std::string &buf,T a){
		if(a){buf+="true";}else{buf+="false";}
	}
	template<typename T>
	typename std::enable_if<std::is_same<T,JC::unique_string>::value,void>::type stringifyTo(std::string &buf,T const& a){
		if(a.p){stringifyTo(buf,std::span<char>(a));}else{buf+="null";}
	}
	static inline void stringifyTo(std::string &buf,const char* a){stringifyTo(buf,std::span<char>(a));}
	static inline void stringifyTo(std::string &buf,int32_t a){stringifyTo(buf,(int64_t)a);}
	static inline void stringifyTo(std::string &buf,int16_t a){stringifyTo(buf,(int64_t)a);}
	static inline void stringifyTo(std::string &buf,int8_t a){stringifyTo(buf,(int64_t)a);}
	static inline void stringifyTo(std::string &buf,uint32_t a){stringifyTo(buf,(uint64_t)a);}
	static inline void stringifyTo(std::string &buf,uint16_t a){stringifyTo(buf,(uint64_t)a);}
	static inline void stringifyTo(std::string &buf,uint8_t a){stringifyTo(buf,(uint64_t)a);}
	static inline void stringifyTo(std::string &buf,float a){stringifyTo(buf,(double)a);}
	template<class T>
	typename std::enable_if<!std::is_same<T,char>::value,void>::type stringifyTo(std::string &buf,const T* a);
	template<class T>
	void stringifyTo(std::string &buf,const std::shared_ptr<T>& a);
	template<typename T>
	void stringifyTo(std::string &buf,std::span<T> a){
		buf.push_back('[');
		for(size_t i=0;i<a.size();i++){
			if(i){buf.push_back(',');}
			stringifyTo(buf,a[i]);
		}
		buf.push_back(']');
	}
	template<typename T>
	void stringifyTo(std::string &buf,const std::vector<T>& a){
		buf.push_back('[');
		for(size_t i=0;i<a.size();i++){
			if(i){buf.push_back(',');}
			stringifyTo(buf,a[i]);
		}
		buf.push_back(']');
	}
	template<typename T,size_t U>
	void stringifyTo(std::string &buf,const std::array<T,U>& a){
		buf.push_back('[');
		for(size_t i=0;i<U;i++){
			if(i){buf.push_back(',');}
			stringifyTo(buf,a[i]);
		}
		buf.push_back(']');
	}
	template<typename String,typename T>
	static inline typename std::enable_if<std::is_convertible<char const*,String>::value,void>::type
	stringifyTo(std::string &buf,const std::unordered_map<String,T>& a){
		buf.push_back('{');
		int is_first=1;
		for(auto const &i:a){
			if(is_first){
				is_first=0;
			}else{
				buf.push_back(',');
			}
			stringifyTo(buf,i.first);
			buf.push_back(':');
			stringifyTo(buf,i.second);
		}
		buf.push_back('}');
	}
	template<class T>
	typename std::enable_if<!std::is_same<T,char>::value,void>::type stringifyTo(std::string &buf,const T* a){
		if(a){
			::JSON::stringifyTo(buf,*a);
		}else{
			buf+="null";
		}
	}
	template<class T>
	void stringifyTo(std::string &buf,const std::shared_ptr<T>& a){
		if(a){
			::JSON::stringifyTo(buf,*a);
		}else{
			buf+="null";
		}
	}
	/////////////
	template<typename T>
	std::string stringify(const T &a){
		std::string buf;
		::JSON::stringifyTo(buf,a);
		return buf;
	}
	/////////////////////////////////////////////////////////////////
	struct JSONParserContext{
		const char* begin;
		const char* end;
		const char* error;
		const char* error_location;
		void SkipNumber();
		void SkipSpace();
		void SkipStringBody();
		void SkipField();
		void SkipColon();
		template<size_t size>
		inline bool TrySkipName(char const (&data)[size]){
			if((intptr_t)(end-begin)>=(intptr_t)size&&memcmp(begin,data,size-1)==0){
				begin+=size-1;
				return true;
			}else{
				return false;
			}
		}
	};
	//////////////
	//the empty template
	//general version
	template<typename T>
	struct ParseFromImpl{
		//nothing
	};
	template<typename T,typename U=typename JSON::ParseFromImpl<T>::type>
	static inline auto parseFrom(JSONParserContext& ctx,T** dummy_ptr){
		return JSON::ParseFromImpl<T>::parseFrom(ctx,dummy_ptr);
	}
	//basic types
	double parseFrom(JSONParserContext& ctx,double**);
	float parseFrom(JSONParserContext& ctx,float**);
	bool parseFrom(JSONParserContext& ctx,bool**);
	template<typename T>
	static inline typename std::enable_if<std::is_convertible<int64_t,T>::value&&std::is_integral<T>::value&&std::is_signed<T>::value,T>::type
	parseFrom(JSONParserContext& ctx,T**){
		const char* s0=ctx.begin;
		ctx.SkipNumber();
		return T(strtoll(s0,NULL,10));
	}
	template<typename T>
	static inline typename std::enable_if<std::is_convertible<int64_t,T>::value&&std::is_integral<T>::value&&std::is_unsigned<T>::value,T>::type
	parseFrom(JSONParserContext& ctx,T**){
		const char* s0=ctx.begin;
		ctx.SkipNumber();
		return T(strtoull(s0,NULL,10));
	}
	//strings
	std::string parseFrom(JSONParserContext& ctx,std::string**);
	static inline JC::unique_string parseFrom(JSONParserContext& ctx,JC::unique_string**){
		if(ctx.begin!=ctx.end&&*ctx.begin=='n'){
			ctx.SkipNumber();
			return JC::unique_string(nullptr);
		}else{
			return JC::unique_string(parseFrom(ctx,(std::string**)NULL));
		}
	}
	//forward decl for overloads
	template<typename T>
	std::shared_ptr<T> parseFrom(JSONParserContext& ctx,std::shared_ptr<T>**);
	template<typename T>
	std::unique_ptr<T> parseFrom(JSONParserContext& ctx,std::unique_ptr<T>**);
	template<typename T>
	std::vector<T> parseFrom(JSONParserContext& ctx,std::vector<T>**);
	//pointers
	template<typename T>
	std::shared_ptr<T> parseFrom(JSONParserContext& ctx,std::shared_ptr<T>**){
		const char* s0=ctx.begin;
		int ch=*s0;
		if(ch=='n'){
			ctx.SkipNumber();
			return nullptr;
		}else{
			return std::make_shared<T>(parseFrom(ctx,(typename std::remove_const<T>::type**)NULL));
		}
	}
	template<typename T>
	std::unique_ptr<T> parseFrom(JSONParserContext& ctx,std::unique_ptr<T>**){
		const char* s0=ctx.begin;
		int ch=*s0;
		if(ch=='n'){
			ctx.SkipNumber();
			return nullptr;
		}else{
			return std::make_unique<T>(parseFrom(ctx,(typename std::remove_const<T>::type**)NULL));
		}
	}
	//arrays
	template<typename T>
	std::vector<T> parseFrom(JSONParserContext& ctx,std::vector<T>**){
		std::vector<T> ret;
		ctx.SkipSpace();
		if(ctx.begin==ctx.end){
			return std::move(ret);
		}
		if(*ctx.begin!='['){
			ctx.error="'[' expected";
			ctx.error_location=ctx.begin;
			return std::move(ret);
		}
		ctx.begin+=1;
		ctx.SkipSpace();
		while(ctx.begin!=ctx.end&&*ctx.begin!=']'){
			ctx.SkipSpace();
			ret.push_back(parseFrom(ctx,(T**)NULL));
			if(ctx.error){return std::move(ret);}
			ctx.SkipSpace();
			if(ctx.begin!=ctx.end&&(*ctx.begin==','||*ctx.begin==']')){
				if(*ctx.begin==','){
					ctx.begin+=1;
				}
			}else{
				ctx.error="',' or ']' expected";
				ctx.error_location=ctx.begin;
				return std::move(ret);
			}
		}
		if(ctx.begin!=ctx.end){
			ctx.begin+=1;
		}
		return std::move(ret);
	}
	//map with string key
	template<typename String,typename T>
	static inline typename std::enable_if<std::is_convertible<char const*,String>::value,std::unordered_map<String,T>>::type
	parseFrom(JSONParserContext& ctx,std::unordered_map<String,T>**){
		std::unordered_map<String,T> ret;
		ctx.SkipSpace();
		if(ctx.begin==ctx.end){
			return std::move(ret);
		}
		if(*ctx.begin!='{'){
			ctx.error="'{' expected";
			ctx.error_location=ctx.begin;
			return std::move(ret);
		}
		ctx.begin+=1;
		ctx.SkipSpace();
		while(ctx.begin!=ctx.end&&*ctx.begin!='}'){
			std::string&& key=parseFrom(ctx,(std::string**)NULL);
			ctx.SkipColon();
			if(ctx.error){return std::move(ret);}
			ret[key]=parseFrom(ctx,(T**)NULL);
			if(ctx.error){return std::move(ret);}
			ctx.SkipSpace();
			if(ctx.begin!=ctx.end&&(*ctx.begin==','||*ctx.begin=='}')){
				if(*ctx.begin==','){
					ctx.begin+=1;
					ctx.SkipSpace();
				}
			}else{
				ctx.error="',' or '}' expected";
				ctx.error_location=ctx.begin;
				return std::move(ret);
			}
		}
		if(ctx.begin!=ctx.end){
			ctx.begin+=1;
		}
		return std::move(ret);
	}
	//class parser will be generated in the macro
	//COULDDO: JSValue, unordered_map
	template<typename ReturnType>
	static inline ReturnType parse(std::span<char> s){
		JSONParserContext ctx{s.begin(),s.end(),NULL,NULL};
		ReturnType &&ret=parseFrom(ctx,(typename std::remove_const<ReturnType>::type**)NULL);
		#ifndef NDEBUG
			if(ctx.error){
				fprintf(stderr,"JSON error at %d: %s\n",(int)(ctx.error_location-s.begin()),ctx.error);
			}
		#endif
		return std::move(ret);
	}
}

#endif
