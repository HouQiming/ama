#pragma no_auto_header()
#if defined(_WIN32)
	#include "path.hpp"
	#include "unicode/case.hpp"
	#include "fs.hpp"
	#include "env.hpp"
	namespace path {
		char delimiter = ';';
		char sep = '\\';
		std::string basename(std::span<char> s_path) {
			//remove trailing seps
			intptr_t pend = s_path.size();
			for (; pend > intptr_t(0L); pend--) {
				if ( !(s_path[pend - 1] == '/' || s_path[pend - 1] == '\\') ) {
					break;
				}
			}
			//find the last non-empty part
			intptr_t p = pend - 1;
			for (; p >= intptr_t(0L); p--) {
				if ( s_path[p] == '/' || s_path[p] == '\\' ) {
					break;
				}
			}
			p += 1;
			if ( !p && s_path.size() >= 2 && s_path[1] == ':' && ((s_path[0] >= 'A' && s_path[0] <= 'Z') || (s_path[0] >= 'a' && s_path[0] <= 'z')) ) {
				//drive letter doesn't count
				p = intptr_t(2L);
			}
			return JC::array_cast<std::string>(s_path--->subarray(p, pend - p));
		}
		std::string extname(std::span<char> s_path) {
			//again ignores trailing seps
			//remove trailing seps
			intptr_t pend = s_path.size();
			for (; pend > intptr_t(0L); pend--) {
				if ( !(s_path[pend - 1] == '/' || s_path[pend - 1] == '\\') ) {
					break;
				}
			}
			//find the last non-empty part
			intptr_t p = pend - 1;
			for (; p >= intptr_t(0L); p--) {
				if ( s_path[p] == '/' || s_path[p] == '\\' ) {
					p = pend;
					break;
				}
				if ( s_path[p] == '.' ) {
					break;
				}
			}
			if ( p < 0 ) {
				p = pend;
			}
			//but . and .. don't count, so we need to check
			if ( !p || s_path[p - 1] == '/' || s_path[p - 1] == '\\' || (pend == (p + 1) && s_path[p - 1] == '.' && (p == 1 || (s_path[p - 2] == '/' || s_path[p - 2] == '\\'))) ) {
				p = pend;
			}
			return JC::array_cast<std::string>(s_path--->subarray(p, pend - p));
		}
		std::string dirname(std::span<char> s_path) {
			//remove trailing seps
			intptr_t pend = s_path.size();
			for (; pend > intptr_t(0L); pend--) {
				if ( !(s_path[pend - 1] == '/' || s_path[pend - 1] == '\\') ) {
					break;
				}
			}
			//find the last non-empty part
			intptr_t p = pend - 1;
			for (; p >= intptr_t(0L); p--) {
				if ( s_path[p] == '/' || s_path[p] == '\\' ) {
					break;
				}
			}
			p += 1;
			if ( p <= intptr_t(3L) && s_path.size() >= 2 && s_path[1] == ':' && ((s_path[0] >= 'A' && s_path[0] <= 'Z') || (s_path[0] >= 'a' && s_path[0] <= 'z')) ) {
				//the drive letter is a part of the dir - we need to handle things like 'c:abc'
				p = intptr_t(2L);
				//if a \\ follows the drive letter, we need to put it in
				if ( p < intptr_t(s_path.size()) && (s_path[intptr_t(2L)] == '/' || s_path[intptr_t(2L)] == '\\') ) {
					p = intptr_t(3L);
				}
			} else if ( p > 1 ) {
				//we need the trailing / if it's root
				//so if it isn't, we need to remove it
				p--;
			}
			return JC::array_cast<std::string>(s_path--->subarray(0, p));
		}
		int isAbsolute(std::span<char> s_path) {
			if ( s_path.size() >= 3 && s_path[1] == ':' && ((s_path[0] >= 'A' && s_path[0] <= 'Z') || (s_path[0] >= 'a' && s_path[0] <= 'z')) && (s_path[2] == '/' || s_path[2] == '\\') ) {
				//'c:' is the current working directory of C:, which is NOT absolute
				return 1;
			} else if ( s_path.size() >= 1 && (s_path[0] == '/' || s_path[0] == '\\') ) {
				return 1;
			} else {
				return 0;
			}
		}
		std::string normalize(std::span<char> s_path) {
			//split and resolve . / .. / ''
			std::string ret{};
			std::vector<size_t> last_levels{};
			//add the parts
			std::span<char> s_drive_letter((""));
			int32_t need_trailing_slash = 0;
			int32_t is_first = 1;
			{
				{
					//coulddo: SSE / NEON vectorization
					size_t I0 = intptr_t(0L);
					for (size_t I = 0; I <= s_path.size(); ++I) {
						//char ch=(I<str.size()?str[I]:0)
						if ( I >= s_path.size() || s_path[I] == '/' || s_path[I] == '\\' ) {
							//this scope will be if-wrapped
							size_t I00 = I0;
							I0 = I + 1;
							//the return will be replaced
							std::span<char> part(s_path.data() + I00, I - I00);
							if ( part == "" ) {
								is_first = 0;
								if ( !ret.size() ) {
									ret--->push('\\');
								} else {
									need_trailing_slash = 1;
								}
							} else if ( part == "." ) {
								is_first = 0;
								need_trailing_slash = 0;
							} else if ( part == ".." ) {
								is_first = 0;
								need_trailing_slash = 0;
								if ( last_levels.size() ) {
									ret.resize(last_levels--->pop());
								} else {
									//don't push last_levels
									ret--->push("..\\");
								}
							} else {
								if ( is_first && part.size() >= 2 && part[1] == ':' && ((part[0] >= 'A' && part[0] <= 'Z') || (part[0] >= 'a' && part[0] <= 'z')) ) {
									//we got a drive letter WITHOUT // or \\, it's drive-current-relative s_path, pull the drive letter out
									is_first = 0;
									s_drive_letter = part--->subarray(0, 2);
									if ( part.size() == 3 && part[2] == '.' ) {
										//eat the dot, do nothing
									} else {
										if ( part.size() > 2 ) {
											last_levels--->push(ret.size());
											ret--->push(part--->subarray(2));
										}
										ret--->push('\\');
									}
									continue;
								}
								is_first = 0;
								need_trailing_slash = 0;
								last_levels--->push(ret.size());
								ret--->push(part);
								ret--->push('\\');
							}
						}
					}
				}
			}
			if ( !need_trailing_slash && ret.size() && ret.back() == '\\' ) {
				ret.pop_back();
			} else if ( need_trailing_slash && (!ret.size() || ret.back() != '\\') ) {
				ret--->push('\\');
			}
			return JC::string_concat(s_drive_letter, ret);
		}
		path::CPathObject parse(std::span<char> s_path) {
			//compute the dir-basename boundary
			//remove trailing seps
			intptr_t pend = s_path.size();
			for (; pend > intptr_t(0L); pend--) {
				if ( !(s_path[pend - 1] == '/' || s_path[pend - 1] == '\\') ) {
					break;
				}
			}
			//find the last non-empty part
			intptr_t pbase = pend - 1;
			for (; pbase >= intptr_t(0L); pbase--) {
				if ( s_path[pbase] == '/' || s_path[pbase] == '\\' ) {
					break;
				}
			}
			intptr_t pend_dir = pbase < 0 ? 0 : pbase;
			if ( pend_dir == 0 && s_path.size() > 0 && (s_path[0] == '/' || s_path[0] == '\\') ) {
				pend_dir = 1;
			}
			pbase += 1;
			if ( !pbase && s_path.size() >= 2 && s_path[1] == ':' && ((s_path[0] >= 'A' && s_path[0] <= 'Z') || (s_path[0] >= 'a' && s_path[0] <= 'z')) ) {
				//drive letter doesn't count as basename
				pbase = intptr_t(2L);
				pend_dir = intptr_t(2L);
				if ( s_path.size() >= 3 && (s_path[2] == '/' || s_path[2] == '\\') ) {
					pbase = intptr_t(3L);
					pend_dir = intptr_t(3L);
					if ( pend < intptr_t(3L) ) {
						pend = intptr_t(3L);
					}
				}
			}
			//compute the root boundary
			//root could be c: c:/ /
			intptr_t pend_root = intptr_t(0L);
			if ( pend_dir >= 2 && s_path[1] == ':' && ((s_path[0] >= 'A' && s_path[0] <= 'Z') || (s_path[0] >= 'a' && s_path[0] <= 'z')) ) {
				//we got a drive letter
				pend_root = intptr_t(2L);
				if ( s_path.size() >= intptr_t(3L) && (s_path[2] == '/' || s_path[2] == '\\') ) {
					if ( pend_dir < intptr_t(3L) ) {
						pend_dir = intptr_t(3L);
					}
					pend_root = intptr_t(3L);
				}
			} else if ( pend_dir > 0 && (s_path[0] == '/' || s_path[0] == '\\') ) {
				pend_root = intptr_t(1L);
			}
			//compute the name-ext boundary
			intptr_t pdot = pend;
			intptr_t p = pend - 1;
			for (; p >= pbase; p--) {
				if ( s_path[p] == '.' ) {
					pdot = p;
					break;
				}
			}
			//. and .. don't count, starting . don't count, so we need to check
			if ( !pdot || s_path[pdot - 1] == '/' || s_path[pdot - 1] == '\\' || (pend == (pdot + 1) && s_path[pdot - 1] == '.' && (pdot == 1 || (s_path[pdot - 2] == '/' || s_path[pdot - 2] == '\\'))) ) {
				pdot = pend;
			}
			return path::CPathObject{.root = JC::array_cast<std::string>(s_path--->subarray(0, pend_root)), .dir = JC::array_cast<std::string>(s_path--->subarray(0, pend_dir)), .base = JC::array_cast<std::string>(s_path--->subarray(pbase, pend - pbase)), .ext = JC::array_cast<std::string>(s_path--->subarray(pdot, pend - pdot)), .name = JC::array_cast<std::string>(s_path--->subarray(pbase, pdot - pbase))};
		}
		std::string toAbsolute(std::span<char> s_path) {
			path::CPathObject parts = parse(s_path);
			std::string real_root = JC::array_cast<std::string>(parts.root);
			std::string real_dir = JC::array_cast<std::string>(parts.dir--->subarray(parts.root.size()));
			if ( real_root.size() == 2 ) {
				//drive-current directory
				JC::StringOrError real_root_env = ENV::get(JC::string_concat("=", parts.root));
				if ( !real_root_env || !real_root_env->size() ) {
					real_root = JC::string_concat(parts.root, '\\');
				} else {
					real_root = JC::string_concat(real_root_env, "");
				}
			}
			if ( !real_root.size() ) {
				real_root = JC::string_concat(fs::cwd(), '\\');
			} else if ( real_root == "\\" ) {
				real_root = parse(fs::cwd()).root;
			}
			return normalize(JC::string_concat(real_root, real_dir, '\\', parts.base));
		}
		std::string relative(std::span<char> from, std::span<char> _to) {
			std::string a = toAbsolute(from);
			std::vector<std::span<char>> parts_from{};
			{
				//coulddo: SSE / NEON vectorization
				size_t I0 = intptr_t(0L);
				for (size_t I = 0; I <= a.size(); ++I) {
					//char ch=(I<str.size()?str[I]:0)
					if ( I >= a.size() || a[I] == '/' || a[I] == '\\' ) {
						//this scope will be if-wrapped
						size_t I00 = I0;
						I0 = I + 1;
						//the return will be replaced
						std::span<char> value(a.data() + I00, I - I00);
						parts_from.push_back(value);
					}
				}
			}
			std::string a_0 = toAbsolute(_to);
			std::vector<std::span<char>> parts_to{};
			{
				//coulddo: SSE / NEON vectorization
				size_t I0 = intptr_t(0L);
				for (size_t I = 0; I <= a_0.size(); ++I) {
					//char ch=(I<str.size()?str[I]:0)
					if ( I >= a_0.size() || a_0[I] == '/' || a_0[I] == '\\' ) {
						//this scope will be if-wrapped
						size_t I00 = I0;
						I0 = I + 1;
						//the return will be replaced
						std::span<char> value(a_0.data() + I00, I - I00);
						parts_to.push_back(value);
					}
				}
			}
			size_t common_prefix = 0;
			for (; common_prefix < parts_from.size() && common_prefix < parts_to.size(); common_prefix++) {
				if ( unicode::toUpper(parts_from[common_prefix]) != unicode::toUpper(parts_to[common_prefix]) ) {
					break;
				}
			}
			std::string ret{};
			for (size_t i = common_prefix; i < parts_from.size(); i += 1) {
				ret--->push("..\\");
			}
			for (size_t i = common_prefix; i < parts_to.size(); i += 1) {
				ret--->push(parts_to[i], '\\');
			}
			if ( ret.size() ) {
				//remove trailing backslash
				ret--->pop();
			}
			return std::move(ret);
		}
	};
#endif
