#pragma once
#ifndef __JC_PLATFORM_H
#define __JC_PLATFORM_H

#include <stdint.h>
#include <stddef.h>

/////////////////////////////////////////////////
//CPU detection macros

//pointer size 
#if INTPTR_MAX == INT32_MAX
	#define JC_POINTER_SIZE 4
#elif INTPTR_MAX == INT64_MAX
	#define JC_POINTER_SIZE 8
#else
	#error failed to detect pointer size
#endif

//CPU architecture, from duktape, re-namespaced
#define JC_CPU_UNKNOWN 0
#define JC_CPU_ARM 1
#define JC_CPU_X86X64 2

#if defined(__arm__) || defined(__thumb__) || defined(_ARM) || \
defined(_M_ARM) || defined(__aarch64__) || defined(__arm64) || defined(__arm64__)
	#define JC_CPU JC_CPU_ARM
#elif defined(__amd64__) || defined(__amd64) || \
defined(__x86_64__) || defined(__x86_64) || \
defined(_M_X64) || defined(_M_AMD64) || \
defined(i386) || defined(__i386) || defined(__i386__) || \
defined(__i486__) || defined(__i586__) || defined(__i686__) || \
defined(__IA32__) || defined(_M_IX86) || defined(__X86__) || \
defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__)
	#define JC_CPU JC_CPU_X86X64
#else
	#define JC_CPU JC_CPU_UNKNOWN
#endif

//SIMD support
#define JC_SIMD_NONE 0
#define JC_SIMD_NEON 1
#define JC_SIMD_SSE2 2

#if defined(__ARM_NEON__) || (defined(__ARM_ARCH_7A__) && (defined(__ANDROID__) || defined(ANDROID))) || defined(__aarch64__)
	#define JC_SIMD JC_SIMD_NEON
#elif JC_CPU == JC_CPU_X86X64
	#define JC_SIMD JC_SIMD_SSE2
#else
	#define JC_SIMD JC_SIMD_NONE
#endif

/////////////////////////////////////////////////
//OS detection macros
#define JC_OS_UNKNOWN 0
#define JC_OS_ANDROID 1
#define JC_OS_MAC 2
#define JC_OS_IOS 3
#define JC_OS_WINDOWS 4
#define JC_OS_WEB 5
#define JC_OS_LINUX 6

#if defined(_WIN32)
	#define JC_OS JC_OS_WINDOWS
	#if JC_POINTER_SIZE >= 8
		#define JC_JC1_ARCH_STRING "win64"
	#else
		#define JC_JC1_ARCH_STRING "win32"
	#endif
#elif defined(__APPLE__)
	#include <TargetConditionals.h>
	#if TARGET_IPHONE_SIMULATOR
		#define JC_OS JC_OS_IOS
		#define JC_JC1_ARCH_STRING "ios"
	#elif TARGET_OS_IPHONE
		#define JC_OS JC_OS_IOS
		#define JC_JC1_ARCH_STRING "ios"
	#elif TARGET_OS_MAC
		#define JC_OS JC_OS_MAC
		#define JC_JC1_ARCH_STRING "mac"
	#else
		#error unsupported apple platform
	#endif
#elif defined(__ANDROID__) || defined(ANDROID)
	#define JC_OS JC_OS_ANDROID
	#define JC_JC1_ARCH_STRING "android"
#elif defined(__EMSCRIPTEN__)
	#define JC_OS JC_OS_WEB
	#define JC_JC1_ARCH_STRING "web"
#elif defined(__linux) || defined(__linux__) || defined(linux)
	//Linux has to be detected last to differentiate it from other Linux-based OS
	#define JC_OS JC_OS_LINUX
	#if JC_POINTER_SIZE >= 8
		#define JC_JC1_ARCH_STRING "linux64"
	#else
		#define JC_JC1_ARCH_STRING "linux32"
	#endif
#else
	#define JC_OS JC_OS_UNKNOWN
	#define JC_JC1_ARCH_STRING "unknown"
#endif

