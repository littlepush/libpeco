/*
    basic.h 
    PECoNet
    2019-05-12
    Push Chen

    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_CO_NET_BASIC_H__
#define PE_CO_NET_BASIC_H__

// Include the pecotask header file
#include <peco/peutils.h>
#include <peco/cotask.h>

#include <sys/un.h>
#include <fcntl.h>

#if PZC_TARGET_LINUX
#include <endian.h>
#elif PZC_TARGET_MAC
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#endif

namespace pe { namespace co { namespace net {
    ON_DEBUG(
        void enable_conet_trace();
        bool is_conet_trace_enabled();
    )

    #ifdef DEBUG
    #define ON_DEBUG_CONET(...)         if ( is_conet_trace_enabled() ) { __VA_ARGS__ }
    #else
    #define ON_DEBUG_CONET(...)
    #endif

    typedef long SOCKET_T;

    #ifndef INVALIDATE_SOCKET
    #define INVALIDATE_SOCKET           ((long)((long)0 - (long)1))
    #endif

    #define SOCKET_NOT_VALIDATE( so )   ((so) == INVALIDATE_SOCKET)
    #define SOCKET_VALIDATE( so )       ((so) != INVALIDATE_SOCKET)

    // Status of a socket's operation
    typedef enum {
        op_done     = 0,
        op_timedout = 1,
        op_failed   = 2
    } socket_op_status;

    // Socket Type
    #ifndef ST_TCP_TYPE
    #define ST_TCP_TYPE     10000
    #endif

    #ifndef ST_UDP_TYPE
    #define ST_UDP_TYPE     10001
    #endif

    // Unix Domain Socket Type
    #ifndef ST_UNIX_DOMAIN
    #define ST_UNIX_DOMAIN  10002
    #endif

    #define NET_DEFAULT_TIMEOUT     std::chrono::seconds(10)

    // Get the string of a socket type.
    const string so_socket_type_name(unsigned int st);

    // Connect callback
    typedef std::function<void()>           connect_t;
    // When all data has been sent, invoke the write callback
    typedef std::function<void()>           write_t;
    // When there is a package incoming, invoke this callback
    typedef std::function<void(string&&)>   read_t;

    // Write To Protocol
    typedef std::function< 
        socket_op_status( task *, const char*, uint32_t, duration_t ) 
        >                                   write_to_t;

    // Buffer Guard
    template< size_t _I_Buffer_Length = 4096 >
    struct buffer_guard {
        enum { blen = _I_Buffer_Length };
        char *          buf;
        buffer_guard() {
            buf = (char *)malloc(sizeof(char) * _I_Buffer_Length);
        }
        ~buffer_guard() {
            free(buf);
        }
    };
    typedef buffer_guard< 1024 >        buffer_guard_1k;
    typedef buffer_guard< 4 * 1024 >    buffer_guard_4k;
    typedef buffer_guard< 8 * 1024 >    buffer_guard_8k;
    typedef buffer_guard< 16 * 1024 >   buffer_guard_16k;
    typedef buffer_guard< 32 * 1024 >   buffer_guard_32k;
    typedef buffer_guard< 64 * 1024 >   buffer_guard_64k;
    typedef buffer_guard< 128 * 1024 >  buffer_guard_128k;
    typedef buffer_guard< 256 * 1024 >  buffer_guard_256k;
    typedef buffer_guard< 512 * 1024 >  buffer_guard_512k;
    typedef buffer_guard< 1024 * 1024 > buffer_guard_1m;


    // Net and host order conversition
    uint16_t h2n(uint16_t v);
    uint32_t h2n(uint32_t v);
    uint64_t h2n(uint64_t v);
    float h2n(float v);
    double h2n(double v);

    // N2H is the same logic of h2n
    uint16_t n2h(uint16_t v);
    uint32_t n2h(uint32_t v);
    uint64_t n2h(uint64_t v);
    float n2h(float v);
    double n2h(double v);

    // Convert host endian to little endian
    uint16_t h2le(uint16_t v);
    uint32_t h2le(uint32_t v);
    uint64_t h2le(uint64_t v);
    float h2le(float v);
    double h2le(double v);

    // Little Endian to Host
    uint16_t le2h(uint16_t v);
    uint32_t le2h(uint32_t v);
    uint64_t le2h(uint64_t v);
    float le2h(float v);
    double le2h(double v);

    // Convert host endian to big endian
    uint16_t h2be(uint16_t v);
    uint32_t h2be(uint32_t v);
    uint64_t h2be(uint64_t v);
    float h2be(float v);
    double h2be(double v);

    // Big Endian to Host
    uint16_t be2h(uint16_t v);
    uint32_t be2h(uint32_t v);
    uint64_t be2h(uint64_t v);
    float be2h(float v);
    double be2h(double v);
}}}

#endif 

// Push Chen
// 
