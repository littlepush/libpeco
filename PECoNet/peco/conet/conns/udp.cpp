/*
    udp.cpp
    PECoNet
    2019-05-14
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */

#include <peco/conet/conns/udp.h>

namespace pe { namespace co { namespace net { namespace udp {
    // Create a socket, and bind to the given peer info
    SOCKET_T create( peer_t bind_addr ) {
        SOCKET_T _so = INVALIDATE_SOCKET;
        do {
            _so = rawf::createSocket(ST_UDP_TYPE);
        } while ( pe::co::this_task::yield_if(SOCKET_NOT_VALIDATE(_so)) );

        if ( (bool)bind_addr ) {
            // Bind to the address
            struct sockaddr_in _sock_addr;
            memset((char *)&_sock_addr, 0, sizeof(_sock_addr));
            _sock_addr.sin_family = AF_INET;
            _sock_addr.sin_port = htons(bind_addr.port);
            _sock_addr.sin_addr.s_addr = (uint32_t)bind_addr.ip;

            if ( ::bind(_so, (struct sockaddr *)&_sock_addr, sizeof(_sock_addr)) == -1 ) {
                ON_DEBUG_CONET(
                std::cerr << "Failed to bind udp socket on: " << bind_addr.str()
                    << ", " << ::strerror( errno ) << std::endl;
                )
                ::close(_so);
                _so = INVALIDATE_SOCKET;
            }
        }
        return _so;
    }

    // Listen on the socket, the socket must has been bound
    // to the listen port, which means the parameter of the
    // create function cannot be nan
    void listen(
        std::function< void( const peer_t&, std::string&& ) > on_accept
    ) {
        while ( true ) {
            std::string _buffer;
            struct sockaddr_in _addr;
            socket_op_status _os = read(_buffer, &_addr);
            if ( _os == op_failed ) {
                ON_DEBUG_CONET(
                std::cerr << "failed to listen! " << ::strerror(errno) << std::endl;
                )
                return;
            }
            if ( _os == op_timedout ) continue;
            if ( on_accept ) on_accept( peer_t(_addr), std::forward< std::string >(_buffer) );
        }
    }

    // Read data from the socket
    socket_op_status read_from(
        task* ptask,
        string& buffer,
        struct sockaddr_in* addr,
        pe::co::duration_t timedout 
    ) {
        if ( !rawf::has_data_pending((SOCKET_T)ptask->id) ) {
            auto _signal = this_task::wait_other_task_for_event(
                ptask,
                event_type::event_read,
                timedout
            );
            if ( _signal == waiting_signals::no_signal ) return op_timedout;
            if ( _signal == waiting_signals::bad_signal ) return op_failed;
        } else {
            this_task::yield();
        }
        unsigned _solen = sizeof(struct sockaddr_in);
        buffer = std::forward< string >( 
            rawf::read(
                (SOCKET_T)ptask->id,
                std::bind(::recvfrom,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    0,
                    (struct sockaddr *)addr,
                    &_solen
                )
            )
        );
        ON_DEBUG_CONET(std::cout << "<= " << ptask->id << ": " << buffer.size() << std::endl;)
        return (buffer.size() > 0 ? op_done : op_failed);
    }
    socket_op_status read( 
        string& buffer,
        struct sockaddr_in* addr,
        pe::co::duration_t timedout
    ) {
        return udp::read_from(this_task::get_task(), buffer, addr, timedout);
    }
    socket_op_status read(
        char* buffer,
        uint32_t& buflen,
        struct sockaddr_in* addr,
        pe::co::duration_t timedout
    ) {
        if ( !rawf::has_data_pending((SOCKET_T)this_task::get_id()) ) {
            if ( buflen == 0 ) return op_failed;
            auto _signal = this_task::wait_for_event(
                event_type::event_read,
                timedout
            );
            if ( _signal == waiting_signals::no_signal ) return op_timedout;
            if ( _signal == waiting_signals::bad_signal ) return op_failed;
        } else {
            this_task::yield();
        }
        
        unsigned _solen = sizeof(struct sockaddr_in);
        buflen = rawf::read(
            (SOCKET_T)this_task::get_id(),
            buffer,
            buflen,
            std::bind(::recvfrom,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                0,
                (struct sockaddr *)addr,
                &_solen
            )
        );
        ON_DEBUG_CONET(std::cout << "<= " << this_task::get_id() << ": " << buflen << std::endl;)
        return (buflen > 0 ? op_done : op_failed);
    }

