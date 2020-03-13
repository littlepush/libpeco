/*
    tcp.cpp
    PECoNet
    2019-05-13
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */

#include <peco/conet/conns/tcp.h>
#include <peco/conet/protos/dns.h>
#include <peco/conet/protos/socks5.h>

namespace pe { namespace co { namespace net { 
    static bool __f_reuseport = false;
    static int __f_reuseport_flag = 1;

    // Set the flag to reuse listening port
    void tcp::reuse_port( bool on ) {
        __f_reuseport = on;
    }

    // Create a socket, and bind to the given peer info
    SOCKET_T tcp::create( peer_t bind_addr ) {
        SOCKET_T _so = INVALIDATE_SOCKET;
        do {
            _so = rawf::createSocket(ST_TCP_TYPE);
        } while ( pe::co::this_task::yield_if(SOCKET_NOT_VALIDATE(_so)) );

        if ( (bool)bind_addr ) {
            // Check if set the reuse port
            if ( __f_reuseport ) {
                setsockopt( _so, SOL_SOCKET, SO_REUSEPORT, 
                    (const char *)&__f_reuseport_flag, sizeof(int) );
            }

            // Bind to the address
            struct sockaddr_in _sock_addr;
            memset((char *)&_sock_addr, 0, sizeof(_sock_addr));
            _sock_addr.sin_family = AF_INET;
            _sock_addr.sin_port = htons(bind_addr.port);
            _sock_addr.sin_addr.s_addr = (uint32_t)bind_addr.ip;

            if ( ::bind(_so, (struct sockaddr *)&_sock_addr, sizeof(_sock_addr)) == -1 ) {
                ON_DEBUG_CONET(
                std::cerr << "Failed to bind tcp socket on: " << bind_addr.str()
                    << ", " << ::strerror( errno ) << std::endl;
                )
                ::close(_so);
                _so = INVALIDATE_SOCKET;
            }
        } else {
            // Set a 32K buffer
            rawf::buffersize(_so, 1024 * 32, 1024 * 32);
        }
        return _so;
    }

    // Listen on the socket, the socket must has been bound
    // to the listen port, which means the parameter of the
    // create function cannot be nan
    void tcp::listen(
        std::function< void( ) > on_accept
    ) {
        if ( -1 == ::listen((SOCKET_T)this_task::get_id(), CO_MAX_SO_EVENTS) ) {
            ON_DEBUG_CONET(
            std::cerr << "failed to listen on socket(" <<
                this_task::get_id() << "), error: " << ::strerror(errno) 
                << std::endl;
            )
            return;
        }
        while ( true ) {
            auto _signal = pe::co::this_task::wait_for_event(
                pe::co::event_type::event_read,
                std::chrono::hours(1)
            );
            if ( _signal == pe::co::waiting_signals::bad_signal ) {
                ON_DEBUG_CONET(
                std::cerr << "failed to listen! " << ::strerror(errno) << std::endl;
                )
                return;
            }
            if ( _signal == pe::co::waiting_signals::no_signal ) continue;

            // Get the incoming socket
            while ( true ) {
                struct sockaddr _inaddr;
                socklen_t _inlen = 0;
                memset(&_inaddr, 0, sizeof(_inaddr));
                SOCKET_T _inso = accept( 
                    (SOCKET_T)pe::co::this_task::get_id(), &_inaddr, &_inlen );
                if ( _inso == -1 ) {
                    // No more incoming
                    if ( errno == EAGAIN || errno == EWOULDBLOCK ) break;
                    // Or error
                    return;
                } else {
                    // Set non-blocking
                    rawf::nonblocking(_inso, true);
                    rawf::nodelay(_inso, true);
                    rawf::reusable(_inso, true);
                    rawf::keepalive(_inso, true);
                    // Set a 32K buffer
                    rawf::buffersize(_inso, 1024 * 32, 1024 * 32);
                }
                // Start a new task for the incoming socket
                pe::co::loop::main.do_job( _inso, on_accept );
            }
        }
    }

