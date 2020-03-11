/*
    udp.h
    PECoNet
    2019-05-14
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */
 
#pragma once

#ifndef PE_CO_NET_UDP_H__
#define PE_CO_NET_UDP_H__

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/rawf.h>
#include <peco/conet/utils/obj.h>

namespace pe { namespace co { namespace net { namespace udp {
    // Create a socket, and bind to the given peer info
    // All following functions must be invoked in a task
    // Usage:
    //  SOCKET_T _so = create();
    //  pe::co::loop::main.do_job(_so, [](){
    //      listen(...);
    //  });
    SOCKET_T create( peer_t bind_addr = peer_t::nan );

    // Listen on the socket, the socket must has been bound
    // to the listen port, which means the parameter of the
    // create function cannot be nan
    void listen(
        std::function< void( const peer_t&, std::string&& ) > on_accept
    );

    // Read data from the socket
    socket_op_status read_from(
        task_t ptask,
        string& buffer,
        struct sockaddr_in* addr,
        pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT 
    );
    socket_op_status read( 
        string& buffer,
        struct sockaddr_in* addr,
        pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT 
    );
    socket_op_status read(
        char* buffer,
        uint32_t& buflen,
        struct sockaddr_in* addr,
        pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
    );

    // Write the given data to peer
    socket_op_status write(
        string&& data, 
        struct sockaddr_in addr,
        pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
    );
    socket_op_status write(
        const char* data, 
        uint32_t length,
        struct sockaddr_in addr,
        pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
    );
    socket_op_status write_to( 
        task_t ptask, 
        const char* data, 
        uint32_t length, 
        struct sockaddr_in addr,
        pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
    );

    // Redirect data between two socket use the
    // established tunnel
    // If there is no tunnel, return false, otherwise
    // after the tunnel has broken, return true
    bool redirect_data( struct sockaddr_in addr, task_t ptask, write_to_t hwt );


}}}}

#endif

// Push Chen
//
