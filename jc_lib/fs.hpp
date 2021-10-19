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
		std::vector<uint16_t> PathToWindows(JC::array_base<char> s);
	#endif
	std::shared_ptr<std::string> readFileSync(JC::array_base<char> fn);
	intptr_t writeFileSync(JC::array_base<char> fn, JC::array_base<char> content);
	int existsSync(JC::array_base<char> fn);
	int DirExists(JC::array_base<char> dir);
	std::string cwd();
	int chdir(JC::array_base<char> dir);
	int mkdirSync(JC::array_base<char> dir);
	intptr_t appendFileSync(JC::array_base<char> fn, JC::array_base<char> content);
};

#endif