    // Connect to peer, default 3 seconds for timedout
    socket_op_status tcp::connect( 
        peer_t remote,
        pe::co::duration_t timedout
    ) {
        SOCKET_T so = (SOCKET_T)pe::co::this_task::get_id();
        struct sockaddr_in _sock_addr;
        memset(&_sock_addr, 0, sizeof(_sock_addr));
        _sock_addr.sin_addr.s_addr = (uint32_t)remote.ip;
        _sock_addr.sin_family = AF_INET;
        _sock_addr.sin_port = htons(remote.port);

        if ( ::connect(so, (struct sockaddr *)&_sock_addr, sizeof(_sock_addr)) == -1 ) {
            int _error = 0, _len = sizeof(_error);
            getsockopt(
                so, SOL_SOCKET, SO_ERROR, (char *)&_error,
                (socklen_t *)&_len);
            if ( _error != 0 ) {
                ON_DEBUG_CONET(
                std::cerr << "Error: Failed to connect to " << remote.str() << 
                    " on tcp socket(" << so << "), " << ::strerror( _error ) <<
                    std::endl;
                )
                return op_failed;
            } else {
                // Async monitor
                auto _signal = pe::co::this_task::wait_for_event(
                    pe::co::event_type::event_write,
                    timedout
                );

                if ( _signal == pe::co::waiting_signals::bad_signal ) {
                    ON_DEBUG_CONET(
                    std::cerr << "Error: Failed to connect to " << remote.str() <<
                        " on tcp socket(" << so << "), " << ::strerror( _error ) <<
                        std::endl;
                    )
                    // Failed
                    return op_failed;
                }
                if ( _signal == pe::co::waiting_signals::no_signal ) {
                    // Timedout
                    ON_DEBUG_CONET(
                    std::cerr << "Error: Timedout on connection to " << remote.str() <<
                        " on tcp socket(" << so << ")." << std::endl;
                    )
                    return op_timedout;
                }
                ON_DEBUG_CONET(
                    std::cout << "Connected to " << remote.str() << " using tcp socket(" <<
                        so << ") on local port " << rawf::localport(so) << std::endl;
                )
                return op_done;
            } 
        } else {
            ON_DEBUG_CONET(
                std::cout << "Connect to " << remote.str() << 
                    " is so fast using tcp socket(" << 
                    so << ") on localport" << rawf::localport(so) << std::endl;
            )
            return op_done;
        }
    }

    // Connect to a host and port
    socket_op_status tcp::connect(
        const string& hostname, 
        uint16_t port,
        pe::co::duration_t timedout
    ) {
        ip_t _ip(hostname);
        if ( !_ip.is_valid() ) {
            _ip = pe::co::net::get_hostname(hostname);
        }
        peer_t _p(_ip, port);
        return connect(_p, timedout);
        return op_failed;
    }

