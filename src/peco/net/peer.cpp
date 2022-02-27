/*
    peer.cpp
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

#include "peco/net/peer.h"

namespace peco {

// Parse the peer string in certen format
void peer_t::__parse_peer(const std::string &fmt) {
  size_t _pos = fmt.rfind(":");
  if (_pos == std::string::npos) {
    ip_ = "0.0.0.0";
    port_ = stoi(fmt, nullptr, 10);
    format_ = "0.0.0.0:" + fmt;
  } else {
    ip_ = fmt.substr(0, _pos);
    port_ = stoi(fmt.substr(_pos + 1), nullptr, 10);
    format_ = fmt;
  }
}

// Constructure
peer_t::peer_t()
  : ip(ip_), port(port_), ip_(0), port_(0), format_("0.0.0.0:0") {}

// Copy
peer_t::peer_t(const peer_t &rhs)
  : ip(ip_), port(port_), ip_(rhs.ip_), port_(rhs.port_), format_(rhs.format_) {
}
peer_t::peer_t(peer_t &&lhs)
  : ip(ip_), port(port_), 
    ip_(std::move(lhs.ip_)), port_(std::move(lhs.port_)), format_(std::move(lhs.format_)) {}
peer_t &peer_t::operator=(const peer_t &rhs) {
  if (this == &rhs)
    return *this;
  ip_ = rhs.ip_;
  port_ = rhs.port_;
  format_ = rhs.format_;
  return *this;
}
peer_t &peer_t::operator=(peer_t &&lhs) {
  if (this == &lhs)
    return *this;
  ip_ = std::move(lhs.ip_);
  port_ = std::move(lhs.port_);
  format_ = std::move(lhs.format_);
  return *this;
}

// Setting
peer_t::peer_t(const ip_t &ip, uint16_t p)
  : ip(ip_), port(port_), ip_(ip), port_(p) {
  format_ = (const std::string &)ip_;
  format_ += ":";
  format_ += std::to_string(port_);
}
peer_t::peer_t(const peer_t::peer_arg_t &ipt)
  : ip(ip_), port(port_), ip_(ipt.first), port_(ipt.second) {
  format_ = (const std::string &)ip_;
  format_ += ":";
  format_ += std::to_string(port_);
}

peer_t::peer_t(const std::string &fmt) : ip(ip_), port(port_) {
  this->__parse_peer(fmt);
}
peer_t::peer_t(const struct sockaddr_in addr)
  : ip(ip_), port(port_), ip_(addr.sin_addr.s_addr), port_(ntohs(addr.sin_port)) {
  format_ = (const std::string &)ip_;
  format_ += ":";
  format_ += std::to_string(port_);
}

peer_t &peer_t::operator=(const peer_t::peer_arg_t &ipt) {
  ip_ = ipt.first;
  port_ = ipt.second;
  format_ = (const std::string &)ip_;
  format_ += ":";
  format_ += std::to_string(port_);
  return *this;
}
peer_t &peer_t::operator=(const std::string &fmt) {
  this->__parse_peer(fmt);
  return *this;
}
peer_t &peer_t::operator=(const struct sockaddr_in addr) {
  ip_ = addr.sin_addr.s_addr;
  port_ = ntohs(addr.sin_port);
  format_ = (const std::string &)ip_;
  format_ += ":";
  format_ += std::to_string(port_);
  return *this;
}

// Operators
peer_t::operator bool() const { return port_ > 0 && port_ <= 65535; }

// Conversitions
peer_t::operator struct sockaddr_in() const {
  struct sockaddr_in _addr;
  _addr.sin_family = AF_INET;
  _addr.sin_addr.s_addr = (uint32_t)ip_;
  _addr.sin_port = htons(port_);
  return _addr;
}
const std::string &peer_t::str() const { return format_; }
const char *peer_t::c_str() const { return format_.c_str(); }
size_t peer_t::size() const { return format_.size(); }

// Get an empty peer info
const peer_t peer_t::nan;

// Output the peer info
std::ostream &operator<<(std::ostream &os, const peer_t &peer) {
  os << peer.str();
  return os;
}

} // namespace peco
