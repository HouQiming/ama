#if defined(_WIN32)
	#include <windows.h>
	#include "unicode.hpp"
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <dirent.h>
	#include <utime.h>
	#include <sys/types.h>
	#include <sys/stat.h>
#endif
#include <stdio.h>
#include <string>
#include <vector>
#include <array>
#include "jc_array.h"
#include "fs.hpp"
typedef struct stat stat_t;
#if defined(_WIN32)
	std::vector<uint16_t> fs::PathToWindows(std::span<char> s) {
		std::vector<uint16_t> ret{};
		unicode::TWTF8Filter filter{.ch0 = 0, .nnxt = 0, .II = intptr_t(0L), .surrogate_1st = -1, .surrogate_II = intptr_t(0L), .ch_min = 0};
		for (intptr_t I = intptr_t(0L); I < intptr_t(s.size()); I++) {
			int32_t ch = filter.NextByte(I, int32_t(uint32_t(uint8_t(s[I]))));
			if ( ch < 0 ) {
				continue;
			}
			if ( ch >= 0x10000 ) {
				//surrogate pair
				ch -= 0x10000;
				ret--->push(uint16_t(uint32_t(0xd800 + (ch >> 10 & 0x3ff))));
				ch &= 0x3ff;
				ch += 0xdc00;
			} else if ( ch == '/' ) {
				ch = '\\';
			}
			ret--->push(uint16_t(uint32_t(ch)));
		}
		ret--->push(uint16_t(0));
		return std::move(ret);
	}
#endif

