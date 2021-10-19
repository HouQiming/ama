#include "findama.hpp"
#include <string>
#include "../util/jc_array.h"
namespace ama {
	std::string FindAma(std::span<char> input) {
		std::string ret{};
		//note: we don't want to see the trigger string in our own file
		intptr_t p_ama = input--->indexOf("@a""ma");
		if ( p_ama < intptr_t(0L) ) { return std::move(ret); }
		auto p_prefix = input--->subarray(intptr_t(0L), p_ama)--->lastIndexOf('\n') + intptr_t(1L);
		std::span<char> prefix = input--->subarray(p_prefix, p_ama - p_prefix);
		std::span<char> ama_str = input--->subarray(p_ama);
		intptr_t p_line = ama_str--->indexOf('\n');
		//align lines
		for (int i = 0; i < p_ama; i += 1) {
			if ( input[i] == '\n' ) {
				ret--->push('\n');
			}
		}
		if ( p_line >= intptr_t(0L) ) {
			assert((p_line + intptr_t(1L) - intptr_t(4L)) >= intptr_t(0L));
			ret--->push(ama_str--->subarray(4, p_line + intptr_t(1L) - intptr_t(4L)));
			ama_str = ama_str--->subarray(p_line + intptr_t(1L));
			while ( ama_str.size() > prefix.size() && ama_str--->startsWith(prefix) && 
			ama_str[prefix.size()] != '*' && ama_str[prefix.size()] != '"' && ama_str[prefix.size()] != '\'' ) {
				intptr_t p_nextline = ama_str--->indexOf('\n');
				if ( p_nextline < intptr_t(0L) ) {
					p_nextline = ama_str.size() - intptr_t(1L);
				}
				ret--->push(ama_str--->subarray(prefix.size(), p_nextline + intptr_t(1L) - prefix.size()));
				ama_str = ama_str--->subarray(p_nextline + intptr_t(1L));
			}
		} else {
			ret--->push(ama_str--->subarray(intptr_t(4L)));
		}
		return std::move(ret);
	}
};
