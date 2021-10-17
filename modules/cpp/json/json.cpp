#include "json.h"
#include <stdio.h>

//COULDDO: more error checks
namespace JSON{
	static const char g_json_escape_lut[256]={
		///// 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 ,  8 , 9 , a , b , c  , d , e , f
		/*0*/ 1 , 1 , 1 , 1 , 1 , 1 , 1 ,'a', 'b','t','n','v','f' ,'r', 1 , 1 ,
		/*1*/ 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,  1 , 1 , 1 , 1 , 1  , 1 , 1 , 1 ,
		/*2*/ 0 , 0 ,'"', 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 ,
		/*3*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 ,
		/*4*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 ,
		/*5*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,'\\', 0 , 0 , 0 ,
		/*6*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 ,
		/*7*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 1 ,
		///// /////////////////////////////////////////////////////////////////
		/*8*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 ,
		/*9*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 , 
		/*a*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 , 
		/*b*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 , 
		/*c*/ 1 , 1 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 , 
		/*d*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 , 
		/*e*/ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 , 0  , 0 , 0 , 0 , 
		/*f*/ 0 , 0 , 0 , 0 , 0 , 1 , 1 , 1 ,  1 , 1 , 1 , 1 , 1  , 1 , 1 , 1 , 
	};
	
	static const char g_hex_chars[17]="0123456789abcdef";
	
	void stringifyTo(std::string &buf,JC::array_base<char> a){
		buf.push_back('"');
		for(size_t i=0;i<a.size();i++){
			char ch=a[i];
			char flag=g_json_escape_lut[(unsigned char)ch];
			if(flag){
				if(flag==1){
					buf.push_back('\\');
					buf.push_back('u');
					buf.push_back('0');
					buf.push_back('0');
					buf.push_back(g_hex_chars[(ch>>4)&15]);
					buf.push_back(g_hex_chars[ch&15]);
				}else{
					buf.push_back('\\');
					buf.push_back(flag);
				}
			}else{
				buf.push_back(ch);
			}
		}
		buf.push_back('"');
	}
	
	void stringifyTo(std::string &buf,int64_t a){
		char tmp[24];
		snprintf(tmp,23,"%lld",(long long)a);
		buf+=tmp;
	}
	
	void stringifyTo(std::string &buf,uint64_t a){
		char tmp[24];
		snprintf(tmp,23,"%llu",(unsigned long long)a);
		buf+=tmp;
	}
	
	void stringifyTo(std::string &buf,double a){
		char tmp[32];
		snprintf(tmp,31,"%.17g",a);
		buf+=tmp;
	}
	
	double parseFrom(JSONParserContext& ctx,double**){
		const char* s0=ctx.begin;
		ctx.SkipNumber();
		int ch=*s0;
		if(ch=='n'){
			//null, which is NaN
			return nan(""); 
		}else{
			return strtod(s0,NULL);
		}
	}
	
	float parseFrom(JSONParserContext& ctx,float**){
		const char* s0=ctx.begin;
		ctx.SkipNumber();
		int ch=*s0;
		if(ch=='n'){
			//null, which is NaN
			return nanf(""); 
		}else{
			return strtof(s0,NULL);
		}
	}
	
	bool parseFrom(JSONParserContext& ctx,bool**){
		const char* s0=ctx.begin;
		ctx.SkipNumber();
		return *s0=='t';
	}
	
	std::string parseFrom(JSONParserContext& ctx,std::string**){
		std::string ret;
		const char* s0=ctx.begin;
		if(s0!=ctx.end&&*s0!='"'){
			ctx.error="'\"' expected";
			ctx.error_location=s0;
			return std::move(ret);
		}
		s0+=1;
		bool in_slash=false;
		while(s0!=ctx.end){
			char ch=*s0;
			if(in_slash){
				switch(ch){
				default: {ret.push_back(ch);break;}
				case 'a': {ret.push_back('\a');break;}
				case 'b': {ret.push_back('\b');break;}
				case 't': {ret.push_back('\t');break;}
				case 'n': {ret.push_back('\n');break;}
				case 'v': {ret.push_back('\v');break;}
				case 'f': {ret.push_back('\f');break;}
				case 'r': {ret.push_back('\r');break;}
				case 'u': {
					if(ctx.end-s0>=5){
						char hex[5];
						char* dummy_end=NULL;
						memcpy(hex,s0+1,4);
						hex[4]=0;
						int chi=(int)strtol(hex,&dummy_end,16);
						if( chi >= 65536 ) {
							ret.push_back(char((chi >> 18 & 0xf) + 0xf0));
							ret.push_back(char(0x80 + (chi >> 12 & 63)));
							ret.push_back(char(0x80 + (chi >> 6 & 63)));
							ret.push_back(char(0x80 + (chi & 63)));
						} else if( chi >= 2048 ) {
							ret.push_back(char((chi >> 12 & 0xf) + 0xe0));
							ret.push_back(char(0x80 + (chi >> 6 & 63)));
							ret.push_back(char(0x80 + (chi & 63)));
						} else if( chi >= 128 ) {
							ret.push_back(char((chi >> 6) + 0xc0));
							ret.push_back(char(0x80 + (chi & 63)));
						} else {
							ret.push_back(char(chi));
						}
						s0+=4;
					}
					break;
				}
				}
				in_slash=false;
			}else if(ch=='\\'){
				in_slash=true;
			}else{
				if(ch=='"'){
					s0+=1;
					break;
				}
				ret.push_back(ch);
			}
			s0+=1;
		}
		ctx.begin=s0;
		return std::move(ret);
	}
	
	void JSONParserContext::SkipNumber(){
		while(begin!=end){
			int ch=begin[0];
			if(ch==','||ch=='}'||ch==']'||ch=='"'){
				break;
			}
			begin+=1;
		}
	}

	void JSONParserContext::SkipSpace(){
		while(begin!=end&&*(unsigned char*)begin<=(unsigned char)' '){
			begin+=1;
		}
	}

	void JSONParserContext::SkipStringBody(){
		std::string ret;
		bool in_slash=false;
		while(this->begin!=this->end){
			char ch=*this->begin;
			if(in_slash){
				in_slash=false;
			}else if(ch=='\\'){
				in_slash=true;
			}else{
				if(ch=='"'){
					this->begin+=1;
					break;
				}
			}
			this->begin+=1;
		}
	}

	void JSONParserContext::SkipField(){
		int level=0;
		while(this->begin!=this->end){
			if(*this->begin=='"'){
				this->begin+=1;
				SkipStringBody();
			}else{
				if(*this->begin=='{'||*this->begin=='['){
					level+=1;
				}else if(*this->begin==']'||*this->begin=='}'){
					level-=1;
					if(level<0){break;}
				}else if(!level&&*this->begin==','){
					break;
				}
				this->begin+=1;
			}
		}
	}
	
	void JSONParserContext::SkipColon(){
		this->SkipSpace();
		if(this->begin!=this->end&&*this->begin==':'){
			this->begin+=1;
			this->SkipSpace();
		}else{
			this->error="':' expected";
			this->error_location=this->begin;
		}
	}
}
