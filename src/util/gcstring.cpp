#include "gcstring.h"

static ama::gcstring_long* g_null_hash=nullptr;
static ama::gcstring_long** g_hash=&g_null_hash;
static size_t g_hash_size=1;
static size_t g_n_elements=0;

//use the same hash as QuickJS
static inline uint64_t hash_string8(uint8_t const* str, size_t len, uint64_t h){
    for(size_t i = 0; i < len; i++){
        h = h * 263 + str[i];
    }
    return h;
}

ama::gcstring_long* ama::toGCStringLongCat(char const* data0, size_t length0,char const* data1, size_t length1){
	size_t length=length0+length1;
	uint64_t h=hash_string8((uint8_t const*)data0,length0,0uLL);
	h=hash_string8((uint8_t const*)data1,length1,h);
	for(ama::gcstring_long* p=g_hash[h&(g_hash_size-1)];p;p=p->next){
		if(p->length==length&&memcmp(p->data(),data0,length0)==0&&memcmp(p->data()+length0,data1,length1)==0){
			return p;
		}
	}
	if(g_n_elements>=g_hash_size){
		//resize
		uint64_t old_size=g_hash_size;
		ama::gcstring_long** old_hash=g_hash;
		uint64_t new_size=old_size*2;
		if(new_size<512){new_size=512;}
		ama::gcstring_long** new_hash=(ama::gcstring_long**)calloc(sizeof(ama::gcstring_long*),g_hash_size);
		if(new_hash){
			for(uint64_t i=0;i<old_size;i++){
				ama::gcstring_long* p=old_hash[i];
				while(p){
					ama::gcstring_long* p_next=p->next;
					uint64_t hp=p->hash&(new_size-1);
					p->next=new_hash[hp];
					new_hash[hp]=p;
					p=p_next;
				}
			}
			if(old_size>1){
				free(old_hash);
				old_hash=nullptr;
			}
			g_hash=new_hash;
			g_hash_size=new_size;
		}else{
			//just give up, do nothing
		}
	}
	g_n_elements+=1;
	uint64_t hp=h&(g_hash_size-1);
	ama::gcstring_long* p_new=(ama::gcstring_long*)malloc(sizeof(ama::gcstring_long)+length+1);
	p_new->length=length;
	p_new->next=g_hash[hp];
	p_new->hash=h;
	memcpy(p_new->data(),data0,length0);
	memcpy(p_new->data()+length0,data1,length1);
	p_new->data()[length]=0;
	g_hash[hp]=p_new;
	return p_new;
}

void ama::gcstring_gcsweep(){
	uint64_t old_size=g_hash_size;
	ama::gcstring_long** old_hash=g_hash;
	for(uint64_t i=0;i<old_size;i++){
		ama::gcstring_long** pp_last=&old_hash[i];
		ama::gcstring_long* p=old_hash[i];
		while(p){
			ama::gcstring_long* p_next=p->next;
			if(p->length&(1uLL<<63)){
				//keep it
				p->length&=~(1uLL<<63);
				*pp_last=p;
				pp_last=&p->next;
			}else{
				//drop it
				free(p);
			}
			p=p_next;
		}
		*pp_last=nullptr;
	}
}
