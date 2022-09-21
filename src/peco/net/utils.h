/*
    utils.h
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

#pragma once

#ifndef PECO_NET_UTILS_H__
#define PECO_NET_UTILS_H__

#include "peco/pecostd.h"
#include "peco/net/peer.h"

namespace peco {

#if PECO_TARGET_WIN
typedef int socklen_t;
typedef SOCKET SOCKET_T;
#else
typedef long SOCKET_T;
#endif

#ifndef INVALIDATE_SOCKET
#define INVALIDATE_SOCKET           ((long)((long)0 - (long)1))
#endif

#define SOCKET_NOT_VALIDATE( so )   ((so) == INVALIDATE_SOCKET)
#define SOCKET_VALIDATE( so )       ((so) != INVALIDATE_SOCKET)

#ifndef CO_MAX_SO_EVENTS
#define CO_MAX_SO_EVENTS 25600
#endif

enum SocketType {
  /**
   * @brief TCP Socket
  */
  kSocketTypeTCP          = 10000,
  /**
   * @brief UDP Socket
  */
  kSocketTypeUDP          = 10001,
  /**
   * @brief Unix Domain/Socket File
  */
  kSocketTypeUnixDomain   = 10002
};

namespace net_utils {
  /**
   * @brief Net and host order conversition
  */
  uint16_t h2n(uint16_t v);
  uint32_t h2n(uint32_t v);
  uint64_t h2n(uint64_t v);
  float h2n(float v);
  double h2n(double v);

  /**
   * @brief N2H is the same logic of h2n
  */
  uint16_t n2h(uint16_t v);
  uint32_t n2h(uint32_t v);
  uint64_t n2h(uint64_t v);
  float n2h(float v);
  double n2h(double v);

  /**
   * @brief Convert host endian to little endian
  */
  uint16_t h2le(uint16_t v);
  uint32_t h2le(uint32_t v);
  uint64_t h2le(uint64_t v);
  float h2le(float v);
  double h2le(double v);

  /**
   * @brief Little Endian to Host
  */
  uint16_t le2h(uint16_t v);
  uint32_t le2h(uint32_t v);
  uint64_t le2h(uint64_t v);
  float le2h(float v);
  double le2h(double v);

  /**
   * @brief Convert host endian to big endian
  */
  uint16_t h2be(uint16_t v);
  uint32_t h2be(uint32_t v);
  uint64_t h2be(uint64_t v);
  float h2be(float v);
  double h2be(double v);

  /**
   * @brief Big Endian to Host
  */
  uint16_t be2h(uint16_t v);
  uint32_t be2h(uint32_t v);
  uint64_t be2h(uint64_t v);
  float be2h(float v);
  double be2h(double v);

  /**
   * @brief Get localhost's computer name on LAN
  */
  const std::string& hostname();

  /**
   * @brief Get peer info from a socket
  */
  peer_t socket_peerinfo(const SOCKET_T hSo);

  /**
   * @brief Get local socket's port
  */
  uint16_t localport(const SOCKET_T hSo);

  /**
   * @brief Get the socket type
  */
  SocketType socktype(SOCKET_T hSo);

  /**
   * @brief Set the linger time for a socketI strong suggest 
   * not to change this value unless you know what you are doing
  */
  bool lingertime(SOCKET_T hSo, bool onoff = true, unsigned timeout = 1);

  /**
   * @brief Set Current socket reusable or not
  */
  bool reusable(SOCKET_T hSo, bool reusable = true);

  /**
   * @brief Make current socket keep alive
  */
  bool keepalive(SOCKET_T hSo, bool keepalive = true);

  /**
   * @brief Make a socket to be nonblocking
  */
  bool nonblocking(SOCKET_T hSo, bool nonblocking = true);

  /**
   * @brief Set the socket to be no-delay
  */
  bool nodelay(SOCKET_T hSo, bool nodelay = true);

  /**
   * @brief Set the socket's buffer size
  */
  bool buffersize(SOCKET_T hSo, uint32_t rmem = 0, uint32_t wmem = 0);

  /**
   * @brief Check if a socket has data to read
  */
  bool has_data_pending(SOCKET_T hSo);

  /**
   * @brief Check if a socket has buffer to write
  */
  bool has_buffer_outgoing(SOCKET_T hSo);

  /**
   * @brief Create Socket Handler
  */
  SOCKET_T create_socket(SocketType sock_type);

  /**
   * @brief Read Data From a socket
  */
  size_t read(
      SOCKET_T hSo,
      char * buffer,
      size_t length,
      std::function<int (SOCKET_T, char *, size_t)> f);
  bool read(
      std::string& buffer,
      SOCKET_T hSo, 
      std::function<int (SOCKET_T, char *, size_t)> f, 
      uint32_t max_buffer_size = 0);

  /**
   * @brief Write Data to a socket
  */
  int write(
      SOCKET_T hSo, 
      const char* data, 
      size_t data_lenth,
      std::function<int (SOCKET_T, const char*, size_t)> f);

} // namespace net_utils
} // namespace peco

#endif

// Push Chen
