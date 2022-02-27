/*
    connect.h
    libpeco
    2022-02-23
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

#ifndef PECO_NET_CONNECT_H__
#define PECO_NET_CONNECT_H__

#include "peco/net/utils.h"
#include "peco/task.h"

namespace peco {

/**
 * @brief Connect a tcp socket
*/
class tcp_connect {
public:
  tcp_connect(const peer_t& dest_addr, duration_t timeout);
  tcp_connect(const std::string& addr, duration_t timeout);
  tcp_connect(const tcp_connect& other);
  tcp_connect(tcp_connect&& other);
  tcp_connect& operator =(const tcp_connect& other);
  tcp_connect& operator =(tcp_connect&& other);

  /**
   * @brief Connect the given tcp socket to dest
  */
  bool operator()(SOCKET_T fd);
protected:
  peer_t dest_addr_;
  duration_t timeout_;
};

/**
 * @brief Connect a udp socket
*/
class udp_connect {
public:
  udp_connect(const peer_t& dest_addr);
  udp_connect(const std::string& addr);
  udp_connect(const udp_connect& other);
  udp_connect(udp_connect&& other);
  udp_connect& operator =(const udp_connect& other);
  udp_connect& operator =(udp_connect&& other);

  /**
   * @brief Connect the given udp socket to dest
  */
  bool operator()(SOCKET_T fd);
protected:
  peer_t dest_addr_;
};

/**
 * @brief Connect to a unix domain socket
*/
class uds_connect {
public:
  uds_connect(const std::string& addr);
  uds_connect(const uds_connect& other);
  uds_connect(uds_connect&& other);
  uds_connect& operator =(const uds_connect& other);
  uds_connect& operator =(uds_connect&& other);

  /**
   * @brief Connect the given udp socket to dest
  */
  bool operator()(SOCKET_T fd);

protected:
  std::string dest_addr_;
};

} // namespace peco

#endif

// Push Chen