    // Write the given data to peer
    socket_op_status write(
        string&& data, 
        struct sockaddr_in addr,
        pe::co::duration_t timedout
    ) {
        return write(data.c_str(), data.size(), addr, timedout);
    }
    socket_op_status write(
        const char* data, 
        uint32_t length,
        struct sockaddr_in addr,
        pe::co::duration_t timedout
    ) {
        if ( length == 0 ) return op_done;
        size_t _sent = 0;
        do {
            int _ret = rawf::write(
                (SOCKET_T)this_task::get_id(),
                data + _sent, length - _sent,
                std::bind(::sendto,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    0 | SO_NETWORK_NOSIGNAL,
                    (struct sockaddr*)&addr,
                    sizeof(addr)
                )
            );
            if ( _ret == -1 ) return op_failed;
            _sent += (size_t)_ret;
        } while ( this_task::wait_for_event_if( 
            _sent < length,
            event_type::event_write,
            timedout
        ) );
        ON_DEBUG_CONET(std::cout << "=> " << this_task::get_id() << ": " << _sent << std::endl;)
        return (_sent == length ? op_done : op_failed);
    }

    socket_op_status write_to( 
        task* ptask, 
        const char* data, 
        uint32_t length, 
        struct sockaddr_in addr,
        pe::co::duration_t timedout 
    ) {
        if ( length == 0 ) return op_done;
        size_t _sent = 0;
        do {
            int _ret = rawf::write(
                (SOCKET_T)ptask->id,
                data + _sent, length - _sent,
                std::bind(::sendto,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    0 | SO_NETWORK_NOSIGNAL,
                    (struct sockaddr*)&addr,
                    sizeof(addr)
                )
            );
            if ( _ret == -1 ) return op_failed;
            _sent += (size_t)_ret;
        } while ( this_task::wait_other_task_for_event_if( 
            _sent < length,
            ptask,
            event_type::event_write,
            timedout
        ) );
        ON_DEBUG_CONET(std::cout << "=> " << ptask->id << ": " << _sent << std::endl;)
        return (_sent == length ? op_done : op_failed);
    }
    // Redirect data between two socket use the
    // established tunnel
    // If there is no tunnel, return false, otherwise
    // after the tunnel has broken, return true
    bool redirect_data( struct sockaddr_in addr, task * ptask, write_to_t hwt ) {
        // Both mark to 1
        ptask->reserved_flags.flag0 = 1;
        this_task::get_task()->reserved_flags.flag0 = 1;

        buffer_guard_16k _sbuf;
        uint32_t _blen = buffer_guard_16k::blen;
        socket_op_status _op = op_failed;
        while ( (_op = udp::read(_sbuf.buf, _blen, &addr)) != op_failed ) {
            // Peer has been closed
            if ( this_task::get_task()->reserved_flags.flag0 == 0 ) break;
            
            if ( _op == op_timedout ) continue;
            if ( op_done != hwt(
                ptask, _sbuf.buf, _blen, 
                NET_DEFAULT_TIMEOUT) ) break;
            _blen = buffer_guard_16k::blen;
        }
        if ( this_task::get_task()->reserved_flags.flag0 == 1 ) {
            ptask->reserved_flags.flag0 = 0;
        }

        return true;
    }

}}}}

// Push Chen
//
