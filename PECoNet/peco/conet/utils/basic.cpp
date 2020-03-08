/*
    basic.cpp
    PECoNet
    2019-05-12
    Push Chen

    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */

#include <peco/conet/utils/basic.h>

extern "C" {
    int PECoNet_Autoconf() { return 0; }
}

namespace pe { namespace co { namespace net {
    ON_DEBUG(
        bool g_conet_trace_flag = false;
        void enable_conet_trace() { g_conet_trace_flag = true; }
        bool is_conet_trace_enabled() { return g_conet_trace_flag; }
    )

    // Get the string of a socket type.
    const string so_socket_type_name(unsigned int st) {
        static string _stypes[] = { "TCP Socket", "UDP Socket", "UnixDomain Socket"};
        return (_stypes[(st - 10000) % 3]);
    }

    uint16_t h2n( uint16_t v ) { return htons(v); }
    uint32_t h2n( uint32_t v ) { return htonl(v); }
    uint64_t h2n( uint64_t v ) {
#if PZC_TARGET_LINUX
        return htobe64(v);
#else
        return htonll(v);
#endif
    }
    float h2n( float v ) {
        uint32_t *_iv = (uint32_t *)(&v);
        uint32_t _nv = h2n(*_iv);
        uint32_t *_pnv = &_nv;
        return *(float *)_pnv;
    }
    double h2n( double v ) {
        uint64_t *_iv = (uint64_t *)(&v);
        uint64_t _nv = h2n(*_iv);
        uint64_t *_pnv = &_nv;
        return *(double *)_pnv;
    }

    uint16_t n2h( uint16_t v ) { return ntohs(v); }
    uint32_t n2h( uint32_t v ) { return ntohl(v); }
    uint64_t n2h( uint64_t v ) { 
#if PZC_TARGET_LINUX
        return be64toh(v);
#else
        return ntohll(v); 
#endif
    }
    float n2h( float v ) { 
        uint32_t* _iv = (uint32_t *)(&v);
        uint32_t _nv = n2h(*_iv);
        uint32_t *_pnv = &_nv;
        return *(float *)_pnv;
    }
    double n2h( double v ) {
        uint64_t* _iv = (uint64_t *)(&v);
        uint64_t _nv = n2h(*_iv);
        uint64_t *_pnv = &_nv;
        return *(double *)_pnv;
    }

    // Convert host endian to little endian
    #if PZC_TARGET_LINUX
    uint16_t h2le(uint16_t v) { return htole16(v); }
    uint32_t h2le(uint32_t v) { return htole32(v); }
    uint64_t h2le(uint64_t v) { return htole64(v); }
    #else
    uint16_t h2le(uint16_t v) { return OSSwapHostToLittleInt16(v); }
    uint32_t h2le(uint32_t v) { return OSSwapHostToLittleInt32(v); }
    uint64_t h2le(uint64_t v) { return OSSwapHostToLittleInt64(v); }
    #endif
    float h2le(float v) {
        uint32_t *_iv = (uint32_t *)(&v);
        uint32_t _lev = h2le(*_iv);
        uint32_t *_plev = &_lev;
        return *(float *)_plev;
    }
    double h2le(double v) {
        uint64_t *_iv = (uint64_t *)(&v);
        uint64_t _lev = h2le(*_iv);
        uint64_t *_plev = &_lev;
        return *(double *)_plev;
    }

    // Little Endian to Host
    #if PZC_TARGET_LINUX
    uint16_t le2h(uint16_t v) { return le16toh(v); }
    uint32_t le2h(uint32_t v) { return le32toh(v); }
    uint64_t le2h(uint64_t v) { return le64toh(v); }
    #else
    uint16_t le2h(uint16_t v) { return OSSwapLittleToHostInt16(v); }
    uint32_t le2h(uint32_t v) { return OSSwapLittleToHostInt32(v); }
    uint64_t le2h(uint64_t v) { return OSSwapLittleToHostInt64(v); }
    #endif
    float le2h(float v) {
        uint32_t *_iv = (uint32_t *)(&v);
        uint32_t _lev = le2h(*_iv);
        uint32_t *_plev = &_lev;
        return *(float *)_plev;
    }
    double le2h(double v) {
        uint64_t *_iv = (uint64_t *)(&v);
        uint64_t _lev = le2h(*_iv);
        uint64_t *_plev = &_lev;
        return *(double *)_plev;
    }

    // Convert host endian to big endian
    #if PZC_TARGET_LINUX
    uint16_t h2be(uint16_t v) { return htobe16(v); }
    uint32_t h2be(uint32_t v) { return htobe32(v); }
    uint64_t h2be(uint64_t v) { return htobe64(v); }
    #else
    uint16_t h2be(uint16_t v) { return OSSwapHostToBigInt16(v); }
    uint32_t h2be(uint32_t v) { return OSSwapHostToBigInt32(v); }
    uint64_t h2be(uint64_t v) { return OSSwapHostToBigInt64(v); }
    #endif
    float h2be(float v) {
        uint32_t *_iv = (uint32_t *)(&v);
        uint32_t _lev = h2be(*_iv);
        uint32_t *_plev = &_lev;
        return *(float *)_plev;
    }
    double h2be(double v) {
        uint64_t *_iv = (uint64_t *)(&v);
        uint64_t _lev = h2be(*_iv);
        uint64_t *_plev = &_lev;
        return *(double *)_plev;
    }

    // Big Endian to Host
    #if PZC_TARGET_LINUX
    uint16_t be2h(uint16_t v) { return be16toh(v); }
    uint32_t be2h(uint32_t v) { return be32toh(v); }
    uint64_t be2h(uint64_t v) { return be64toh(v); }
    #else
    uint16_t be2h(uint16_t v) { return OSSwapBigToHostInt16(v); }
    uint32_t be2h(uint32_t v) { return OSSwapBigToHostInt32(v); }
    uint64_t be2h(uint64_t v) { return OSSwapBigToHostInt64(v); }
    #endif
    float be2h(float v) {
        uint32_t *_iv = (uint32_t *)(&v);
        uint32_t _lev = be2h(*_iv);
        uint32_t *_plev = &_lev;
        return *(float *)_plev;
    }
    double be2h(double v) {
        uint64_t *_iv = (uint64_t *)(&v);
        uint64_t _lev = be2h(*_iv);
        uint64_t *_plev = &_lev;
        return *(double *)_plev;
    }

}}}

// Push Chen
//