/////////////////////////////////////////////////
//Android ABI
//NONE means "not Android"
#define JC_ANDROID_ABI_NONE 0
#define JC_ANDROID_ABI_ARMEABI_V7A 1
#define JC_ANDROID_ABI_ARM64_V8A 2
#define JC_ANDROID_ABI_X86 3
#define JC_ANDROID_ABI_X86_64 4

#ifndef JC_ANDROID_ABI
	#define JC_ANDROID_ABI JC_ANDROID_ABI_NONE
#endif

/////////////////////////////////////////////////
//debug / release macros
#define JC_BUILD_DEBUG 0
#define JC_BUILD_RELEASE 1

#ifdef NDEBUG
	#define JC_BUILD JC_BUILD_RELEASE
	#define JC_JC1_BUILD_STRING "release"
#else
	#define JC_BUILD JC_BUILD_DEBUG
	#define JC_JC1_BUILD_STRING "debug"
#endif

/////////////////////////////////////////////////
//output macros, expected from makefile
#define JC_OUTPUT_EXECUTABLE 0
#define JC_OUTPUT_DYNAMIC_LIBRARY 1
#define JC_OUTPUT_STATIC_LIBRARY 2

#ifndef JC_OUTPUT
	#define JC_OUTPUT JC_OUTPUT_EXECUTABLE
#endif

/////////////////////////////////////////////////
//IS_FOO macros
#if JC_OS != JC_OS_WINDOWS && JC_OS != JC_OS_UNKNOWN
	#define JC_TARGET_IS_UNIX 1
#else
	#define JC_TARGET_IS_UNIX 0
#endif

#define JC_TARGET_IS_WINDOWS (JC_OS == JC_OS_WINDOWS)
#define JC_TARGET_IS_MOBILE (JC_OS == JC_OS_IOS || JC_OS == JC_OS_ANDROID)
#if defined(__APPLE__)
	#define JC_TARGET_IS_APPLE 1
#else
	#define JC_TARGET_IS_APPLE 0
#endif
//we assumed Android and web are not Linux since IS_LINUX was mainly used to detect servers (i.e. no guaranteed camera / GPU)
#define JC_TARGET_IS_LINUX (JC_OS == JC_OS_LINUX)
//when Linux API is needed, use JC_TARGET_IS_LINUX_COMPATIBLE
#if defined(__linux) || defined(__linux__) || defined(linux)
	#define JC_TARGET_IS_LINUX_COMPATIBLE 1
#else
	#define JC_TARGET_IS_LINUX_COMPATIBLE 0
#endif

/////////////////////////////////////////////////
//JC configuration macros
#define JC_VERSION 0x300

//Windows could need dllexport
#if JC_OS == JC_OS_WINDOWS && JC_OUTPUT == JC_OUTPUT_DYNAMIC_LIBRARY
	#define JC_EXPORT __declspec(dllexport)
#else
	#define JC_EXPORT
#endif

//compiler workaround macros
#if __cplusplus >= 201103L
	#define JC_ALIGNOF alignof
	#define JC_ALIGNED_STRUCT(alignment) struct alignas(alignment)
	#define JC_ALIGNED_CONST(alignment) alignas(alignment)
#else
	#if defined(_MSC_VER)
		#define JC_ALIGNOF __alignof
		#define JC_ALIGNED_STRUCT(alignment) __declspec(align(alignment)) struct
		#define JC_ALIGNED_CONST(alignment) __declspec(align(alignment))
	#else
		#define JC_ALIGNOF __alignof__
		#define JC_ALIGNED_STRUCT(alignment) struct __attribute__((aligned(alignment)))
		#define JC_ALIGNED_CONST(alignment) __attribute__((aligned(alignment)))
	#endif
#endif

#ifdef offsetof
//the performance advantage of offsetof is too big to ignore over compliance
#if __cplusplus
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
#define JC_OFFSETOF(type, field) offsetof(type, field)
#else
//some C++ compilers could actually break this
#define JC_OFFSETOF(type, field) ((intptr_t) & ((type*)0)->field)
#endif
#define JC_GET_OBJECT_PTR(type, field, ptr) ((type*)((intptr_t)(ptr) - JC_OFFSETOF(type, field)))

#endif
