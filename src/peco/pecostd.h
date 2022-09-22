/*
    pecostd.h
    libpeco
    2016-01-11
    Push Chen
 */

/*
MIT License

Copyright (c) 2019 Push Chen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#ifndef PECO_STD_H__
#define PECO_STD_H__

#define LIBPECO_STANDARD_VERSION 0x00030000

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

/*
    Check the target platform
*/
#if (defined WIN32 | defined _WIN32 | defined WIN64 | defined _WIN64)
#define _PECO_PLATFORM_WIN 1
#elif TARGET_OS_WIN32
#define _PECO_PLATFORM_WIN 1
#elif defined __CYGWIN__
#define _PECO_PLATFORM_WIN 1
#else
#define _PECO_PLATFORM_WIN 0
#endif
#ifdef __APPLE__
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#define _PECO_PLATFORM_MAC 0
#define _PECO_PLATFORM_IOS 1
#else
#define _PECO_PLATFORM_MAC 1
#define _PECO_PLATFORM_IOS 0
#endif
#else
#define _PECO_PLATFORM_MAC 0
#endif
#ifdef __ANDROID__
#define _PECO_PLATFORM_ANDROID 1
#else
#define _PECO_PLATFORM_ANDROID 0
#endif
#if _PECO_PLATFORM_WIN == 0 && _PECO_PLATFORM_MAC == 0 &&                      \
  _PECO_PLATFORM_ANDROID == 0
#define _PECO_PLATFORM_LINUX 1
#else
#define _PECO_PLATFORM_LINUX 0
#endif

#define PECO_TARGET_WIN (_PECO_PLATFORM_WIN == 1)
#define PECO_TARGET_LINUX (_PECO_PLATFORM_LINUX == 1)
#define PECO_TARGET_MAC (_PECO_PLATFORM_MAC == 1)
#define PECO_TARGET_IOS (_PECO_PLATFORM_IOS == 1)
#define PECO_TARGET_ANDROID (_PECO_PLATFORM_ANDROID == 1)
#define PECO_TARGET_POSIX (!PECO_TARGET_WIN)
#define PECO_TARGET_MOBILE (PECO_TARGET_IOS || PECO_TARGET_ANDROID)
#define PECO_TARGET_APPLE (PECO_TARGET_MAC || PECO_TARGET_IOS)
#define PECO_TARGET_PC (PECO_TARGET_WIN || PECO_TARGET_LINUX || PECO_TARGET_MAC)
#define PECO_TARGET_OPENOS (!PECO_TARGET_WIN && !PECO_TARGET_APPLE)

#ifdef __cplusplus
extern "C" {
#endif

/*
 Standard C Header Files
*/
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#if !PECO_TARGET_WIN
#include <sys/time.h>
#include <unistd.h>
#endif

#if (PECO_TARGET_WIN32)
#include <Windows.h>
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/*
    C++ Standard Files
 */
#include <iostream>
#include <algorithm>
#include <chrono>
#include <memory>
#include <functional>
#include <string>

#if defined(__GLIBC__) || defined(__GLIBCPP__) || defined(__clang__)
#define PECO_USE_GNU 1
#else
#define PECO_USE_GNU 0
#endif

#define PECO_IS_SHARED  (PECO_LIB_STATIC == 0 && PECO_LIB_SHARED == 1)
#define PECO_IS_STATIC  (PECO_LIB_STATIC == 1 && PECO_LIB_SHARED == 0)

#if PECO_TARGET_WIN && PECO_IS_SHARED
#if defined(PECO_IS_BUILDING) && (PECO_IS_BUILDING == 1)
#define PECO_VISIABLE   __declspec(dllexport)
#else
#define PECO_VISIABLE   __declspec(dllimport)
#endif
#else
#define PECO_VISIABLE
#endif

namespace peco {

template <typename T> inline void ignore_result(T unused_result) {
  (void)unused_result;
}

#endif

} // namespace peco

#endif

// Push Chen