static const intptr_t MAX_READ_BATCH = intptr_t(8388608L);
JC::StringOrError fs::readFileSync(std::span<char> fn) {
	size_t sz = intptr_t(0L);
	intptr_t n_read = intptr_t(0L);
	JC::StringOrError ret_read = JC::StringOrError(nullptr);
	intptr_t p{};
	#if defined(_WIN32)
		{
			std::vector<uint16_t> fnu = fs::PathToWindows(fn);
			HANDLE hf = CreateFileW(
				LPCWSTR(fnu.data()),
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr
			);
			if ( !hf || hf == INVALID_HANDLE_VALUE ) {
				return nullptr;
			}
			std::array<int64_t, intptr_t(1L)> psz{int64_t(0LL)};
			GetFileSizeEx(hf, PLARGE_INTEGER(psz.data()));
			sz = size_t(psz[0]);
			ret_read = std::string(sz, char(0));
			p = intptr_t(0L);
			for (; ;) {
				intptr_t sz_read = intptr_t(sz - p);
				if ( sz_read > MAX_READ_BATCH ) { sz_read = MAX_READ_BATCH; }
				n_read = DWORD(0);
				ReadFile(hf, ret_read->data() + p, int(sz_read), LPDWORD(&n_read), LPOVERLAPPED(nullptr));
				if ( n_read <= intptr_t(0L) ) {
					break;
				}
				p += n_read;
			}
			CloseHandle(hf);
			if ( p != intptr_t(sz) ) {
				return nullptr;
			} else {
				return std::move(ret_read);
			}
		}
	#else
		{
			std::string fnz = JC::array_cast<std::string>(fn);
			fnz--->push(char(0));
			int fd = open(fnz.data(), O_RDONLY, 0777);
			if ( fd == -1 ) {
				return nullptr;
			}
			stat_t sb = stat_t();
			sb.st_size = 0;
			fstat(fd, &sb);
			sz = size_t(sb.st_size);
			ret_read = std::string(sz, char(0));
			p = intptr_t(0L);
			for (; ;) {
				intptr_t sz_read = intptr_t(sz - p);
				if ( sz_read > MAX_READ_BATCH ) { sz_read = MAX_READ_BATCH; }
				n_read = read(fd, (char*)(ret_read->data()) + p, sz_read);
				if ( n_read <= intptr_t(0L) ) {
					break;
				}
				p += n_read;
			}
			close(fd);
			if ( p != intptr_t(sz) ) {
				return nullptr;
			} else {
				return std::move(ret_read);
			}
		}
	#endif
}
intptr_t fs::writeFileSync(std::span<char> fn, std::span<char> content) {
	FILE* f{};
	#if defined(_WIN32)
		std::array<int16_t, intptr_t(3L)> s_wb{int16_t('w'), int16_t('b'), int16_t(0)};
		std::vector<uint16_t> fnu = fs::PathToWindows(fn);
		_wfopen_s(&f, LPCWSTR(fnu.data()), LPCWSTR(s_wb.data()));
	#else
		std::string fnz = JC::array_cast<std::string>(fn);
		//fnz.push('\0');
		f = fopen(fnz.c_str(), "wb");
	#endif
	if ( !f ) {
		return intptr_t(0L);
	}
	size_t n_written = fwrite(content.data(), 1, content.size(), f);
	fclose(f);
	return n_written;
}
int fs::existsSync(std::span<char> fn) {
	#if defined(_WIN32)
		std::vector<uint16_t> fnu = fs::PathToWindows(fn);
		return GetFileAttributesW(LPCWSTR(fnu.data())) != -1;
	#else
		std::string fnz = JC::array_cast<std::string>(fn);
		fnz--->push(char(0));
		stat_t sb = stat_t();
		return stat(fnz.data(), &sb) == 0;
	#endif
}
int fs::DirExists(std::span<char> dir) {
	#if defined(_WIN32)
		std::vector<uint16_t> fnu = fs::PathToWindows(dir);
		DWORD attr = GetFileAttributesW(LPCWSTR(fnu.data()));
		return attr != -1 && (attr & FILE_ATTRIBUTE_DIRECTORY);
	#else
		std::string fnz = JC::array_cast<std::string>(dir);
		stat_t sb = stat_t();
		return stat(fnz.data(), &sb) == 0 && S_ISDIR(sb.st_mode);
	#endif
}
std::string fs::cwd() {
	#if defined(_WIN32)
		{
			std::vector<uint16_t> buf((264));
			if ( !GetCurrentDirectoryW(260, LPWSTR(buf.data())) ) {
				return std::move(nullptr);
			}
			for (int32_t i = 0; i < buf.size(); i += 1) {
				if ( !buf[i] ) {
					buf.resize(i);
					break;
				}
			}
			return unicode::UTF16ToUTF8(buf);
		}
	#else
		{
			std::string buf(1024, char(0));
			while ( !getcwd((char*)(buf.data()), buf.size() - 1) && buf.size() < (1 << 24) ) {
				buf.resize(buf.size() * 2);
			}
			for (int32_t i = 0; i < buf.size(); i += 1) {
				if ( !buf[i] ) {
					buf.resize(i);
					break;
				}
			}
			return std::move(buf);
		}
	#endif
}
int fs::chdir(std::span<char> dir) {
	#if defined(_WIN32)
		std::vector<uint16_t> su = fs::PathToWindows(dir);
		return SetCurrentDirectoryW(LPCWSTR(su.data()));
	#else
		std::string sdir = JC::array_cast<std::string>(dir);
		return ::chdir(sdir.c_str()) == 0;
	#endif
}
int fs::mkdirSync(std::span<char> dir) {
	#if defined(_WIN32)
		std::vector<uint16_t> su = fs::PathToWindows(dir);
		return CreateDirectoryW(LPCWSTR(su.data()), nullptr);
	#else
		std::string sdir = JC::array_cast<std::string>(dir);
		return mkdir(sdir.c_str(), 509) == 0;
	#endif
}
intptr_t fs::appendFileSync(std::span<char> fn, std::span<char> content) {
	FILE* f{};
	#if defined(_WIN32)
		std::array<int16_t, intptr_t(3L)> s_wb{int16_t('a'), int16_t('b'), int16_t(0)};
		std::vector<uint16_t> fnu = fs::PathToWindows(fn);
		_wfopen_s(&f, LPCWSTR(fnu.data()), LPCWSTR(s_wb.data()));
	#else
		std::string fnz = JC::array_cast<std::string>(fn);
		//fnz.push('\0');
		f = fopen(fnz.c_str(), "ab");
	#endif
	if ( !f ) {
		return intptr_t(0L);
	}
	size_t n_written = fwrite(content.data(), 1, content.size(), f);
	fclose(f);
	return n_written;
}
std::vector<fs::Dirent> fs::readdirSync(std::span<char> dir) {
	std::vector<fs::Dirent> ret{};
	#if defined(_WIN32)
		std::vector<uint16_t> su = fs::PathToWindows(dir);
		su.pop_back();
		su.push_back(uint16_t('\\'));
		su.push_back(uint16_t('*'));
		su.push_back(uint16_t(0));
		WIN32_FIND_DATAW fdata{};
		HANDLE hdir = FindFirstFileW(LPCWSTR(su.data()), &fdata);
		if (hdir && hdir != INVALID_HANDLE_VALUE) {
			for (; ;) {
				intptr_t wslen = 0;
				while (wslen < MAX_PATH) {
					if (!fdata.cFileName[wslen]) {break;}
					wslen++;
				}
				ret.push_back(fs::Dirent{
					.name = unicode::UTF16ToUTF8(std::span<uint16_t>((uint16_t*)fdata.cFileName, wslen)),
					.is_file = uint8_t(!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)),
					.is_dir = uint8_t(!!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				});
				if (!FindNextFileW(hdir, &fdata)) {
					break;
				}
			}
			FindClose(hdir);
		}
	#else
		std::string sdir = JC::array_cast<std::string>(dir);
		DIR* hdir = opendir(sdir.c_str());
		if (hdir) {
			for (; ;) {
				struct dirent *entry = readdir(hdir);
				if (!entry) {break;}
				ret.push_back(fs::Dirent{
					.name = entry->d_name,
					.is_file = uint8_t(entry->d_type == DT_REG),
					.is_dir = uint8_t(entry->d_type == DT_DIR)
				});
			}
			closedir(hdir);
		}
	#endif
	return std::move(ret);
}

