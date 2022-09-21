/*
    ip.cpp
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

#include "peco/net/ip.h"

#if PECO_TARGET_WIN
#include <WS2tcpip.h>
#endif

namespace peco {
// Internal Conversition Methods
uint32_t __ipstring2inaddr(const std::string& ipstring) {
#if PECO_TARGET_WIN
  IN_ADDR addr;
  auto ret = inet_pton(AF_INET, ipstring.c_str(), &addr);
  if (ret > 0) {
    return addr.s_addr;
  }
  return (uint32_t)-1;
#else
  return inet_addr(ipstring.c_str());
#endif
}

void __inaddr2ipstring(uint32_t inaddr, std::string& ipstring) {
  ipstring.resize(16);
#if PECO_TARGET_WIN
  int _l = sprintf_s( &ipstring[0], 16, "%u.%u.%u.%u",
#else
  int _l = sprintf( &ipstring[0], "%u.%u.%u.%u",
#endif
    (inaddr >> (0 * 8)) & 0x00FF,
    (inaddr >> (1 * 8)) & 0x00FF,
    (inaddr >> (2 * 8)) & 0x00FF,
    (inaddr >> (3 * 8)) & 0x00FF 
  );
  ipstring.resize(_l);
}

// Basic Empty Constructure
ip_t::ip_t() : ip_("255.255.255.255") {}

// Copy & Move Operators
ip_t::ip_t(const ip_t& rhs) : ip_(rhs.ip_) { }
ip_t::ip_t(ip_t&& lhs) : ip_(move(lhs.ip_)) { }
ip_t& ip_t::operator = (const ip_t& rhs) {
  if ( this == &rhs ) return *this;
  ip_ = rhs.ip_;
  return *this;
}
ip_t& ip_t::operator = (ip_t&& lhs) {
  if ( this == &lhs ) return *this;
  ip_ = move(lhs.ip_);
  return *this;
}

// Conversition
ip_t::ip_t(const std::string &ipaddr) { 
  __inaddr2ipstring(__ipstring2inaddr(ipaddr), ip_);
}
ip_t::ip_t(uint32_t ipaddr) {
  __inaddr2ipstring(ipaddr, ip_);
}
ip_t& ip_t::operator = (const std::string &ipaddr) {
  __inaddr2ipstring(__ipstring2inaddr(ipaddr), ip_);
  return *this;
}
ip_t& ip_t::operator = (uint32_t ipaddr) {
  __inaddr2ipstring(ipaddr, ip_);
  return *this;
}

ip_t::operator bool const() {
  return this->is_valid();
}
bool ip_t::is_valid() const {
  return (uint32_t)-1 != this->operator uint32_t();
}
// Conversition operators
ip_t::operator uint32_t() const { return __ipstring2inaddr(ip_); }
const std::string& ip_t::str() const { return ip_; }
const char* ip_t::c_str() const { return ip_.c_str(); }

// string length of the ip address
size_t ip_t::size() const { return ip_.size(); }

// Operators
bool ip_t::operator == (const ip_t& rhs) const { return ip_ == rhs.ip_; }
bool ip_t::operator != (const ip_t& rhs) const { return ip_ != rhs.ip_; }
bool ip_t::operator <(const ip_t& rhs) const { 
  return ntohl((uint32_t)(*this)) < ntohl((uint32_t)rhs); 
}
bool ip_t::operator >(const ip_t& rhs) const { 
  return ntohl((uint32_t)(*this)) > ntohl((uint32_t)rhs); 
}
bool ip_t::operator <=(const ip_t& rhs) const { 
  return ntohl((uint32_t)(*this)) <= ntohl((uint32_t)rhs); 
}
bool ip_t::operator >=(const ip_t& rhs) const { 
  return ntohl((uint32_t)(*this)) >= ntohl((uint32_t)rhs); 
}

// Output
std::ostream & operator << (std::ostream &os, const ip_t & ip) {
  os << ip.str(); return os;
}
} // namespace peco

// Push Chen
