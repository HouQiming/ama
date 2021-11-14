#if defined(_WIN32)
#include <windows.h>
#pragma warning(disable: 4091)
#include <dbghelp.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <tlhelp32.h>
#pragma no_auto_header()

namespace DumpStack {
	//NOT DEAD
	static const int DUMP_STACK_EXCEPTION = 0x0507DEAD;
	struct IMAGEHLP_SYMBOL64_with_buffer {
		IMAGEHLP_SYMBOL64 main{};
		char buffer[1024];
	};
	static IMAGEHLP_SYMBOL64_with_buffer symbol = {};
	static IMAGEHLP_MODULEW64 module = {};
	void DumpCallStackFromContext(PCONTEXT ctx, HANDLE hthread) {
		STACKFRAME frame = {};
		#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
			frame.AddrPC.Offset = ctx->Rip; frame.AddrPC.Mode = AddrModeFlat;
			frame.AddrFrame.Offset = ctx->Rbp; frame.AddrFrame.Mode = AddrModeFlat;
			frame.AddrStack.Offset = ctx->Rsp; frame.AddrStack.Mode = AddrModeFlat;
			#define MY_MACHINE_TYPE IMAGE_FILE_MACHINE_AMD64
			#define PTR_FORMAT "%016llx"
			fprintf(stderr,
				"rax=" PTR_FORMAT" rcx=" PTR_FORMAT" rdx=" PTR_FORMAT" rbx=" PTR_FORMAT"\n"
				"rsp=" PTR_FORMAT" rbp=" PTR_FORMAT" rsi=" PTR_FORMAT" rdi=" PTR_FORMAT"\n"
				"r8 =" PTR_FORMAT" r9 =" PTR_FORMAT" r10=" PTR_FORMAT" r11=" PTR_FORMAT"\n"
				"r12=" PTR_FORMAT" r13=" PTR_FORMAT" r14=" PTR_FORMAT" r15=" PTR_FORMAT"\n"
				"rip=" PTR_FORMAT" eflags=%08x\n",
				ctx->Rax, ctx->Rcx, ctx->Rdx, ctx->Rbx,
				ctx->Rsp, ctx->Rbp, ctx->Rsi, ctx->Rdi,
				ctx->R8, ctx->R9, ctx->R10, ctx->R11,
				ctx->R12, ctx->R13, ctx->R14, ctx->R15,
				ctx->Rip, ctx->EFlags
			);
			fflush(stderr);
		#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__IA32__) || defined(_M_IX86) || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__)
			frame.AddrPC.Offset = ctx->Eip; frame.AddrPC.Mode = AddrModeFlat;
			frame.AddrFrame.Offset = ctx->Ebp; frame.AddrFrame.Mode = AddrModeFlat;
			frame.AddrStack.Offset = ctx->Esp; frame.AddrStack.Mode = AddrModeFlat;
			#define MY_MACHINE_TYPE IMAGE_FILE_MACHINE_I386
			#define PTR_FORMAT "%08x"
		#endif
		HANDLE hprocess = GetCurrentProcess();
		int is_call = 0;
		for (; ;) {
			if (!StackWalk64(MY_MACHINE_TYPE, hprocess, hthread, &frame, ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
				break;
			}
			//////////
			DWORD displacement = 0;
			IMAGEHLP_LINEW64 line = {};
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			if (SymGetLineFromAddrW64(hprocess, frame.AddrPC.Offset - is_call, &displacement, &line)) {
				fprintf(stderr, "%S:%d: " PTR_FORMAT, line.FileName, line.LineNumber, frame.AddrPC.Offset);
			} else {
				DWORD64 hmodule = SymGetModuleBase64(hprocess, frame.AddrPC.Offset);
				if (hmodule) {
					memset(&module, 0, sizeof(module));
					module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
					if (SymGetModuleInfoW64(hprocess, hmodule, &module)) {
						fprintf(stderr, "module %S: " PTR_FORMAT, module.LoadedImageName, frame.AddrPC.Offset);
					} else {
						fprintf(stderr, " in module " PTR_FORMAT": " PTR_FORMAT, hmodule, frame.AddrPC.Offset);
					}
				} else {
					fprintf(stderr, "(unknown file): " PTR_FORMAT, frame.AddrPC.Offset);
				}
			}
			/////////
			DWORD64 displacement64 = 0;
			memset(&symbol, 0, sizeof(symbol));
			symbol.main.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
			symbol.main.MaxNameLength = 1024;
			if (SymGetSymFromAddr64(hprocess, frame.AddrPC.Offset - is_call, &displacement64, &symbol.main)) {
				fprintf(stderr, " in function %s() + %lld\n", symbol.main.Name, displacement64 + (DWORD64)is_call);
			} else {
				DWORD64 hmodule = SymGetModuleBase64(hprocess, frame.AddrPC.Offset);
				if (hmodule) {
					memset(&module, 0, sizeof(module));
					module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
					if (SymGetModuleInfoW64(hprocess, hmodule, &module)) {
						fwprintf(stderr, L" in module %s (" PTR_FORMAT")\n", module.LoadedImageName, hmodule);
					} else {
						fprintf(stderr, " in module " PTR_FORMAT"\n", hmodule);
					}
				} else {
					fprintf(stderr, " in unknown module\n");
				}
			}
			fflush(stderr);
			is_call = 1;
		}
	}
	
	static CONTEXT g_backup_ctx = {};
	static CONTEXT g_thread_ctx = {};
	static CRITICAL_SECTION g_lock = {};
	volatile int g_dump_all_threads = 0;
	static LONG WINAPI StackDumper(_In_ struct _EXCEPTION_POINTERS * ExceptionInfo) {
		///////////////////
		if (!TryEnterCriticalSection(&g_lock)) {
			//if we crash again during the dump, give up
			return EXCEPTION_CONTINUE_SEARCH;
		}
		PEXCEPTION_RECORD err = ExceptionInfo->ExceptionRecord;
		switch (err->ExceptionCode) {
		default: {
			fprintf(stderr, "Exception code %x at address %p\n", err->ExceptionCode, err->ExceptionAddress);
			break;
		}
		case DUMP_STACK_EXCEPTION: {
			fprintf(stderr, "Stack dump\n");
			break;
		}
		case EXCEPTION_ACCESS_VIOLATION: {
			const char* how = "access";
			if (err->ExceptionInformation[0] == 0) {
				how = "read";
			} else if (err->ExceptionInformation[0] == 1) {
				how = "write";
			} else if (err->ExceptionInformation[0] == 8) {
				how = "execution";
			}
			if ((uintptr_t)(intptr_t)err->ExceptionInformation[1] < (uintptr_t)0x1000) {
				fprintf(stderr, "NULL pointer %s at address %p\n", how, (void*)err->ExceptionInformation[1]);
			} else {
				fprintf(stderr, "%s access violation at address %p\n", how, (void*)err->ExceptionInformation[1]);
			}
			break;
		}
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: {
			fprintf(stderr, "EXCEPTION_ARRAY_BOUNDS_EXCEEDED at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_BREAKPOINT: {
			fprintf(stderr, "EXCEPTION_BREAKPOINT at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_FLT_DENORMAL_OPERAND: {
			fprintf(stderr, "EXCEPTION_FLT_DENORMAL_OPERAND at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: {
			fprintf(stderr, "EXCEPTION_FLT_DIVIDE_BY_ZERO at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_FLT_INEXACT_RESULT: {
			fprintf(stderr, "EXCEPTION_FLT_INEXACT_RESULT at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_FLT_INVALID_OPERATION: {
			fprintf(stderr, "EXCEPTION_FLT_INVALID_OPERATION at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_FLT_OVERFLOW: {
			fprintf(stderr, "EXCEPTION_FLT_OVERFLOW at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_FLT_STACK_CHECK: {
			fprintf(stderr, "EXCEPTION_FLT_STACK_CHECK at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_FLT_UNDERFLOW: {
			fprintf(stderr, "EXCEPTION_FLT_UNDERFLOW at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_ILLEGAL_INSTRUCTION: {
			fprintf(stderr, "EXCEPTION_ILLEGAL_INSTRUCTION at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_IN_PAGE_ERROR: {
			fprintf(stderr, "EXCEPTION_IN_PAGE_ERROR at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_INT_DIVIDE_BY_ZERO: {
			fprintf(stderr, "EXCEPTION_INT_DIVIDE_BY_ZERO at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_INT_OVERFLOW: {
			fprintf(stderr, "EXCEPTION_INT_OVERFLOW at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_INVALID_DISPOSITION: {
			fprintf(stderr, "EXCEPTION_INVALID_DISPOSITION at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: {
			fprintf(stderr, "EXCEPTION_NONCONTINUABLE_EXCEPTION at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_PRIV_INSTRUCTION: {
			fprintf(stderr, "EXCEPTION_PRIV_INSTRUCTION at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_SINGLE_STEP: {
			fprintf(stderr, "EXCEPTION_SINGLE_STEP at address %p\n", err->ExceptionAddress);
			break;
		}
		case EXCEPTION_STACK_OVERFLOW: {
			fprintf(stderr, "EXCEPTION_STACK_OVERFLOW at address %p\n", err->ExceptionAddress);
			break;
		}}
		fflush(stderr);
		///////////////////
		//the calling thread
		//we don't want to destroy ExceptionInfo->ContextRecord
		g_backup_ctx = *ExceptionInfo->ContextRecord;
		DumpCallStackFromContext(&g_backup_ctx, GetCurrentThread());
		// Take a snapshot of all running threads
		if (g_dump_all_threads) {
			THREADENTRY32 te32{};
			HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
			int pid = GetProcessId(GetCurrentProcess());
			int tid = GetThreadId(GetCurrentThread());
			if ( hThreadSnap && hThreadSnap != INVALID_HANDLE_VALUE ) {
				// Fill in the size of the structure before using it. 
				te32.dwSize = sizeof(THREADENTRY32); 
				// Retrieve information about the first thread,
				// and exit if unsuccessful
				if ( Thread32First(hThreadSnap, &te32) ) {
					do { 
						if ( te32.th32OwnerProcessID == pid ) {
							fprintf(stderr, "\nThread #%d\n", te32.th32ThreadID);
							fflush(stderr);
							if (tid == te32.th32ThreadID) {
								fprintf(stderr, "  is the current thread\n");
								fflush(stderr);
								continue;
							}
							HANDLE hthread = OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, 0, te32.th32ThreadID);
							if (hthread && hthread != INVALID_HANDLE_VALUE) {
								SuspendThread(hthread);
								memset(&g_thread_ctx, 0, sizeof(g_thread_ctx));
								g_thread_ctx.ContextFlags = CONTEXT_ALL;
								if (GetThreadContext(hthread, &g_thread_ctx)) {
									DumpCallStackFromContext(&g_thread_ctx, hthread);
								}
								ResumeThread(hthread);
								CloseHandle(hthread);
							}
						}
					} while ( Thread32Next(hThreadSnap, &te32) );}
				CloseHandle(hThreadSnap);
			}
		}
		LeaveCriticalSection(&g_lock);
		if (err->ExceptionCode == DUMP_STACK_EXCEPTION) {
			return EXCEPTION_CONTINUE_EXECUTION;
		} else {
			return EXCEPTION_CONTINUE_SEARCH;
		}
	}
	
	static void CrashOnAbort(int signal) {
		if (signal == SIGINT) {g_dump_all_threads = 1;}
		AddVectoredExceptionHandler(1, StackDumper);
		SetUnhandledExceptionFilter(nullptr);
		RaiseException(EXCEPTION_BREAKPOINT, 0, 0, nullptr);
		RemoveVectoredExceptionHandler(StackDumper);
	}
	
	//BOOL WINAPI ConsoleHandler(DWORD dwCtrlType){
	//	fprintf(stderr,"dwCtrlType=%d\n",dwCtrlType);fflush(stderr);
	//	if(dwCtrlType==CTRL_C_EVENT){
	//		AddVectoredExceptionHandler(1,StackDumper);
	//		RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
	//		RemoveVectoredExceptionHandler(StackDumper);
	//	}
	//	return 0;
	//}
	void PrintCallStack() {
		AddVectoredExceptionHandler(1, StackDumper);
		RaiseException(DUMP_STACK_EXCEPTION, 0, 0, nullptr);
		RemoveVectoredExceptionHandler(StackDumper);
	}
	
	void EnableDump() {
		InitializeCriticalSection(&g_lock);
		SymInitializeW(GetCurrentProcess(), nullptr, true);
		SymSetOptions(SYMOPT_UNDNAME);
		SetUnhandledExceptionFilter(StackDumper);
		signal(SIGABRT, CrashOnAbort);
		signal(SIGINT, CrashOnAbort);
		//SetConsoleCtrlHandler(ConsoleHandler,TRUE);
	}
}
#endif
