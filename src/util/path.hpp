#ifndef _PATH_JCH_HPP
#define _PATH_JCH_HPP
#include <string>
#include "jc_array.h"
#include "../../modules/cpp/json/json.h"
#pragma add("c_files", "./path_win32.cpp");
#pragma add("c_files", "./path_posix.cpp");
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
#pragma gen_begin(JSON::stringify<path::CPathObject>)
namespace JSON {
	template <>
	struct StringifyToImpl<path::CPathObject> {
		//`type` is only used for SFINAE
		typedef void type;
		template <typename T = path::CPathObject>
		static void stringifyTo(std::string& buf, path::CPathObject const& a) {
			buf.push_back('{');
			buf.append("\"root\":");
			JSON::stringifyTo(buf, a.root);
			buf.push_back(',');
			buf.append("\"dir\":");
			JSON::stringifyTo(buf, a.dir);
			buf.push_back(',');
			buf.append("\"base\":");
			JSON::stringifyTo(buf, a.base);
			buf.push_back(',');
			buf.append("\"ext\":");
			JSON::stringifyTo(buf, a.ext);
			buf.push_back(',');
			buf.append("\"name\":");
			JSON::stringifyTo(buf, a.name);
			buf.push_back('}');
		}
	};
}
#pragma gen_end(JSON::stringify<path::CPathObject>)

#endif
