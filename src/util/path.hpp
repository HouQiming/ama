#ifndef _PATH_JCH_HPP
#define _PATH_JCH_HPP
//the goal is bug-for-bug node.js compatibility
#include "jc_array.h"
#include <string>
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
	std::string basename(std::span<char> path);
	std::string dirname(std::span<char> path);
	std::string extname(std::span<char> path);
	int isAbsolute(std::span<char> path);
	std::string normalize(std::span<char> path);
	CPathObject parse(std::span<char> path);
	std::string relative(std::span<char> from, std::span<char> _to);
	//toAbsolute implements resolve
	std::string toAbsolute(std::span<char> path);
	struct CPathResolver {
		std::string cur_path{};
		CPathResolver* add(std::span<char> b) {
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
