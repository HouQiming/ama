#if defined(_WIN32)
#include <windows.h>
#include <stdio.h>
struct TConsoleFixer {
	TConsoleFixer() {
		HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD mode = 0;
		GetConsoleMode(hstdout, &mode);
		//4 is ENABLE_VIRTUAL_TERMINAL_PROCESSING
		SetConsoleMode(hstdout, mode | 4);
		//65001 is CP_UTF8
		SetConsoleCP(65001);
		SetConsoleOutputCP(65001);
	}
};
static TConsoleFixer a = TConsoleFixer();
#endif