int fs::SyncTimestamp(std::span<char> fn_src, std::span<char> fn_tar) {
	#if defined(_WIN32)
		std::vector<uint16_t> fn_srcu = fs::PathToWindows(fn_src);
		std::vector<uint16_t> fn_taru = fs::PathToWindows(fn_tar);
		HANDLE hsrc = CreateFileW(LPCWSTR(fn_srcu.data()), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (!hsrc || hsrc == INVALID_HANDLE_VALUE) {return 0;}
		HANDLE htar = CreateFileW(LPCWSTR(fn_taru.data()), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (!htar || htar == INVALID_HANDLE_VALUE) {
			CloseHandle(hsrc);
			return 0;
		}
		int ret = 0;
		FILETIME ctime{};
		FILETIME atime{};
		FILETIME mtime{};
		if (GetFileTime(hsrc, &ctime, &atime, &mtime)) {
			SetFileTime(htar, &ctime, &atime, &mtime);
			ret = 1;
		}
		CloseHandle(hsrc);
		CloseHandle(htar);
		return ret;
	#else
		std::string fn_srcz = JC::array_cast<std::string>(fn_src);
		std::string fn_tarz = JC::array_cast<std::string>(fn_tar);
		struct stat sb{};
		if ( stat(fn_srcz.c_str(), &sb) != 0 ) {
			return 0;
		}
		struct timespec times[2];
		times[0] = sb.st_atim;
		times[1] = sb.st_mtim;
		return utimensat(AT_FDCWD, fn_tarz.c_str(), times, 0) == 0;
	#endif
}
