#ifndef _FS_JCH_HPP
#define _FS_JCH_HPP
#include <string>
#include <vector>
#include "../src/util/jc_array.h"
#include <memory>
#include <functional>
/*#pragma add("jc_files", "./fs.jc");*/
namespace fs {
	#if JC_OS == JC_OS_WINDOWS
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
