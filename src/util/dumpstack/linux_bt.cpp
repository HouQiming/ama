#if (defined(__linux)||defined(__linux__)||defined(linux))&&!(defined(__ANDROID__)||defined(ANDROID)||defined(__EMSCRIPTEN__))
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/wait.h>
extern "C" char __executable_start;
extern "C" char __etext;
namespace DumpStack {
	volatile int g_dump_all_threads = 0;
	volatile int g_is_our_abort = 0;
	void PrintCallStack() {
		pid_t pid = getpid();
		void* bt_addresses[64] = {};
		int n_levels = (int)backtrace(bt_addresses, 64);
		char** messages = backtrace_symbols(bt_addresses, n_levels);
		//level 2 is the offending instruction, anything above that is a call
		for (int i = 0; i < n_levels; i++) {
			char proc_self[64] = {};
			char cmd[1024] = {};
			char* addr = (char*)bt_addresses[i] - (i > 2 ? 1 : 0);
			char* s_message_i = messages[i];
			//the self refers to addr2line, we just gamble it's loading the same .so-s
			char* s_begin_of_exe = (char*)"/proc/self/exe";
			char* s_end_of_exe = s_begin_of_exe + 14;
			char* module_base = (char*) & __executable_start;
			int is_self = (size_t)(addr - &__executable_start) < (size_t)(&__etext - &__executable_start);
			if (is_self) {
				s_begin_of_exe = proc_self;
				sprintf(proc_self, "/proc/%d/exe", (int)pid);
				s_end_of_exe = proc_self + strlen(proc_self);
			} else if (s_message_i) {
				char* end_of_exe = strchr(s_message_i, '(');
				if (end_of_exe) {
					s_begin_of_exe = s_message_i;
					s_end_of_exe = end_of_exe;
				}
				//base address - dladdr
				Dl_info info{};
				dladdr(addr, &info);
				module_base = (char*)info.dli_fbase;
			}
			//if(!i){
			//	sprintf(cmd,"cat /proc/%d/maps",pid);
			//	system(cmd);
			//	fprintf(stderr,"\033[0m%p %p\n",&__executable_start,&__etext);
			//}
			if (isatty(2)) {
				fprintf(stderr, "\033[1m");
			}
			sprintf(cmd,
				"ADDR=`addr2line -e %.*s -fC %p`;"
				"if echo \"${ADDR}\" | grep '??""' > /dev/null ; then "
				"ADDR=`addr2line -e %.*s -fC 0x%zx`;"
				"fi;"
				"echo \"${ADDR}\" | sed -n 'h;n;x;H;x;s/\\n/: in function /g;p' | tr -d '\\n'",
				(int)(s_end_of_exe - s_begin_of_exe), s_begin_of_exe, addr,
				(int)(s_end_of_exe - s_begin_of_exe), s_begin_of_exe, (size_t)(addr - module_base));
			//int code=system(cmd);
			//(void)code;
			pid_t pid = vfork();
			int code = 0;
			if (pid == 0) {
				//shell won't work if stdin / stdout are broken, so we put in substitutes
				//if stderr were broken, we won't get to print anything anyway
				dup2(open("/dev/null", O_RDONLY), STDIN_FILENO);
				dup2(STDERR_FILENO, STDOUT_FILENO);
				exit(execl("/bin/sh", "sh", "-c", cmd, NULL));
			}
			if (pid != -1) {
				waitpid(pid, &code, 0);
			}
			//printf("??:0: in function ??");
			if (isatty(2)) {
				fprintf(stderr, "\033[0;90m");
			}
			fprintf(stderr, " (%s)\r\n", messages && messages[i] ? messages[i] : "backtrace_symbols failed");
			if (isatty(2)) {
				fprintf(stderr, "\033[0m");
			}
			fflush(stderr);
		}
		if (messages) {
			free(messages);
		}
	}
	
	static void StackDumper(int sig, siginfo_t *si, void *unused) {
		if (!g_is_our_abort) {
			const char* name = "SIGNAL";
			switch (sig) {
			case SIGSEGV: name = "SIGSEGV";break;
			case SIGABRT: name = "SIGABRT";break;
			case SIGBUS: name = "SIGBUS";break;
			case SIGPIPE: name = "SIGPIPE";break;
			case SIGTRAP: name = "SIGTRAP";break;
			}
			pid_t pid = getpid();
			if (isatty(2)) {
				fprintf(stderr, "\033[?7h\033[?25h\033[0m");
			}
			if (sig == SIGTRAP) {
				fprintf(stderr, "PID %d has triggered a breakpoint\r\n", pid);
			} else if (sig == SIGSEGV && (uintptr_t)(intptr_t)si->si_addr < (uintptr_t)0x1000) {
				fprintf(stderr, "PID %d accessed a NULL pointer at offset %d\r\n", pid, (int)(intptr_t)si->si_addr);
			} else {
				fprintf(stderr,
					"PID %d received %s at address: %p\r\n",
					pid, name, si->si_addr
				);
			}
			fflush(stderr);
			//////////
			PrintCallStack();
		}
		g_is_our_abort = 1;
		abort();
	}
	
	void EnableDump() {
		struct sigaction sa= {};
		//////////
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = StackDumper;
		sa.sa_flags   = SA_SIGINFO | SA_RESETHAND;
		sigaction(SIGSEGV, &sa, NULL);
		//////////
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = StackDumper;
		sa.sa_flags   = SA_SIGINFO | SA_RESETHAND;
		sigaction(SIGTRAP, &sa, NULL);
		//////////
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = StackDumper;
		sa.sa_flags   = SA_SIGINFO | SA_RESETHAND;
		sigaction(SIGABRT, &sa, NULL);
		//////////
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = StackDumper;
		sa.sa_flags   = SA_SIGINFO | SA_RESETHAND;
		sigaction(SIGBUS, &sa, NULL);
		//////////
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = StackDumper;
		sa.sa_flags   = SA_SIGINFO | SA_RESETHAND;
		sigaction(SIGPIPE, &sa, NULL);
		//////////
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = StackDumper;
		sa.sa_flags   = SA_SIGINFO | SA_RESETHAND;
		sigaction(SIGINT, &sa, NULL);
		//////////
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = StackDumper;
		sa.sa_flags   = SA_SIGINFO | SA_RESETHAND;
		sigaction(SIGFPE, &sa, NULL);
	}
}
#endif
