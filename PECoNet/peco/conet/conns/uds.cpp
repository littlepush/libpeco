/*
    uds.cpp
    PECoNet
    2019-05-17
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */

#include <peco/conet/conns/uds.h>
#include <peco/peutils.h>
#include <signal.h>

namespace pe { namespace co { namespace net { 

    // Create a socket, and bind to the given peer info
    SOCKET_T uds::create( const std::string& filepath ) {
        SOCKET_T _so = INVALIDATE_SOCKET;
        do {
            _so = rawf::createSocket(ST_UNIX_DOMAIN);
        } while ( pe::co::this_task::yield_if(SOCKET_NOT_VALIDATE(_so)) );

        if ( filepath.size() > 0 ) {
            // Get the path part, and create it with "mkdir -p"
            size_t _last_ = filepath.rfind("/");
            std::string _path_part = filepath.substr(0, _last_);
            if ( !pe::utils::rek_make_dir(_path_part) ) {
                std::cerr << "Failed to create UDS socket path: " << _path_part << std::endl;
                ::close(_so);
                return INVALIDATE_SOCKET;
            }

            struct sockaddr_un  _addr;
            bzero(&_addr, sizeof(_addr));
            strcpy(_addr.sun_path, filepath.c_str());

            mode_t _cm = umask(S_IXUSR | S_IXGRP | S_IXOTH);

            _addr.sun_family = AF_UNIX;
        #ifdef __APPLE__
            int _addrlen = filepath.size() + sizeof(_addr.sun_family) + sizeof(_addr.sun_len);
            _addr.sun_len = sizeof(_addr);
        #else
            int _addrlen = filepath.size() + sizeof(_addr.sun_family);
        #endif
            unlink(filepath.c_str());

            if ( ::bind(_so, (struct sockaddr*)&_addr, _addrlen) < 0 ) {
                ON_DEBUG_CONET(
                std::cerr << "Failed to bind uds socket on: " << filepath
                    << ", " << ::strerror( errno ) << std::endl;
                )
                ::close(_so);
                return INVALIDATE_SOCKET;
            }

            umask(_cm);
        } else {
            // Set a 32K buffer
            rawf::buffersize(_so, 1024 * 32, 1024 * 32);
        }

        // int _nopipe = 1;
        // setsockopt(_so, SOL_SOCKET, SO_NOSIGPIPE, (void *)&_nopipe, sizeof(int));
        return _so;
    }

    // Listen on the socket, the socket must has been bound
    // to the listen port, which means the parameter of the
    // create function cannot be nan
    void uds::listen(
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
                    rawf::reusable(_inso, true);
                    rawf::keepalive(_inso, true);
                    // Set a 32K buffer
                    rawf::buffersize(_inso, 1024 * 32, 1024 * 32);
                }
                // Start a new task for the incoming socket
                this_loop.do_job( _inso, on_accept );
            }
        }
    }

    // Connect to a host and port
    socket_op_status uds::connect(
        const string& filepath,
        pe::co::duration_t timedout
    ) {
        struct sockaddr_un  _peeraddr;
        bzero(&_peeraddr, sizeof(_peeraddr));
        strcpy(_peeraddr.sun_path, filepath.c_str());
        _peeraddr.sun_family = AF_UNIX;
    #ifdef __APPLE__
        int _addrlen = filepath.size() + sizeof(_peeraddr.sun_family) + sizeof(_peeraddr.sun_len);
        _peeraddr.sun_len = sizeof(_peeraddr);
    #else
        int _addrlen = filepath.size() + sizeof(_peeraddr.sun_family);
    #endif

        if ( -1 == ::connect((SOCKET_T)this_task::get_id(), 
                    (struct sockaddr *)&_peeraddr, _addrlen) ) {
            ON_DEBUG_CONET(
            std::cerr << "Error: Failed to connect to " << filepath << 
                " on uds socket(" << this_task::get_id() << "), " << ::strerror( errno ) <<
                std::endl;
            )
            return op_failed;
        }
        return op_done;
    }

    // Read data from the socket
    socket_op_status uds::read_from(
        task* ptask,
        string& buffer,
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

        buffer = std::forward< string >( 
            rawf::read(
                (SOCKET_T)ptask->id,
                std::bind(::recv,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    0 | SO_NETWORK_NOSIGNAL
                )
            )
        );
        ON_DEBUG_CONET(std::cout << "<= " << ptask->id << ": " << buffer.size() << std::endl;)
        return (buffer.size() > 0 ? op_done : op_failed);
    }
    socket_op_status uds::read( 
        string& buffer,
        pe::co::duration_t timedout
    ) {
        return uds::read_from(this_task::get_task(), buffer, timedout);
    }
    socket_op_status uds::read(
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
                0 | SO_NETWORK_NOSIGNAL
            )
        );
        ON_DEBUG_CONET(std::cout << "<= " << this_task::get_id() << ": " << buflen << std::endl;)
        return (buflen > 0 ? op_done : op_failed);
    }

    // Write the given data to peer
    socket_op_status uds::write(
        string&& data, 
        pe::co::duration_t timedout
    ) {
        return uds::write_to(
            this_task::get_task(), 
            data.c_str(), data.size(), timedout);
    }
    socket_op_status uds::write(
        const char* data, 
        uint32_t length,
        pe::co::duration_t timedout
    ) {
        return uds::write_to(
            this_task::get_task(), 
            data, length, timedout);
    }

    socket_op_status uds::write_to( 
        task* ptask, 
        const char* data, 
        uint32_t length, 
        pe::co::duration_t timedout 
    ) {
        if ( length == 0 ) return op_done;
        size_t _sent = 0;
        do {
            int _ret = rawf::write(
                (SOCKET_T)ptask->id,
                data + _sent, length - _sent,
                std::bind(::send,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    0 | SO_NETWORK_NOSIGNAL
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
    bool uds::redirect_data( task * ptask, write_to_t hwt ) {
        // Both mark to 1
        ptask->reserved_flags.flag0 = 1;
        this_task::get_task()->reserved_flags.flag0 = 1;

        buffer_guard_16k _sbuf;
        uint32_t _blen = buffer_guard_16k::blen;
        socket_op_status _op = op_failed;
        while ( (_op = uds::read(_sbuf.buf, _blen)) != op_failed ) {
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

}}}

// Push Chen
// 