    // Read data from the socket
    socket_op_status tcp::read_from(
        task_t ptask,
        string& buffer,
        pe::co::duration_t timedout
    ) {
        if ( !rawf::has_data_pending((SOCKET_T)task_get_id(ptask)) ) {
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

        buffer = std::forward< string >( 
            rawf::read(
                (SOCKET_T)task_get_id(ptask),
                std::bind(::recv,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    0 | SO_NETWORK_NOSIGNAL)
            )
        );
        ON_DEBUG_CONET(std::cout << "<= " << task_get_id(ptask) << ": " << buffer.size() << std::endl;)
        return (buffer.size() > 0 ? op_done : op_failed);
    }
    socket_op_status tcp::read( 
        string& buffer,
        pe::co::duration_t timedout
    ) {
        return tcp::read_from(this_task::get_task(), buffer, timedout);
    }
    socket_op_status tcp::read(
        char* buffer,
        uint32_t& buflen,
        pe::co::duration_t timedout
    ) {
        if ( buflen == 0 ) return op_failed;
        if ( !rawf::has_data_pending((SOCKET_T)this_task::get_id()) ) {
            auto _signal = this_task::wait_for_event(
                event_type::event_read,
                timedout
            );
            if ( _signal == waiting_signals::no_signal ) return op_timedout;
            if ( _signal == waiting_signals::bad_signal ) return op_failed;
        } else {
            this_task::yield();
        }

        buflen = rawf::read(
            (SOCKET_T)this_task::get_id(),
            buffer,
            buflen,
            std::bind(::recv,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                0 | SO_NETWORK_NOSIGNAL)
        );
        ON_DEBUG_CONET(std::cout << "<= " << this_task::get_id() << ": " << buflen << std::endl;)
        return (buflen > 0 ? op_done : op_failed);
    }

    // Write the given data to peer
    socket_op_status tcp::write(
        string&& data, 
        pe::co::duration_t timedout
    ) {
        return write(data.c_str(), data.size(), timedout);
    }
    socket_op_status tcp::write(
        const char* data, 
        uint32_t length,
        pe::co::duration_t timedout
    ) {
        return tcp::write_to( 
            this_task::get_task(), 
            data, length, timedout 
        );
    }
    socket_op_status tcp::write_to( 
        task_t ptask, 
        const char* data, 
        uint32_t length, 
        pe::co::duration_t timedout 
    ) {
        if ( length == 0 ) return op_done;
        size_t _sent = 0;
        do {
            // Single Package max size is 4k
            int _single_pkg = std::min((size_t)(length - _sent), (size_t)(4 * 1024));
            int _ret = rawf::write(
                (SOCKET_T)task_get_id(ptask),
                data + _sent, _single_pkg,
                std::bind(::send,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    0 | SO_NETWORK_NOSIGNAL)
            );
            if ( _ret == -1 ) return op_failed;
            _sent += (size_t)_ret;
        } while ( this_task::wait_other_task_for_event_if(
            _sent < length,
            ptask,
            event_type::event_write,
            timedout
        ) );
        ON_DEBUG_CONET(std::cout << "=> " << task_get_id(ptask) << ": " << _sent << std::endl;)
        return (_sent == length ? op_done : op_failed);
    }


    // Redirect data between two socket use the
    // established tunnel
    // If there is no tunnel, return false, otherwise
    // after the tunnel has broken, return true
    bool tcp::redirect_data( task_t ptask, write_to_t hwt ) {
        // Both mark to 1
        task_set_user_flag0(ptask, 1);
        this_task::set_user_flag0(1);

        buffer_guard_16k _sbuf;
        uint32_t _blen = buffer_guard_16k::blen;
        socket_op_status _op = op_failed;
        while ( (_op = tcp::read(_sbuf.buf, _blen)) != op_failed ) {
            // Peer has been closed
            if ( this_task::get_user_flag0() == 0 ) break;
            
            if ( _op == op_timedout ) continue;
            if ( op_done != hwt(
                ptask, _sbuf.buf, _blen, 
                NET_DEFAULT_TIMEOUT) ) break;
            _blen = buffer_guard_16k::blen;
        }
        if ( this_task::get_user_flag0() == 1 ) {
            task_set_user_flag0(ptask, 0);
        }

        return true;
    }

    typedef std::function< socket_op_status () >    __re_connect_t;

    void _redirect_to_tcp( __re_connect_t rc ) {
        SOCKET_T _rso = tcp::create();
        task_t _ptask = NULL;
        loop::main.do_job(_rso, [&_ptask, rc]() {
            do {
                parent_task::guard _g;
                if ( op_done != rc() ) return;
                _g.job_done();
                _ptask = this_task::get_task();
            } while ( false );
            // Redirect to parent task
            tcp::redirect_data( parent_task::get_task() );
        });
        if ( ! this_task::holding() ) return;
        tcp::redirect_data( _ptask );
    }

    // Simple Redirect to Another TCP
    void tcp::redirect_to_tcp( peer_t remote ) {
        _redirect_to_tcp( [remote]() -> socket_op_status {
            return tcp::connect(remote);
        } );
    }
    void tcp::redirect_to_tcp( const string& hostname, uint16_t port ) {
        _redirect_to_tcp( [hostname, port]() -> socket_op_status {
            return tcp::connect(hostname, port);
        } );
    }
    void tcp::redirect_to_tcp( peer_t remote, peer_t socks5 ) {
        if ( ! socks5 ) {
            redirect_to_tcp( remote );
            return;
        }
        _redirect_to_tcp( [remote, socks5]() -> socket_op_status {
            return socks5_connect( socks5, remote );
        } );
    }
    void tcp::redirect_to_tcp( const string& hostname, uint16_t port, peer_t socks5 ) {
        if ( ! socks5 ) {
            redirect_to_tcp( hostname, port );
            return;
        }
        ip_t _ip(hostname);
        if ( _ip.is_valid() ) {
            redirect_to_tcp( peer_t(_ip, port), socks5 );
            return;
        }
        _redirect_to_tcp( [hostname, port, socks5]() -> socket_op_status {
            return socks5_connect( socks5, hostname, port );
        } );
    }
}}}

// Push Chen
// 

