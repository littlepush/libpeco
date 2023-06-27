/*
    peer.h
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

#ifndef PECO_NET_PEER_H__
#define PECO_NET_PEER_H__

#include "pecostd.h"
#include "net/ip.h"

namespace peco {
/*!
Peer Info, contains an IP address and a port number.
should be output in the following format: 0.0.0.0:0
*/
class peer_t {
public:
  typedef std::pair<ip_t, uint16_t>   peer_arg_t;
protected:
  // Parse the peer string in certen format
  void __parse_peer(const std::string& fmt);
public:
  const ip_t &        ip;
  const uint16_t &    port;

  // Constructure
  peer_t();

  // Copy
  peer_t(const peer_t& rhs);
  peer_t(peer_t&& lhs);
  peer_t& operator = (const peer_t& rhs);
  peer_t& operator = (peer_t&& lhs);

  // Setting
  peer_t(const ip_t& ip, uint16_t p);
  explicit peer_t(const peer_arg_t& ipt);
  explicit peer_t(const std::string& fmt);
  explicit peer_t(const struct sockaddr_in addr);

  peer_t& operator = (const peer_arg_t& ipt);
  peer_t& operator = (const std::string& fmt);
  peer_t& operator = (const struct sockaddr_in addr);

  // Operators
  operator bool() const;

  // Conversitions
  operator struct sockaddr_in() const;
  const std::string& str() const;
  const char *c_str() const;
  size_t size() const;

  // Get an empty peer info
  static const peer_t nan;
private:
  ip_t            ip_;
  uint16_t        port_;
  std::string     format_;
};

// Output the peer info
std::ostream & operator << (std::ostream &os, const peer_t &peer);
} // namespace peco

#endif

// Push Chen
