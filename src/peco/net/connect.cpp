/*
    connect.cpp
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

#include "peco/net/connect.h"
#include "peco/utils/stringutil.h"
#include "peco/utils/logs.h"

#if !PECO_TARGET_WIN
#include <fcntl.h>
#include <sys/un.h>
#else
#include <afunix.h>
#endif

namespace peco {
/**
 * @brief Connect a tcp socket
*/
tcp_connect::tcp_connect(const peer_t& dest_addr, duration_t timeout)
  : dest_addr_(dest_addr), timeout_(timeout) {}
tcp_connect::tcp_connect(const std::string& addr, duration_t timeout)
  : dest_addr_(addr), timeout_(timeout) {}

tcp_connect::tcp_connect(const tcp_connect& other)
  : dest_addr_(other.dest_addr_), timeout_(other.timeout_) { }
tcp_connect::tcp_connect(tcp_connect&& other)
  : dest_addr_(std::move(other.dest_addr_)), timeout_(std::move(other.timeout_)) { }
tcp_connect& tcp_connect::operator =(const tcp_connect& other) {
  if (this == &other) return *this;
  dest_addr_ = other.dest_addr_;
  timeout_ = other.timeout_;
  return *this;
}
tcp_connect& tcp_connect::operator =(tcp_connect&& other) {
  if (this == &other) return *this;
  dest_addr_ = std::move(other.dest_addr_);
  timeout_ = std::move(other.timeout_);
  return *this;
}

/**
 * @brief Connect the given tcp socket to dest
*/
bool tcp_connect::operator()(SOCKET_T fd) {
  if (!dest_addr_) return false;

  struct sockaddr_in sock_addr = dest_addr_;
  if ( ::connect(fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1 ) {
    int err = 0, len = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, (socklen_t *)&len);
    if (err != 0) {
      log::error << "Error: Failed to connect to " << dest_addr_ << 
        " on tcp socket(" << fd << "), " << err << ":" << get_sys_error(err) << std::endl;
      return false;
    } else {
      task::this_task().wait_fd_for_event(fd, kEventTypeWrite, timeout_);
      auto sig = task::this_task().signal();
      if (sig == kWaitingSignalReceived) {
        return true;
      } else if (sig == kWaitingSignalNothing) {
        log::warning << "Warning: timedout on connection to " << dest_addr_
          << " on tcp socket(" << fd << ")." << std::endl;
        return false;
      } else {
        if (err == 0) {
          log::warning << "Warning: Task has been cancelled, tcp socket(" << fd << ") is "
            << "not allowed to continue process connecting event" << std::endl;
        } else {
          log::error << "Error: Failed to connect to " << dest_addr_ <<
            " on tcp socket(" << fd << "), err: " << err << ", " << get_sys_error(err) <<
            std::endl;
        }
        return false;
      }
    } 
  } else {
    return true;
  }
  // never go here
  return false;
}

/**
 * @brief Connect a udp socket
*/
udp_connect::udp_connect(const peer_t& dest_addr)
  : dest_addr_(dest_addr) {}
udp_connect::udp_connect(const std::string& addr)
  : dest_addr_(addr) {}
udp_connect::udp_connect(const udp_connect& other)
  : dest_addr_(other.dest_addr_) {}
udp_connect::udp_connect(udp_connect&& other)
  : dest_addr_(std::move(other.dest_addr_)) {}
udp_connect& udp_connect::operator =(const udp_connect& other) {
  if (this == &other) return *this;
  dest_addr_ = other.dest_addr_;
  return *this;
}
udp_connect& udp_connect::operator =(udp_connect&& other) {
  if (this == &other) return *this;
  dest_addr_ = std::move(other.dest_addr_);
  return *this;
}

/**
 * @brief Connect the given udp socket to dest
*/
bool udp_connect::operator()(SOCKET_T fd) {
  if (!dest_addr_) return false;
  struct sockaddr_in sock_addr = dest_addr_;
  if ( ::connect(fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1 ) {
    log::error << "Error: Failed to connect to " << dest_addr_ <<
      " on udp socket(" << fd << "), " << get_sys_error(errno) <<
      std::endl;
    return false;
  }
  return true;
}

/**
 * @brief Connect a uds socket
*/
uds_connect::uds_connect(const std::string& addr)
  : dest_addr_(addr) {}
uds_connect::uds_connect(const uds_connect& other)
  : dest_addr_(other.dest_addr_) {}
uds_connect::uds_connect(uds_connect&& other)
  : dest_addr_(std::move(other.dest_addr_)) {}
uds_connect& uds_connect::operator =(const uds_connect& other) {
  if (this == &other) return *this;
  dest_addr_ = other.dest_addr_;
  return *this;
}
uds_connect& uds_connect::operator =(uds_connect&& other) {
  if (this == &other) return *this;
  dest_addr_ = std::move(other.dest_addr_);
  return *this;
}

/**
 * @brief Connect the given udp socket to dest
*/
bool uds_connect::operator()(SOCKET_T fd) {
  if (dest_addr_.size() == 0) return false;

  struct sockaddr_un  peeraddr;
  memset(&peeraddr, 0, sizeof(peeraddr));
  memcpy(peeraddr.sun_path, dest_addr_.c_str(), dest_addr_.size());
  peeraddr.sun_family = AF_UNIX;
#if PECO_TARGET_APPLE
  int addrlen = static_cast<int>(dest_addr_.size() + sizeof(peeraddr.sun_family) + sizeof(peeraddr.sun_len));
  peeraddr.sun_len = sizeof(peeraddr);
#else
  int addrlen = static_cast<int>(dest_addr_.size() + sizeof(peeraddr.sun_family));
#endif

  if ( -1 == ::connect(fd, (struct sockaddr *)&peeraddr, addrlen) ) {
    log::error << "Error: Failed to connect to " << dest_addr_ << 
      " on uds socket(" << fd << "), " << get_sys_error( errno ) <<
      std::endl;
    return false;
  }
  return true;
}

} // namespace peco

// Push Chen
