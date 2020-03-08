/*
    socks5.cpp
    PECoNet
    2019-05-17
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */

#include <peco/conet/protos/socks5.h>
#include <peco/conet/protos/dns.h>

namespace pe { namespace co { namespace net { namespace proto { namespace socks5 {
    const char *socks5msg(socks5_rep rep) {
        static const char * _gmsg[] = {
            "successed",
            "general SOCKS server failure",
            "connection not allowed by ruleset",
            "Network unreachable",
            "Host unreachable",
            "Connection refused",
            "TTL expired",
            "Command not supported",
            "Address type not supported",
            "Unknow Error Code"
        };
        if ( rep > socks5_rep_erroraddress ) return _gmsg[socks5_rep_erroraddress + 1];
        return _gmsg[rep];
    };

    static bool socks5_supported_method[3] = {false, false, false};

    // Setup the supported methods, can be invoke multiple times
    void socks5_set_supported_method(socks5_methods m) {
        if ( m > socks5_method_userpwd ) return;
        socks5_supported_method[m] = true;
    }

    // Get string inside the package
    string socks5_get_string(const char* buffer, uint32_t length) {
        string _result;
        if ( length <= sizeof(uint8_t) ) return _result;
        uint8_t _string_size = buffer[0];
        if ( length < (sizeof(uint8_t) + _string_size) ) return _result;
        _result.append(buffer + 1, _string_size);
        return _result;
    }

    // Append String to the end of the package
    void socks5_set_string(char* buffer, const string& value) {
        buffer[0] = (uint8_t)value.size();
        memcpy(buffer + 1, value.c_str(), value.size());
    }

    // Append IP Address and Port Info
    void socks5_set_peer(char* buffer, const peer_t& peer) {
        uint32_t *_ibuf = (uint32_t *)buffer;
        *_ibuf = (uint32_t)peer.ip;
        uint16_t *_pbuf = (uint16_t *)(buffer + sizeof(uint32_t));
        *_pbuf = htons(peer.port);
    }

    // Append Host and port
    void socks5_set_host(char* buffer, const string& host, uint16_t port) {
        socks5_set_string(buffer, host);
        uint16_t *_pbuf = (uint16_t *)(buffer + 1 + host.size());
        *_pbuf = htons(port);
    }

}}}}}

namespace pe { namespace co { namespace net {
    // Connect to the socks5 server
    socket_op_status connect( const peer_t& server, pe::co::duration_t timedout ) {
        auto _op = pe::co::net::tcp::connect( server, timedout );
        if ( _op != op_done ) return _op;

        proto::socks5::socks5_noauth_request _noauthReq;
        string _reqPkg((char *)&_noauthReq, sizeof(_noauthReq));
        _op = pe::co::net::tcp::write( std::move( _reqPkg ) );
        if ( _op != op_done ) return _op;
        string _incoming;
        _op = pe::co::net::tcp::read(_incoming, timedout);
        if ( _op != op_done ) return _op;
        const proto::socks5::socks5_handshake_response* _hResp = 
            (const proto::socks5::socks5_handshake_response*)_incoming.c_str();
        
        if ( _hResp->ver != 0x05 ) return op_failed;
        if ( _hResp->method != proto::socks5::socks5_method_noauth ) return op_failed;
        return op_done;
    }

    socket_op_status __connect( std::string&& pkg ) {
        pe::co::net::tcp::write( std::move(pkg) );
        /*
         * The maximum size of the protocol message we are waiting for is 10
         * bytes -- VER[1], REP[1], RSV[1], ATYP[1], BND.ADDR[4] and
         * BND.PORT[2]; see RFC 1928, section "6. Replies" for more details.
         * Everything else is already a part of the data we are supposed to
         * deliver to the requester. We know that BND.ADDR is exactly 4 bytes
         * since as you can see below, we accept only ATYP == 1 which specifies
         * that the IPv4 address is in a binary format.
         */
        std::string _incoming;
        auto _op = pe::co::net::tcp::read(_incoming);
        if ( _op != op_done ) return _op;
        const proto::socks5::socks5_ipv4_response* _resp = 
            (const proto::socks5::socks5_ipv4_response *)_incoming.c_str();
        if ( _resp->ver != 0x05 ) {
            return op_failed;
        }
        if ( _resp->rep != proto::socks5::socks5_rep_successed ) {
            return op_failed;
        }
        if ( _resp->atyp != proto::socks5::socks5_atyp_ipv4 ) {
            return op_failed;
        }
        return op_done;
    }


    socket_op_status socks5_connect(
        const peer_t& socks5,
        const std::string& host,
        uint16_t port, 
        pe::co::duration_t timedout
    ) {
        auto _op = connect( socks5, timedout );
        if ( _op != op_done ) return _op;

        proto::socks5::socks5_connect_request _connReq;
        _connReq.atyp = proto::socks5::socks5_atyp_dname;
        string _data(sizeof(_connReq) + sizeof(uint8_t) + host.size() + sizeof(uint16_t), '\0');
        char *_buf = &_data[0];
        memcpy(_buf, (char *)&_connReq, sizeof(_connReq));
        proto::socks5::socks5_set_host(_buf + sizeof(_connReq), host, port);

        // Send the packet
        return __connect(std::move(_data));
    }

    socket_op_status socks5_connect(
        const peer_t& socks5,
        const peer_t& target,
        pe::co::duration_t timedout
    ) {
        auto _op = connect( socks5, timedout );
        if ( _op != op_done ) return _op;

        proto::socks5::socks5_connect_request _connReq;
        _connReq.atyp = proto::socks5::socks5_atyp_ipv4;
        string _data(sizeof(_connReq) + sizeof(uint32_t) + sizeof(uint16_t), '\0');
        char *_buf = &_data[0];
        memcpy(_buf, (char *)&_connReq, sizeof(_connReq));
        proto::socks5::socks5_set_peer(_buf + sizeof(_connReq), target);

        // Send the packet
        return __connect(std::move(_data));
   }

