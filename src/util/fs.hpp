#ifndef _FS_JCH_HPP
#define _FS_JCH_HPP
#include <string>
#include <vector>
#include <memory>
#include "jc_array.h"
/*#pragma add("jc_files", "./fs.jc");*/
namespace fs {
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
};

#endif
