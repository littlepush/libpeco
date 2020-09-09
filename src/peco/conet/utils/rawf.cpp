/*
    rawf.cpp
    PECoNet
    2019-05-12
    Push Chen

    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
*/

#include <peco/conet/utils/rawf.h>
#include <poll.h>

namespace pe { namespace co { namespace net { namespace rawf {
    // Get localhost's computer name on LAN
    const string& hostname() {
        static string _hn;
        if ( _hn.size() == 0 ) {
            char __hostname[256] = { 0 };
            if ( gethostname( __hostname, 256 ) == -1 ) {
                _hn = __hostname;
            }
        }
        return _hn;
    }

    // Get peer info from a socket
    peer_t socket_peerinfo(const SOCKET_T hSo) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return peer_t::nan;

        struct sockaddr_in _addr;
        socklen_t _addrLen = sizeof(_addr);
        memset( &_addr, 0, sizeof(_addr) );
        if ( 0 == getpeername( hSo, (struct sockaddr *)&_addr, &_addrLen ) ) {
            return peer_t(_addr);
        }
        return peer_t::nan;
    }

    // Get local socket's port
    uint16_t localport(const SOCKET_T hSo) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return 0;

        struct sockaddr_in _addr;
        socklen_t _addrLen = sizeof(_addr);
        memset( &_addr, 0, sizeof(_addr) );
        if ( 0 == getsockname( hSo, (struct sockaddr *)&_addr, &_addrLen ) ) {
            return ntohs(_addr.sin_port);
        }
        return 0;
    }

    // Get the socket type 
    uint32_t socktype(SOCKET_T hSo) {
        // Get the type
        struct sockaddr _addr;
        socklen_t _len = sizeof(_addr);
        getsockname(hSo, &_addr, &_len);

        // Check un first
        uint16_t _sfamily = ((struct sockaddr_un*)(&_addr))->sun_family;
        if ( _sfamily == AF_UNIX || _sfamily == AF_LOCAL ) {
            return ST_UNIX_DOMAIN;
        } else {
            int _type;
            _len = sizeof(_type);
            getsockopt( hSo, SOL_SOCKET, SO_TYPE,
                    (char *)&_type, (socklen_t *)&_len);
            if ( _type == SOCK_STREAM ) {
                return ST_TCP_TYPE;
            } else {
                return ST_UDP_TYPE;
            }
        }
    }

    // Set the linger time for a socket
    // I strong suggest not to change this value unless you 
    // know what you are doing
    bool lingertime(SOCKET_T hSo, bool onoff, unsigned timeout) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return false;
        struct linger _sol = { (onoff ? 1 : 0), (int)timeout };
        return ( setsockopt(hSo, SOL_SOCKET, SO_LINGER, &_sol, sizeof(_sol)) == 0 );
    }

