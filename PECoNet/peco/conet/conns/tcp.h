/*
    tcp.h
    PECoNet
    2019-05-13
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */
 
#pragma once

#ifndef PE_CO_NET_TCP_H__
#define PE_CO_NET_TCP_H__

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/rawf.h>
#include <peco/conet/utils/obj.h>

#include <peco/conet/conns/conns.hpp>

namespace pe { namespace co { namespace net { 
    struct tcp {

        typedef peer_t          address_bind;
        typedef peer_t          connect_address;

        // Set the flag to reuse listening port
        static void reuse_port( bool on = true );

        // Create a socket, and bind to the given peer info
        // All following functions must be invoked in a task
        // Usage:
        //  SOCKET_T _so = create();
        //  pe::co::loop::main.do_job(_so, [](){
        //      listen(...);
        //  });
        static SOCKET_T create( peer_t bind_addr = peer_t::nan );

        // Listen on the socket, the socket must has been bound
        // to the listen port, which means the parameter of the
        // create function cannot be nan
        static void listen(
            std::function< void( ) > on_accept
        );

        // Connect to peer, default 3 seconds for timedout
        static socket_op_status connect( 
            peer_t remote,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );
        // Connect to a host and port
        static socket_op_status connect(
            const string& hostname, 
            uint16_t port,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );

        // Read data from the socket
        static socket_op_status read_from(
            task_t ptask,
            string& buffer,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT 
        );
        static socket_op_status read( 
            string& buffer,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT 
        );
        static socket_op_status read(
            char* buffer,
            uint32_t& buflen,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );

        // Write the given data to peer
        static socket_op_status write(
            string&& data, 
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );
        static socket_op_status write(
            const char* data, 
            uint32_t length,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );
        static socket_op_status write_to( 
            task_t ptask, 
            const char* data, 
            uint32_t length, 
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );

        // Redirect data between two socket use the
        // established tunnel
        // If there is no tunnel, return false, otherwise
        // after the tunnel has broken, return true
        static bool redirect_data( task_t ptask, write_to_t hwt = &write_to );

        // Simple Redirect to Another TCP
        static void redirect_to_tcp( peer_t remote );
        static void redirect_to_tcp( const string& hostname, uint16_t port );
        static void redirect_to_tcp( peer_t remote, peer_t socks5 );
        static void redirect_to_tcp( const string& hostname, uint16_t port, peer_t socks5 );

    };

    // The tcp factory
    typedef conn_factory< tcp >  tcp_factory;
    typedef typename tcp_factory::item  tcp_item;
}}}

#endif

// Push Chen
//

