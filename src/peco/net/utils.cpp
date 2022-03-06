/*
    utils.cpp
    libpeco
    2022-02-17
    Push Chen
*/

/*
MIT License

Copyright (c) 2019 Push Chen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "peco/net/utils.h"
#include "peco/utils.h"

#if PECO_TARGET_LINUX
#include <endian.h>
#elif PECO_TARGET_APPLE
#include <libkern/OSByteOrder.h>
#include <machine/endian.h>
#else
// Windows?
#endif

#if !PECO_TARGET_WIN
#include <fcntl.h>
#include <sys/un.h>
#endif

namespace peco {
namespace net_utils {

uint16_t h2n(uint16_t v) { return htons(v); }
uint32_t h2n(uint32_t v) { return htonl(v); }
uint64_t h2n(uint64_t v) {
#if PECO_TARGET_OPENOS
  return htobe64(v);
#else
  return htonll(v);
#endif
}
float h2n(float v) {
  uint32_t *_iv = (uint32_t *)(&v);
  uint32_t _nv = h2n(*_iv);
  uint32_t *_pnv = &_nv;
  return *(float *)_pnv;
}
double h2n(double v) {
  uint64_t *_iv = (uint64_t *)(&v);
  uint64_t _nv = h2n(*_iv);
  uint64_t *_pnv = &_nv;
  return *(double *)_pnv;
}

uint16_t n2h(uint16_t v) { return ntohs(v); }
uint32_t n2h(uint32_t v) { return ntohl(v); }
uint64_t n2h(uint64_t v) {
#if PECO_TARGET_OPENOS
  return be64toh(v);
#else
  return ntohll(v);
#endif
}
float n2h(float v) {
  uint32_t *_iv = (uint32_t *)(&v);
  uint32_t _nv = n2h(*_iv);
  uint32_t *_pnv = &_nv;
  return *(float *)_pnv;
}
double n2h(double v) {
  uint64_t *_iv = (uint64_t *)(&v);
  uint64_t _nv = n2h(*_iv);
  uint64_t *_pnv = &_nv;
  return *(double *)_pnv;
}

// Convert host endian to little endian
#if PECO_TARGET_OPENOS
uint16_t h2le(uint16_t v) { return htole16(v); }
uint32_t h2le(uint32_t v) { return htole32(v); }
uint64_t h2le(uint64_t v) { return htole64(v); }
#elif PECO_TARGET_APPLE
uint16_t h2le(uint16_t v) { return OSSwapHostToLittleInt16(v); }
uint32_t h2le(uint32_t v) { return OSSwapHostToLittleInt32(v); }
uint64_t h2le(uint64_t v) { return OSSwapHostToLittleInt64(v); }
#else // Windows
uint16_t h2le(uint16_t v) { return v; }
uint32_t h2le(uint32_t v) { return v; }
uint64_t h2le(uint64_t v) { return v; }
#endif
float h2le(float v) {
  uint32_t *_iv = (uint32_t *)(&v);
  uint32_t _lev = h2le(*_iv);
  uint32_t *_plev = &_lev;
  return *(float *)_plev;
}
double h2le(double v) {
  uint64_t *_iv = (uint64_t *)(&v);
  uint64_t _lev = h2le(*_iv);
  uint64_t *_plev = &_lev;
  return *(double *)_plev;
}

// Little Endian to Host
#if PECO_TARGET_OPENOS
uint16_t le2h(uint16_t v) { return le16toh(v); }
uint32_t le2h(uint32_t v) { return le32toh(v); }
uint64_t le2h(uint64_t v) { return le64toh(v); }
#elif PECO_TARGET_APPLE
uint16_t le2h(uint16_t v) { return OSSwapLittleToHostInt16(v); }
uint32_t le2h(uint32_t v) { return OSSwapLittleToHostInt32(v); }
uint64_t le2h(uint64_t v) { return OSSwapLittleToHostInt64(v); }
#else // Windows
uint16_t le2h(uint16_t v) { return v; }
uint32_t le2h(uint32_t v) { return v; }
uint64_t le2h(uint64_t v) { return v; }
#endif
float le2h(float v) {
  uint32_t *_iv = (uint32_t *)(&v);
  uint32_t _lev = le2h(*_iv);
  uint32_t *_plev = &_lev;
  return *(float *)_plev;
}
double le2h(double v) {
  uint64_t *_iv = (uint64_t *)(&v);
  uint64_t _lev = le2h(*_iv);
  uint64_t *_plev = &_lev;
  return *(double *)_plev;
}

// Convert host endian to big endian
#if PECO_TARGET_OPENOS
uint16_t h2be(uint16_t v) { return htobe16(v); }
uint32_t h2be(uint32_t v) { return htobe32(v); }
uint64_t h2be(uint64_t v) { return htobe64(v); }
#elif PECO_TARGET_APPLE
uint16_t h2be(uint16_t v) { return OSSwapHostToBigInt16(v); }
uint32_t h2be(uint32_t v) { return OSSwapHostToBigInt32(v); }
uint64_t h2be(uint64_t v) { return OSSwapHostToBigInt64(v); }
#else
#if defined(_MSC_VER)
uint16_t h2be(uint16_t v) { return _byteswap_ushort(v); }
uint32_t h2be(uint32_t v) { return _byteswap_ulong(v); }
uint64_t h2be(uint64_t v) { return _byteswap_uint64(v); }
#elif PECO_USE_GNU
uint16_t h2be(uint16_t v) { return __builtin_bswap16(v); }
uint32_t h2be(uint32_t v) { return __builtin_bswap32(v); }
uint64_t h2be(uint64_t v) { return __builtin_bswap64(v); }
#endif

#endif
float h2be(float v) {
  uint32_t *_iv = (uint32_t *)(&v);
  uint32_t _lev = h2be(*_iv);
  uint32_t *_plev = &_lev;
  return *(float *)_plev;
}
double h2be(double v) {
  uint64_t *_iv = (uint64_t *)(&v);
  uint64_t _lev = h2be(*_iv);
  uint64_t *_plev = &_lev;
  return *(double *)_plev;
}

// Big Endian to Host
#if PECO_TARGET_OPENOS
uint16_t be2h(uint16_t v) { return be16toh(v); }
uint32_t be2h(uint32_t v) { return be32toh(v); }
uint64_t be2h(uint64_t v) { return be64toh(v); }
#elif PECO_TARGET_APPLE
uint16_t be2h(uint16_t v) { return OSSwapBigToHostInt16(v); }
uint32_t be2h(uint32_t v) { return OSSwapBigToHostInt32(v); }
uint64_t be2h(uint64_t v) { return OSSwapBigToHostInt64(v); }
#else
#if defined(_MSC_VER)
uint16_t be2h(uint16_t v) { return _byteswap_ushort(v); }
uint32_t be2h(uint32_t v) { return _byteswap_ulong(v); }
uint64_t be2h(uint64_t v) { return _byteswap_uint64(v); }
#elif PECO_USE_GNU
uint16_t be2h(uint16_t v) { return __builtin_bswap16(v); }
uint32_t be2h(uint32_t v) { return __builtin_bswap32(v); }
uint64_t be2h(uint64_t v) { return __builtin_bswap64(v); }
#endif
#endif
float be2h(float v) {
  uint32_t *_iv = (uint32_t *)(&v);
  uint32_t _lev = be2h(*_iv);
  uint32_t *_plev = &_lev;
  return *(float *)_plev;
}
double be2h(double v) {
  uint64_t *_iv = (uint64_t *)(&v);
  uint64_t _lev = be2h(*_iv);
  uint64_t *_plev = &_lev;
  return *(double *)_plev;
}

// Get localhost's computer name on LAN
const std::string &hostname() {
  static std::string _hn;
  if (_hn.size() == 0) {
    char __hostname[256] = {0};
    if (gethostname(__hostname, 256) == -1) {
      _hn = __hostname;
    }
  }
  return _hn;
}

// Get peer info from a socket
peer_t socket_peerinfo(const SOCKET_T hSo) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return peer_t::nan;

  struct sockaddr_in _addr;
  socklen_t _addrLen = sizeof(_addr);
  memset(&_addr, 0, sizeof(_addr));
  if (0 == getpeername(hSo, (struct sockaddr *)&_addr, &_addrLen)) {
    return peer_t(_addr);
  }
  return peer_t::nan;
}

// Get local socket's port
uint16_t localport(const SOCKET_T hSo) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return 0;

  struct sockaddr_in _addr;
  socklen_t _addrLen = sizeof(_addr);
  memset(&_addr, 0, sizeof(_addr));
  if (0 == getsockname(hSo, (struct sockaddr *)&_addr, &_addrLen)) {
    return ntohs(_addr.sin_port);
  }
  return 0;
}

// Get the socket type
SocketType socktype(SOCKET_T hSo) {
  // Get the type
  struct sockaddr _addr;
  socklen_t _len = sizeof(_addr);
  getsockname(hSo, &_addr, &_len);

  // Check un first
  uint16_t _sfamily = ((struct sockaddr_un *)(&_addr))->sun_family;
  if (_sfamily == AF_UNIX || _sfamily == AF_LOCAL) {
    return kSocketTypeUnixDomain;
  } else {
    int _type;
    _len = sizeof(_type);
    getsockopt(hSo, SOL_SOCKET, SO_TYPE, (char *)&_type, (socklen_t *)&_len);
    if (_type == SOCK_STREAM) {
      return kSocketTypeTCP;
    } else {
      return kSocketTypeUDP;
    }
  }
}

// Set the linger time for a socket
// I strong suggest not to change this value unless you
// know what you are doing
bool lingertime(SOCKET_T hSo, bool onoff, unsigned timeout) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return false;
  struct linger _sol = {(onoff ? 1 : 0), (int)timeout};
  return (setsockopt(hSo, SOL_SOCKET, SO_LINGER, &_sol, sizeof(_sol)) == 0);
}

// Set Current socket reusable or not
bool reusable(SOCKET_T hSo, bool reusable) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return false;
  int _reused = reusable ? 1 : 0;
  bool _r = setsockopt(hSo, SOL_SOCKET, SO_REUSEADDR, (const char *)&_reused,
                       sizeof(int)) != -1;
  if (!_r) {
    log::warning << "Warning: cannot set the socket as reusable." << std::endl;
  }
  return _r;
}

// Make current socket keep alive
bool keepalive(SOCKET_T hSo, bool keepalive) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return false;
  int _keepalive = keepalive ? 1 : 0;
  return setsockopt(hSo, SOL_SOCKET, SO_KEEPALIVE, (const char *)&_keepalive,
                    sizeof(int));
}

bool __setINetNonblocking(SOCKET_T hSo, bool nonblocking) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return false;
  unsigned long _u = (nonblocking ? 1 : 0);
  bool _r = SO_NETWORK_IOCTL_CALL(hSo, FIONBIO, &_u) >= 0;
  if (!_r) {
    log::warning << "Warning: cannot change the nonblocking flag of socket."
                 << std::endl;
  }
  return _r;
}

bool __setUDSNonblocking(SOCKET_T hSo, bool nonblocking) {
  // Set NonBlocking
  int _f = fcntl(hSo, F_GETFL, NULL);
  if (_f < 0) {
    log::warning << "Warning: cannot get UDS socket file info." << std::endl;
    return false;
  }
  if (nonblocking) {
    _f |= O_NONBLOCK;
  } else {
    _f &= (~O_NONBLOCK);
  }
  if (fcntl(hSo, F_SETFL, _f) < 0) {
    log::warning
      << "Warning: failed to change the nonblocking flag of UDS socket."
      << std::endl;
    return false;
  }
  return true;
}

// Make a socket to be nonblocking
bool nonblocking(SOCKET_T hSo, bool nonblocking) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return false;
  uint32_t _st = socktype(hSo);
  if (_st == kSocketTypeUnixDomain)
    return __setUDSNonblocking(hSo, nonblocking);
  else
    return __setINetNonblocking(hSo, nonblocking);
}
bool nodelay(SOCKET_T hSo, bool nodelay) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return false;
  if (socktype(hSo) != kSocketTypeTCP)
    return true;
  // Try to set as tcp-nodelay
  int _flag = nodelay ? 1 : 0;
  bool _r = (setsockopt(hSo, IPPROTO_TCP, TCP_NODELAY, (const char *)&_flag,
                        sizeof(int)) != -1);
  if (!_r) {
    log::warning << "Warning: cannot set the socket to be tcp-nodelay"
                 << std::endl;
  }
  return _r;
}
// Set the socket's buffer size
bool buffersize(SOCKET_T hSo, uint32_t rmem, uint32_t wmem) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return false;

  if (rmem != 0) {
    setsockopt(hSo, SOL_SOCKET, SO_RCVBUF, (char *)&rmem, sizeof(rmem));
  }
  if (wmem != 0) {
    setsockopt(hSo, SOL_SOCKET, SO_SNDBUF, (char *)&wmem, sizeof(wmem));
  }
  return true;
}
// Check if a socket has data to read
bool has_data_pending(SOCKET_T hSo) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return false;
  fd_set _fs;
  FD_ZERO(&_fs);
  FD_SET(hSo, &_fs);

  int _ret = 0;
  struct timeval _tv = {0, 0};

  do {
    FD_ZERO(&_fs);
    FD_SET(hSo, &_fs);
    _ret = ::select(hSo + 1, &_fs, NULL, NULL, &_tv);
  } while (_ret < 0 && errno == EINTR);
  // Try to peek
  if (_ret <= 0) return false;
  _ret = recv(hSo, NULL, 0, MSG_PEEK);
  // If _ret < 0, means recv an error signal
  return (_ret > 0);
}

// Check if a socket has buffer to write
bool has_buffer_outgoing(SOCKET_T hSo) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return false;
  fd_set _fs;

  int _ret = 0;
  struct timeval _tv = {0, 0};
  do {
    FD_ZERO(&_fs);
    FD_SET(hSo, &_fs);
    _ret = ::select(hSo + 1, NULL, &_fs, NULL, &_tv);
  } while (_ret < 0 && errno == EINTR);
  return (_ret > 0);
}

// Create Socket Handler
SOCKET_T create_socket(SocketType sock_type) {
  SOCKET_T _so = INVALIDATE_SOCKET;
  if (sock_type == kSocketTypeTCP) {
    _so = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  } else if (sock_type == kSocketTypeUDP) {
    _so = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  } else if (sock_type == kSocketTypeUnixDomain) {
    _so = ::socket(AF_UNIX, SOCK_STREAM, 0);
  } else {
    log::error << "Error: Cannot create new socket with unknow type <"
               << sock_type << ">" << std::endl;
    return _so;
  }

  if (SOCKET_NOT_VALIDATE(_so)) {
    log::error << "Error: Failed to create socket with type <" << sock_type
               << ">" << std::endl;
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
size_t read(SOCKET_T hSo, char *buffer, size_t length,
            std::function<int(SOCKET_T, char *, size_t)> f) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return 0;
  size_t BUF_SIZE = length;
  size_t _received = 0;
  size_t _leftspace = BUF_SIZE;

  do {
    int _retCode = f(hSo, buffer + _received, _leftspace);
    if (_retCode < 0) {
      if (errno == EINTR)
        continue; // Signal 7, retry
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No more incoming data in this socket's cache
        return _received;
      }
      log::error << "Error: failed to receive data on socket(" << hSo << "), "
                 << ::strerror(errno) << std::endl;
      return 0;
    } else if (_retCode == 0) {
      return 0;
    } else {
      _received += _retCode;
      _leftspace -= _retCode;
    }
  } while (_leftspace > 0);
  return _received;
}
bool read(std::string& buffer, 
  SOCKET_T hSo, std::function<int(SOCKET_T, char *, size_t)> f,
                 uint32_t max_buffer_size) {
  if (SOCKET_NOT_VALIDATE(hSo))
    return false;
  bool _hasLimit = (max_buffer_size != 0);
  size_t BUF_SIZE = (_hasLimit ? max_buffer_size : 1024); // 1KB
  std::string _buffer(BUF_SIZE, '\0');
  //_buffer.resize(BUF_SIZE);
  size_t _received = 0;
  size_t _leftspace = BUF_SIZE;

  do {
    int _retCode = f(hSo, &_buffer[0] + _received, _leftspace);
    if (_retCode < 0) {
      if (errno == EINTR)
        continue; // signal 7, retry
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No more data on a non-blocking socket
        _buffer.resize(_received);
        break;
      }
      // Other error
      _buffer.resize(0);
      log::error << "Error: Failed to receive data on socket(" << hSo << ", "
                 << ::strerror(errno) << std::endl;
      return false;
    } else if (_retCode == 0) {
      // Peer Close
      _buffer.resize(0);
      return false;
    } else {
      _received += _retCode;
      _leftspace -= _retCode;
      if (_leftspace > 0) {
        // Unfull
        _buffer.resize(_received);
        break;
      } else {
        // If has limit, and left space is zero,
        // which means the data size has reach
        // the limit and we should not read any more
        // data from the socket this time.
        if (_hasLimit)
          break;

        // Otherwise, try to double the buffer
        // The buffer is full, try to double the buffer and try again
        if (BUF_SIZE * 2 <= _buffer.max_size()) {
          BUF_SIZE *= 2;
        } else if (BUF_SIZE < _buffer.max_size()) {
          BUF_SIZE = _buffer.max_size();
        } else {
          break; // direct return, wait for next read.
        }
        // Resize the buffer and try to read again
        _leftspace = BUF_SIZE - _received;
        _buffer.resize(BUF_SIZE);
      }
    }
  } while (true);
  buffer = std::move(_buffer);
  return true;
}
// Write Data to a socket
int write(SOCKET_T hSo, const char *data, size_t data_lenth,
          std::function<int(SOCKET_T, const char *, size_t)> f) {
  size_t _sent = 0;
  while (_sent < data_lenth) {
    int _ret = f(hSo, data + _sent, data_lenth - _sent);
    if (_ret < 0) {
      if (ENOBUFS == errno || EAGAIN == errno || EWOULDBLOCK == errno) {
        // No buf
        break;
      } else {
        log::warning << "Failed to send data on socket(" << hSo << "), "
                     << ::strerror(errno) << std::endl;
        return _ret;
      }
    } else if (_ret == 0) {
      break;
    } else {
      _sent += _ret;
    }
  }
  return (int)_sent;
}
} // namespace net_utils
} // namespace peco

// Push Chen