    // Set Current socket reusable or not
    bool reusable(SOCKET_T hSo, bool reusable) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return false;
        int _reused = reusable ? 1 : 0;
        bool _r = setsockopt( hSo, SOL_SOCKET, SO_REUSEADDR,
            (const char *)&_reused, sizeof(int) ) != -1;
        ON_DEBUG_CONET(
        if ( !_r ) {
            std::cerr << CLI_YELLOW << "Warning: cannot set the socket as reusable." << CLI_NOR << std::endl;
        }
        )
        return _r;
    }

    // Make current socket keep alive
    bool keepalive(SOCKET_T hSo, bool keepalive) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return false;
        int _keepalive = keepalive ? 1 : 0;
        return setsockopt( hSo, SOL_SOCKET, SO_KEEPALIVE, 
            (const char *)&_keepalive, sizeof(int) );
    }

    bool __setINetNonblocking(SOCKET_T hSo, bool nonblocking) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return false;
        unsigned long _u = (nonblocking ? 1 : 0);
        bool _r = SO_NETWORK_IOCTL_CALL(hSo, FIONBIO, &_u) >= 0;
        ON_DEBUG_CONET(
        if ( !_r ) {
            std::cerr << CLI_YELLOW << "Warning: cannot change the nonblocking flag of socket." 
                << CLI_NOR << std::endl;
        })
        return _r;
    }

    bool __setUDSNonblocking(SOCKET_T hSo, bool nonblocking) {
        // Set NonBlocking
        int _f = fcntl(hSo, F_GETFL, NULL);
        if ( _f < 0 ) {
            ON_DEBUG_CONET(
            std::cerr << CLI_YELLOW << "Warning: cannot get UDS socket file info." 
                << CLI_NOR << std::endl;
            )
            return false;
        }
        if ( nonblocking ) {
            _f |= O_NONBLOCK;
        } else {
            _f &= (~O_NONBLOCK);
        }
        if ( fcntl(hSo, F_SETFL, _f) < 0 ) {
            ON_DEBUG_CONET(
            std::cerr << CLI_YELLOW <<
                "Warning: failed to change the nonblocking flag of UDS socket." <<
                CLI_NOR << std::endl;
            )
            return false;
        }
        return true;
    }

    // Make a socket to be nonblocking
    bool nonblocking(SOCKET_T hSo, bool nonblocking) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return false;
        uint32_t _st = socktype(hSo);
        if ( _st == ST_UNIX_DOMAIN ) return __setUDSNonblocking(hSo, nonblocking);
        else return __setINetNonblocking(hSo, nonblocking);
    }
    bool nodelay(SOCKET_T hSo, bool nodelay) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return false;
        if ( socktype(hSo) != ST_TCP_TYPE ) return true;
        // Try to set as tcp-nodelay
        int _flag = nodelay ? 1 : 0;
        bool _r = (setsockopt( hSo, IPPROTO_TCP, 
            TCP_NODELAY, (const char *)&_flag, sizeof(int) ) != -1);
        ON_DEBUG_CONET(
        if ( !_r ) {
            std::cerr << CLI_YELLOW << "Warning: cannot set the socket to be tcp-nodelay" 
                << CLI_NOR << std::endl;
        })
        return _r;
    }    
    // Set the socket's buffer size
    bool buffersize(SOCKET_T hSo, uint32_t rmem, uint32_t wmem) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return false;

        if ( rmem != 0 ) {
            setsockopt(hSo, SOL_SOCKET, SO_RCVBUF, 
                    (char *)&rmem, sizeof(rmem));
        }
        if ( wmem != 0 ) {
            setsockopt(hSo, SOL_SOCKET, SO_SNDBUF,
                    (char *)&wmem, sizeof(wmem));
        }
        return true;
    }
    // Check if a socket has data to read
    bool has_data_pending(SOCKET_T hSo) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return false;
        fd_set _fs;
        FD_ZERO( &_fs );
        FD_SET( hSo, &_fs );

        int _ret = 0; struct timeval _tv = {0, 0};

        do {
            _ret = ::select( hSo + 1, &_fs, NULL, NULL, &_tv );
        } while ( _ret < 0 && errno == EINTR );
        return (_ret > 0);
    }

    // Check if a socket has buffer to write
    bool has_buffer_outgoing(SOCKET_T hSo) {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return false;
        fd_set _fs;
        FD_ZERO( &_fs );
        FD_SET( hSo, &_fs );

        int _ret = 0; struct timeval _tv = {0, 0};
        do {
            _ret = ::select( hSo + 1, NULL, &_fs, NULL, &_tv );
        } while ( _ret < 0 && errno == EINTR );
        return (_ret > 0);
    }
    // Create Socket Handler
    SOCKET_T createSocket(int SOCK_TYPE) {
        SOCKET_T _so = INVALIDATE_SOCKET;
        if ( SOCK_TYPE == ST_TCP_TYPE ) {
            _so = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        } else if ( SOCK_TYPE == ST_UDP_TYPE ) {
            _so = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        } else if ( SOCK_TYPE == ST_UNIX_DOMAIN ) {
            _so = ::socket(AF_UNIX, SOCK_STREAM, 0);
        } else {
            ON_DEBUG_CONET(
            std::cerr << CLI_RED << "Error: Cannot create new socket with unknow type <" \
                << SOCK_TYPE << ">" << CLI_NOR << std::endl;
            )
            return _so;
        }

        if ( SOCKET_NOT_VALIDATE(_so) ) {
            ON_DEBUG_CONET(
            std::cerr << CLI_RED << "Error: Failed to create socket with type <" \
                << SOCK_TYPE << ">" << CLI_NOR << std::endl;
            )
            return _so;
        }

        // Set the socket to be nonblocking;
        nonblocking(_so, true);
        nodelay(_so, true);
        reusable(_so, true);
        keepalive(_so, true);
        return _so;
    }

    // Read Data From a socket
    size_t read(
        SOCKET_T hSo,
        char * buffer,
        size_t length,
        std::function<int (SOCKET_T, char *, size_t)> f)
    {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return 0;
        size_t BUF_SIZE = length;
        size_t _received = 0;
        size_t _leftspace = BUF_SIZE;

        do {
            int _retCode = f(hSo, buffer + _received, _leftspace);
            if ( _retCode < 0 ) {
                if ( errno == EINTR ) continue; // Signal 7, retry
                if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
                    // No more incoming data in this socket's cache
                    return _received;
                }
                ON_DEBUG_CONET(
                std::cerr << CLI_YELLOW << "Error: failed to receive data on socket("
                    << hSo << "), " << ::strerror( errno ) << CLI_NOR << std::endl;
                )
                return 0;
            } else if ( _retCode == 0 ) {
                ON_DEBUG_CONET(
                    std::cout << CLI_BLUE << "Peer has closed the socket(" << hSo << ")"
                        << CLI_NOR << std::endl;
                )
                return 0;
            } else {
                _received += _retCode;
                _leftspace -= _retCode;
            }
        } while ( _leftspace > 0 );
        return _received;
    }
    string read(
        SOCKET_T hSo, 
        std::function<int (SOCKET_T, char *, size_t)> f,
        uint32_t max_buffer_size) 
    {
        if ( SOCKET_NOT_VALIDATE(hSo) ) return string("");
        bool _hasLimit = (max_buffer_size != 0);
        size_t BUF_SIZE = (_hasLimit ? max_buffer_size : 1024);  // 1KB
        string _buffer(BUF_SIZE, '\0');
        //_buffer.resize(BUF_SIZE);
        size_t _received = 0;
        size_t _leftspace = BUF_SIZE;

        do {
            int _retCode = f(hSo, &_buffer[0] + _received, _leftspace);
            if ( _retCode < 0 ) {
                if ( errno == EINTR ) continue;    // signal 7, retry
                if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
                    // No more data on a non-blocking socket
                    _buffer.resize(_received);
                    break;
                }
                // Other error
                _buffer.resize(0);
                ON_DEBUG_CONET(
                std::cerr << CLI_YELLOW << "Error: Failed to receive data on socket(" 
                    << hSo << ", " << ::strerror( errno ) << CLI_NOR << std::endl;
                )
                break;
            } else if ( _retCode == 0 ) {
                // Peer Close
                _buffer.resize(0);
                break;
            } else {
                _received += _retCode;
                _leftspace -= _retCode;
                if ( _leftspace > 0 ) {
                    // Unfull
                    _buffer.resize(_received);
                    break;
                } else {
                    // If has limit, and left space is zero, 
                    // which means the data size has reach
                    // the limit and we should not read any more
                    // data from the socket this time.
                    if ( _hasLimit ) break;

                    // Otherwise, try to double the buffer
                    // The buffer is full, try to double the buffer and try again
                    if ( BUF_SIZE * 2 <= _buffer.max_size() ) {
                        BUF_SIZE *= 2;
                    } else if ( BUF_SIZE < _buffer.max_size() ) {
                        BUF_SIZE = _buffer.max_size();
                    } else {
                        break;    // direct return, wait for next read.
                    }
                    // Resize the buffer and try to read again
                    _leftspace = BUF_SIZE - _received;
                    _buffer.resize(BUF_SIZE);
                }
            }
        } while ( true );
        return _buffer;
    }
    // Write Data to a socket
    int write(
        SOCKET_T hSo, 
        const char* data, 
        size_t data_lenth,
        std::function<int (SOCKET_T, const char*, size_t)> f) {
        size_t _sent = 0;
        while ( _sent < data_lenth ) {
            int _ret = f(hSo, data + _sent, data_lenth - _sent);
            if ( _ret < 0 ) {
                if ( ENOBUFS == errno || EAGAIN == errno || EWOULDBLOCK == errno ) {
                    // No buf
                    break;
                } else {
                    ON_DEBUG_CONET(
                    std::cerr << CLI_YELLOW << "Failed to send data on socket(" 
                        << hSo << "), " << ::strerror(errno) << CLI_NOR << std::endl;
                    )
                    return _ret;
                }
            } else if ( _ret == 0 ) {
                break;
            } else {
                _sent += _ret;
            }
        }
        return (int)_sent;
    }

}}}}

// Push Chen
//
