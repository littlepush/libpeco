/*
    socks5.h
    PECoNet
    2019-05-17
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */
 
#pragma once

#ifndef PE_CO_NET_SOCSK5_H__
#define PE_CO_NET_SOCKS5_H__

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/obj.h>
#include <peco/conet/conns/tcp.h>

namespace pe { namespace co { namespace net { namespace proto { namespace socks5 {
    enum socks5_methods {
        socks5_method_noauth        = 0,
        socks5_method_gssapi        = 1,
        socks5_method_userpwd       = 2,
        socks5_method_nomethod      = 0xff
    };

    enum socks5_cmd {
        socks5_cmd_connect          = 1,
        socks5_cmd_bind             = 2,
        socks5_cmd_udp              = 3
    };

    enum socks5_atyp {
        socks5_atyp_ipv4            = 1,
        socks5_atyp_dname           = 3,
        socks5_atyp_ipv6            = 4,
    };

    enum socks5_rep {
        socks5_rep_successed        = 0,    // successed
        socks5_rep_failed           = 1,    // general SOCKS server failure
        socks5_rep_connectnotallow  = 2,    // connection not allowed by ruleset
        socks5_rep_unreachable      = 3,    // Network unreachable
        socks5_rep_hostunreachable  = 4,    // Host unreachable
        socks5_rep_refused          = 5,    // Connection refused
        socks5_rep_expired          = 6,    // TTL expired
        socks5_rep_notsupport       = 7,    // Command not supported
        socks5_rep_erroraddress     = 8,    // Address type not supported
    };

    // Convert the rep code to string
    const char *socks5msg(socks5_rep rep);

    #pragma pack(push, 1)
    struct socks5_packet {
        uint8_t     ver;

        // Default we only support version 5
        socks5_packet() : ver(5) {}
    };

    struct socks5_handshake_request : public socks5_packet {
        uint8_t     nmethods;
    };

    struct socks5_noauth_request : public socks5_handshake_request {
        uint8_t     method;

        socks5_noauth_request(): 
            socks5_handshake_request(), method(socks5_method_noauth) {
            nmethods = 1;
            }
    };

    struct socks5_gssapi_request : public socks5_handshake_request {
        uint8_t     method;

        socks5_gssapi_request():
            socks5_handshake_request(), method(socks5_method_gssapi) {
            nmethods = 1;
            }
    };

    struct socks5_userpwd_request : public socks5_handshake_request {
        uint8_t     method;

        socks5_userpwd_request():
            socks5_handshake_request(), method(socks5_method_userpwd) {
            nmethods = 1;
            }
    };

    struct socks5_handshake_response : public socks5_packet {
        uint8_t     method;

        socks5_handshake_response() : socks5_packet() {}
        socks5_handshake_response(socks5_methods m) : socks5_packet(), method(m) { }
    };

    struct socks5_connect_request : public socks5_packet {
        uint8_t     cmd;
        uint8_t     rsv;    // reserved
        uint8_t     atyp;   // address type

        socks5_connect_request():
            socks5_packet(), cmd(socks5_cmd_connect), rsv(0) {}
    };

    struct socks5_ipv4_request : public socks5_connect_request {
        uint32_t    ip;
        uint16_t    port;

        socks5_ipv4_request(uint32_t ipv4, uint16_t p):
            socks5_connect_request(), ip(ipv4), port(p) {
            atyp = socks5_atyp_ipv4;
            }
    };

    struct socks5_connect_response : public socks5_packet {
        uint8_t     rep;
        uint8_t     rsv;
        uint8_t     atyp;

        socks5_connect_response() : socks5_packet() {}
    };

    struct socks5_ipv4_response : public socks5_connect_response {
        uint32_t    ip;
        uint16_t    port;

        socks5_ipv4_response(): socks5_connect_response() {}
        socks5_ipv4_response(uint32_t addr, uint16_t p):
            socks5_connect_response(), 
            ip(addr),
            port(p)
        {
            rep = socks5_rep_successed;
            atyp = socks5_atyp_ipv4;
        }
    };
    #pragma pack(pop)

    // Setup the supported methods, can be invoke multiple times
    void socks5_set_supported_method(socks5_methods m);

    // Get string inside the package
    string socks5_get_string(const char* buffer, uint32_t length);

    // Append String to the end of the package
    void socks5_set_string(char* buffer, const string& value);

    // Append IP Address and Port Info
    void socks5_set_peer(char* buffer, const peer_t& peer);

    // Append Host and port
    void socks5_set_host(char* buffer, const string& host, uint16_t port);

}}}}}

namespace pe { namespace co { namespace net {
    // Connect to the socks5 server and ask to connect to host
    socket_op_status socks5_connect(
        const peer_t& socks5,
        const std::string& host,
        uint16_t port, 
        pe::co::duration_t timedout = std::chrono::seconds(3)
    );
    // Connect to the socks5 server and ask to connect to peer
    socket_op_status socks5_connect(
        const peer_t& socks5,
        const peer_t& target,
        pe::co::duration_t timedout = std::chrono::seconds(3)
    );

    // Start a socks5 server
    namespace socks5_server {
        typedef std::function< void( const std::string&, uint16_t ) > handler_t;

        // Simply create a socksr server
        void listen( handler_t handler );
    }

    // Conn API
    struct socks5 {
        typedef net::peer_t                         address_bind;

        typedef struct {
            std::string     addr;
            uint16_t        port;
        } peer_info;
        
        typedef struct {
            peer_info       proxy;
            peer_info       target;
        } connect_address;

        // Create a socket who will connect to peer socks5 proxy
        static SOCKET_T create( peer_t bind_addr = peer_t::nan );

        // Get current incoming's target address
        static std::string get_target_addr();

        // Get current incoming's target port
        static uint16_t get_target_port();

        // Listen
        static void listen( std::function< void() > on_accept );

        // Connect to peer, default 3 seconds for timedout
        static socket_op_status connect( 
            connect_address cinfo,
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

        static bool redirect_data( task * ptask, write_to_t hwt = &write_to );
    };

    // Socks5 Connection Factory
    typedef conn_factory< socks5 >          socks5_factory;
    typedef typename socks5_factory::item   socks5_item;

}}}

#endif

// Push Chen
//

