#ifndef _PATH_JCH_HPP
#define _PATH_JCH_HPP
//the goal is bug-for-bug node.js compatibility
#include "jc_platform.h"
#include <string>
#include "../src/util/jc_array.h"
/*#pragma add("jc_files", "./path_win32.jc");*/
/*#pragma add("jc_files", "./path_posix.jc");*/
namespace path {
	struct CPathObject {
		std::string root{};
		std::string dir{};
		std::string base{};
		std::string ext{};
		std::string name{};
	};
	extern char delimiter;
	extern char sep;
	std::string basename(JC::array_base<char> path);
	std::string dirname(JC::array_base<char> path);
	std::string extname(JC::array_base<char> path);
	int isAbsolute(JC::array_base<char> path);
	std::string normalize(JC::array_base<char> path);
	CPathObject parse(JC::array_base<char> path);
	std::string relative(JC::array_base<char> from, JC::array_base<char> _to);
	//toAbsolute implements resolve
	std::string toAbsolute(JC::array_base<char> path);
	struct CPathResolver {
		std::string cur_path{};
		CPathResolver* add(JC::array_base<char> b) {
			if ( isAbsolute(b) ) {
				this->cur_path.clear();
			} else {
				if ( this->cur_path.size() ) {
					this->cur_path--->push(sep);
				}
			}
			this->cur_path--->push(b);
			return this;
		}
		std::string done() const {
			return std::move(toAbsolute(this->cur_path));
		}
	};
};

#endif
