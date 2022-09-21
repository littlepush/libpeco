/*
    ip.h
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

#ifndef PECO_NET_IP_H__
#define PECO_NET_IP_H__

#include "peco/pecostd.h"

#if PECO_TARGET_WIN

#include <winsock2.h>
#define SO_NETWORK_NOSIGNAL           0
#define SO_NETWORK_IOCTL_CALL         ioctlsocket
#define SO_NETWORK_CLOSESOCK          closesocket

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#if PECO_TARGET_OPENOS
#include <linux/netfilter_ipv4.h>
#endif

#define SO_NETWORK_NOSIGNAL           MSG_NOSIGNAL
#define SO_NETWORK_IOCTL_CALL         ioctl
#define SO_NETWORK_CLOSESOCK          ::close

#endif

namespace peco {

/*!
The IP object, compatible with std::string and uint32_t
This is a ipv4 ip address class.
*/
class ip_t {
public:
  // Basic Empty Constructure
  ip_t();

  // Copy & Move Operators
  ip_t(const ip_t& rhs);
  ip_t(ip_t&& lhs);
  ip_t& operator = (const ip_t& rhs);
  ip_t& operator = (ip_t&& lhs);

  // Conversition
  explicit ip_t(const std::string &ipaddr);
  explicit ip_t(uint32_t ipaddr);
  ip_t& operator = (const std::string &ipaddr);
  ip_t& operator = (uint32_t ipaddr);

  // Return if the IP is validate
  explicit operator bool const();
  bool is_valid() const;

  // Conversition operators
  explicit operator uint32_t() const;
  const std::string& str() const;
  const char *c_str() const;

  // string length of the ip address
  size_t size() const;

  // Operators
  bool operator == (const ip_t& rhs) const;
  bool operator != (const ip_t& rhs) const;
  bool operator <(const ip_t& rhs) const;
  bool operator >(const ip_t& rhs) const;
  bool operator <=(const ip_t& rhs) const;
  bool operator >=(const ip_t& rhs) const;

protected:
  std::string          ip_;
};

// Output
std::ostream & operator << (std::ostream &os, const ip_t & ip);

} // namespace peco

#endif

// Push Chen
