/*
    uds.h
    PECoNet
    2019-05-17
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */
 
#pragma once

#ifndef PE_CO_NET_UDS_H__
#define PE_CO_NET_UDS_H__

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/rawf.h>
#include <peco/conet/utils/obj.h>
#include <peco/conet/conns/conns.hpp>

namespace pe { namespace co { namespace net { 
    struct uds {

        typedef std::string          address_bind;
        typedef std::string          connect_address;

        // Create a socket
        // All following functions must be invoked in a task
        // Usage:
        //  SOCKET_T _so = create();
        //  pe::co::loop::main.do_job(_so, [](){
        //      listen(...);
        //  });
        static SOCKET_T create( const std::string& filepath = "" );

        // Listen on the socket, the socket must has been bound
        // to the listen port, which means the parameter of the
        // create function cannot be nan
        static void listen(
            std::function< void( ) > on_accept
        );

        // Connect to a host and port
        static socket_op_status connect(
            const string& filepath, 
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );

        // Read data from the socket
        static socket_op_status read_from(
            task* ptask,
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
            task* ptask, 
            const char* data, 
            uint32_t length, 
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );

        // Redirect data between two socket use the
        // established tunnel
        // If there is no tunnel, return false, otherwise
        // after the tunnel has broken, return true
        static bool redirect_data( task * ptask, write_to_t hwt = &write_to );
    };

    // The tcp factory
    typedef conn_factory< uds >  uds_factory;
    typedef typename uds_factory::item  uds_item;

}}}

#endif

// Push Chen
// 

