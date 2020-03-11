/*
    mustd.h
    mumake
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

#ifndef PZC_LIB_C_PZCSTD_H__
#define PZC_LIB_C_PZCSTD_H__

#define LIBPECO_STANDARD_VERSION 0x00020002

/*
    Check the target platform
*/
#if ( defined WIN32 | defined _WIN32 | defined WIN64 | defined _WIN64 )
    #define _PZC_PLATFORM_WIN      1
#elif TARGET_OS_WIN32
    #define _PZC_PLATFORM_WIN      1
#elif defined __CYGWIN__
    #define _PZC_PLATFORM_WIN      1
#else
    #define _PZC_PLATFORM_WIN      0
#endif
#ifdef __APPLE__
    #define _PZC_PLATFORM_MAC      1
#else
    #define _PZC_PLATFORM_MAC      0
#endif
#if _PZC_PLATFORM_WIN == 0 && _PZC_PLATFORM_MAC == 0
    #define _PZC_PLATFORM_LINUX    1
#else
    #define _PZC_PLATFORM_LINUX    0
#endif

#ifdef __APPLE__
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define _PZC_PLATFORM_IOS      1
    #else
        #define _PZC_PLATFORM_IOS      0
    #endif
#endif

#define PZC_TARGET_WIN32 (_PZC_PLATFORM_WIN == 1)
#define PZC_TARGET_LINUX (_PZC_PLATFORM_LINUX == 1)
#define PZC_TARGET_MAC   (_PZC_PLATFORM_MAC == 1)
#define PZC_TARGET_IOS   (_PZC_PLATFORM_IOS == 1)

#if PZC_TARGET_WIN32
#error "PZC Error: Target platform is not supported, we now not support Windows. Please use Linux, Mac OS, or any BSD system."
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 Standard C Header Files
*/
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <stddef.h>
#include <math.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <stddef.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>

#if (PZC_TARGET_LINUX)
#include <linux/netfilter_ipv4.h>
#endif
 
#ifdef __cplusplus
}
#endif

#endif

// MeetU Information and Technology Inc.
// Push Chen
// @littlepush
