#pragma once

#ifdef _WIN64
#define OS_WIN64 1
#define OS_WINDOWS 1
#elif _WIN32
#define OS_WIN32 1
#define OS_WINDOWS 1
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
#define OS_APPLE 1
#define OS_IPHONE 1
#define OS_IPHONE_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define OS_APPLE 1
#define OS_IPHONE 1
#else
#define TARGET_OS_OSX 1
#define OS_APPLE 1
#define OS_OSX 1
#endif
#elif __ANDROID__
#define OS_ANDROID 1
#elif __linux
#define OS_LINUX 1
#define OS_POSIX 1
#elif __unix
#define OS_LINUX 1
#define OS_POSIX 1
#elif __posix
#define OS_POSIX 1
#endif

#if defined(_MSC_VER)
#define CXX_MSVC 1
#elif defined(__clang__)
#define CXX_CLANG 1
#elif (defined(__GNUC__) || defined(__GNUG__)) && !defined(__INTEL_COMPILER)
#define CXX_GCC 1
#endif

#if !NDEBUG || _DEBUG
#define CXX_LEVEL_DEBUG 1
#endif
