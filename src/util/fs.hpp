#ifndef _FS_JCH_HPP
#define _FS_JCH_HPP
#include <string>
#include <vector>
#include "jc_array.h"
#include "../../modules/cpp/json/json.h"
#pragma add("c_files", "./fs.cpp");
namespace fs {
	struct Dirent {
		std::string name{};
		uint8_t is_file{};
		uint8_t is_dir{};
	};
	#if defined(_WIN32)
		std::vector<uint16_t> PathToWindows(std::span<char> s);
	#endif
	JC::StringOrError readFileSync(std::span<char> fn);
	intptr_t writeFileSync(std::span<char> fn, std::span<char> content);
	int existsSync(std::span<char> fn);
	int DirExists(std::span<char> dir);
	std::string cwd();
	int chdir(std::span<char> dir);
	int mkdirSync(std::span<char> dir);
	intptr_t appendFileSync(std::span<char> fn, std::span<char> content);
	std::vector<Dirent> readdirSync(std::span<char> dir);
	int SyncTimestamp(std::span<char> fn_src, std::span<char> fn_tar);
	void mkdirp(std::span<char> dir);
};
#pragma gen_begin(JSON::stringify<fs::Dirent>)
namespace JSON {
	template <>
	struct StringifyToImpl<fs::Dirent> {
		//`type` is only used for SFINAE
		typedef void type;
		template <typename T = fs::Dirent>
		static void stringifyTo(std::string& buf, fs::Dirent const& a) {
			buf.push_back('{');
			buf.append("\"name\":");
			JSON::stringifyTo(buf, a.name);
			buf.push_back(',');
			buf.append("\"is_file\":");
			JSON::stringifyTo(buf, a.is_file);
			buf.push_back(',');
			buf.append("\"is_dir\":");
			JSON::stringifyTo(buf, a.is_dir);
			buf.push_back('}');
		}
	};
}
#pragma gen_end(JSON::stringify<fs::Dirent>)

#endif
