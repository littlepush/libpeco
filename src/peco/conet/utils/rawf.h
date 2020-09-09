/*
    rawf.h
    PECoNet
    2019-05-12
    Push Chen

    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */

#pragma once

#ifndef PE_CO_NET_RAWF_H__
#define PE_CO_NET_RAWF_H__

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/obj.h>

namespace pe { namespace co { namespace net { namespace rawf {
    // Get localhost's computer name on LAN
    const string& hostname();

    // Get peer info from a socket
    peer_t socket_peerinfo(const SOCKET_T hSo);

    // Get local socket's port
    uint16_t localport(const SOCKET_T hSo);

    // Get the socket type
    uint32_t socktype(SOCKET_T hSo);

    // Set the linger time for a socket
    // I strong suggest not to change this value unless you 
    // know what you are doing
    bool lingertime(SOCKET_T hSo, bool onoff = true, unsigned timeout = 1);

    // Set Current socket reusable or not
    bool reusable(SOCKET_T hSo, bool reusable = true);

    // Make current socket keep alive
    bool keepalive(SOCKET_T hSo, bool keepalive = true);

    // Make a socket to be nonblocking
    bool nonblocking(SOCKET_T hSo, bool nonblocking = true);

    // Set the socket to be no-delay
    bool nodelay(SOCKET_T hSo, bool nodelay = true);

    // Set the socket's buffer size
    bool buffersize(SOCKET_T hSo, uint32_t rmem = 0, uint32_t wmem = 0);

    // Check if a socket has data to read
    bool has_data_pending(SOCKET_T hSo);

    // Check if a socket has buffer to write
    bool has_buffer_outgoing(SOCKET_T hSo);

    // Create Socket Handler
    SOCKET_T createSocket(int SOCK_TYPE);

    // Read Data From a socket
    size_t read(
        SOCKET_T hSo,
        char * buffer,
        size_t length,
        std::function<int (SOCKET_T, char *, size_t)> f);
    string read(
        SOCKET_T hSo, 
        std::function<int (SOCKET_T, char *, size_t)> f, 
        uint32_t max_buffer_size = 0);

    // Write Data to a socket
    int write(
        SOCKET_T hSo, 
        const char* data, 
        size_t data_lenth,
        std::function<int (SOCKET_T, const char*, size_t)> f);

}}}}

#endif

// Push Chen
//
