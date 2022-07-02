/*
    tcp.cpp
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

#include "peco/net/tcp.h"
#include "peco/utils/logs.h"
#include "peco/net/bind.h"
#include "peco/net/connect.h"

namespace peco {

tcp_connector::~tcp_connector() {}
/**
 * @brief Factory
*/
std::shared_ptr<tcp_connector> tcp_connector::create() {
  return std::shared_ptr<tcp_connector>(new tcp_connector);
}
/**
 * @brief Create a tcp connector with a given fd and mark it as connected
*/
std::shared_ptr<tcp_connector> tcp_connector::create(SOCKET_T fd) {
  if (SOCKET_NOT_VALIDATE(fd)) {
    return std::shared_ptr<tcp_connector>(new tcp_connector);
  } else {
    return std::shared_ptr<tcp_connector>(new tcp_connector(fd));
  }
}
/**
 * @brief Bind address to this socket, different socket type should use
 * different bind slot
*/
bool tcp_connector::bind(const peer_t& local_addr) {
  if (connector_adapter::is_connected()) return false;
  return inet_adapter::bind(peer_bind(local_addr));
}
bool tcp_connector::bind(const std::string& local_addr) {
  if (connector_adapter::is_connected()) return false;
  return inet_adapter::bind(peer_bind(local_addr));
}

/**
 * @brief Connect to peer
*/
bool tcp_connector::connect(const peer_t& dest_addr, duration_t timeout) {
  return connector_adapter::connect(tcp_connect(dest_addr, timeout));
}
bool tcp_connector::connect(const std::string& dest_addr, duration_t timeout) {
  return connector_adapter::connect(tcp_connect(dest_addr, timeout));
}

/**
 * @brief Get local port
*/
uint16_t tcp_connector::localport() const {
  return net_utils::localport(fd_);
}
/**
 * @brief Get remote address
*/
peer_t tcp_connector::peer_info() const {
  return net_utils::socket_peerinfo(fd_);
}

/**
 * @brief Create a tcp socket
*/
tcp_connector::tcp_connector() {
  inet_adapter::create_socket(kSocketTypeTCP);
  net_utils::nonblocking(fd_, true);
  net_utils::nodelay(fd_, true);
  net_utils::reusable(fd_, true);
  net_utils::keepalive(fd_, true);
}
tcp_connector::tcp_connector(SOCKET_T fd)
  : connector_adapter(fd, true) {
  net_utils::nonblocking(fd_, true);
  net_utils::nodelay(fd_, true);
  net_utils::reusable(fd_, true);
  net_utils::keepalive(fd_, true);
}


tcp_listener::~tcp_listener() { }
/**
 * @brief Factory
*/
std::shared_ptr<tcp_listener> tcp_listener::create() {
  return std::shared_ptr<tcp_listener>(new tcp_listener);
}

/**
 * @brief Bind address to this socket, different socket type should use
 * different bind slot
*/
bool tcp_listener::bind(const peer_t& local_addr) {
  return inet_adapter::bind(peer_bind(local_addr));
}
bool tcp_listener::bind(const std::string& local_addr) {
  return inet_adapter::bind(peer_bind(local_addr));
}

/**
 * @brief Listen on the binded socket
*/
bool tcp_listener::listen(listener_adapter::slot_accept_t accept_slot) {
  if ( -1 == ::listen(fd_, CO_MAX_SO_EVENTS) ) {
    log::error << "failed to listen on socket(" << fd_ << "), error: " << ::strerror(errno) << std::endl;
    return false;
  }

  auto self = this->shared_from_this();
  loop::shared()->run([=]() {
    std::string task_name = "tcp_listen:" + std::to_string(net_utils::localport(fd_));
    task::this_task().set_name(task_name.c_str());
    while (true) {
      task::this_task().wait_fd_for_event(self->fd_, kEventTypeRead, PECO_TIME_S(1800));
      auto sig = task::this_task().signal();
      if (sig == kWaitingSignalBroken) {
        // task cancelled
        return;
      }
      if (sig == kWaitingSignalNothing) continue;
      while(true) {
        struct sockaddr in_addr;
        socklen_t in_len = 0;
        memset(&in_addr, 0, sizeof(in_len));
        SOCKET_T in_fd = accept(self->fd_, &in_addr, &in_len);
        if (in_fd == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) break;
          // On error
          return;
        } else {
          loop::shared()->run([=]() {
            accept_slot(in_fd, net_utils::socket_peerinfo(in_fd));
          });
        }
      }
    }
  }, PECO_CODE_LOCATION);
  return true;
}
/**
 * @brief Listen on the binded socket
*/
bool tcp_listener::listen(std::function<void(std::shared_ptr<tcp_connector>)> accept_slot) {
  return this->listen([=](SOCKET_T fd, const peer_t& remote_addr) {
    accept_slot(tcp_connector::create(fd));
  });
}

/**
 * @brief Not allowed
*/
tcp_listener::tcp_listener() {
  inet_adapter::create_socket(kSocketTypeTCP);
  net_utils::nonblocking(fd_, true);
  net_utils::nodelay(fd_, true);
  net_utils::reusable(fd_, true);
  net_utils::keepalive(fd_, true);
}


} // namespace 

// Push Chen