    // Start a socks5 server
    namespace socks5_server {

        using namespace proto::socks5;

        // Simply create a socksr server
        void listen( handler_t handler ) {
            tcp::listen( [handler]() {
                std::string _buffer;
                auto _op = tcp::read(_buffer);
                if ( _op != op_done ) return;

                // Check hand shake version
                const socks5_handshake_request* _req = (
                    const socks5_handshake_request *
                )_buffer.c_str();
                if ( _req->ver != 5 ) return;

                // Tell the incoming request we only support noauth
                socks5_handshake_response _resp(socks5_method_noauth);
                std::string _sresp((char *)&_resp, sizeof(_resp));
                _op = tcp::write(std::move(_sresp));
                if ( _op != op_done ) return;

                // Get the peer info 
                _op = tcp::read(_buffer);
                if ( _op != op_done ) return;
                const socks5_connect_request* _connReq = 
                    (const socks5_connect_request *)_buffer.c_str();
                if ( _connReq->atyp == socks5_atyp_ipv4 ) {
                    const socks5_ipv4_request* _ipv4Req = 
                        (const socks5_ipv4_request *)_connReq;
                    peer_t _req_peer(ip_t(_ipv4Req->ip), ntohs(_ipv4Req->port));

                    socks5_ipv4_response _resp(_ipv4Req->ip, _ipv4Req->port);
                    _sresp.clear();
                    _sresp.append((char *)&_resp, sizeof(_resp));
                    _op = tcp::write(std::move(_sresp));
                    if ( _op != op_done ) return;
                    handler( _req_peer.ip.str(), _req_peer.port );
                } else if ( _connReq->atyp == socks5_atyp_dname ) {
                    const char *_b = (_buffer.c_str() + 
                        sizeof(socks5_connect_request));
                    string _host = socks5_get_string(_b, 
                        _buffer.size() - sizeof(socks5_connect_request));
                    _b += (1 + _host.size());
                    uint16_t _port = ntohs(*(uint16_t *)_b);

                    // The IP will be reversed after the handler processed it.
                    socks5_ipv4_response _resp(0u, _port);
                    _sresp.clear();
                    _sresp.append((char *)&_resp, sizeof(_resp));
                    _op = tcp::write(std::move(_sresp));
                    if ( _op != op_done ) return;
                    handler(_host, _port);
                } // No else, other type will be omitted.
            });
        }
    }

    struct __socks5_target_info {
        std::string         addr;
        uint16_t            port;
    };

    // Create a socket who will connect to peer socks5 proxy
    SOCKET_T socks5::create( peer_t bind_addr ) {
        return net::tcp::create( bind_addr );
    }

    // Get current incoming's target address
    std::string socks5::get_target_addr() {
        task * _ptask = this_task::get_task();
        if ( _ptask->arg == NULL ) return std::string("");
        return ((__socks5_target_info *)_ptask->arg)->addr;
    }

    // Get current incoming's target port
    uint16_t socks5::get_target_port() {
        task * _ptask = this_task::get_task();
        if ( _ptask->arg == NULL ) return 0;
        return ((__socks5_target_info *)_ptask->arg)->port;
    }

    // Listen
    void socks5::listen( std::function< void() > on_accept ) {
        net::socks5_server::listen([on_accept](
            const std::string& addr, uint16_t port
        ) {
            // Save the target info in the arg
            __socks5_target_info _ti{addr, port};
            this_task::get_task()->arg = (void *)&_ti;
            if ( on_accept ) on_accept();
        });
    }

    // Connect to peer, default 3 seconds for timedout
    socket_op_status socks5::connect(
        connect_address cinfo,
        pe::co::duration_t timedout
    ) {
        net::peer_t _socks5(
            net::get_hostname(cinfo.proxy.addr), 
            cinfo.proxy.port);
        net::ip_t _taddr(cinfo.target.addr);
        if ( _taddr ) {
            return net::socks5_connect(
                _socks5, net::peer_t(_taddr, cinfo.target.port), 
                timedout);
        } else {
            return net::socks5_connect(
                _socks5, cinfo.target.addr, cinfo.target.port, 
                timedout);
        }
    }

    // Read data from the socket
    socket_op_status socks5::read_from(
        task* ptask,
        string& buffer,
        pe::co::duration_t timedout
    ) {
        return net::tcp::read_from(ptask, buffer, timedout);
    }
    socket_op_status socks5::read(
        string& buffer,
        pe::co::duration_t timedout
    ) {
        return net::tcp::read(buffer, timedout);
    }
    socket_op_status socks5::read (
        char* buffer,
        uint32_t& buflen,
        pe::co::duration_t timedout
    ) {
        return net::tcp::read(buffer, buflen, timedout);
    }

    // Write the given data to peer
    socket_op_status socks5::write (
        string&& data, 
        pe::co::duration_t timedout
    ) {
        return net::tcp::write( std::move(data), timedout );
    }
    socket_op_status socks5::write (
        const char* data, 
        uint32_t length,
        pe::co::duration_t timedout
    ) {
        return net::tcp::write(data, length, timedout);
    }
    socket_op_status socks5::write_to(
        task* ptask, 
        const char* data, 
        uint32_t length, 
        pe::co::duration_t timedout
    ) {
        return net::tcp::write_to(ptask, data, length, timedout);
    }

    bool socks5::redirect_data( task * ptask, write_to_t hwt ) {
        return net::tcp::redirect_data( ptask, hwt );
    }
}}}

// Push Chen
//

