/*
    uds.cpp
    libpeco
    2022-02-24
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

#include "peco/net/uds.h"
#include "peco/net/bind.h"
#include "peco/net/connect.h"
#include "peco/utils/logs.h"

namespace peco {

uds_connector::~uds_connector() { }

/**
 * @brief Factory
*/
std::shared_ptr<uds_connector> uds_connector::create() {
  return std::shared_ptr<uds_connector>(new uds_connector);
}

/**
 * @brief Create a tcp connector with a given fd and mark it as connected
*/
std::shared_ptr<uds_connector> uds_connector::create(SOCKET_T fd) {
  if (SOCKET_NOT_VALIDATE(fd)) {
    return uds_connector::create();
  } else {
    return std::shared_ptr<uds_connector>(new uds_connector(fd));
  }
}

/**
 * @brief Bind address to this socket, different socket type should use
 * different bind slot
*/
bool uds_connector::bind(const std::string& local_addr) {
  return false;
}

/**
 * @brief Connect to peer
*/
bool uds_connector::connect(const std::string& dest_addr) {
  return false;
}

/**
 * @brief Create a uds socket
*/
uds_connector::uds_connector() {
  inet_adapter::create_socket(kSocketTypeUnixDomain);
  net_utils::nonblocking(fd_, true);
  net_utils::nodelay(fd_, true);
  net_utils::reusable(fd_, true);
  net_utils::keepalive(fd_, true);
}
uds_connector::uds_connector(SOCKET_T fd)
  : connector_adapter(fd, true) {
  net_utils::nonblocking(fd_, true);
  net_utils::nodelay(fd_, true);
  net_utils::reusable(fd_, true);
  net_utils::keepalive(fd_, true);
}

uds_listener::~uds_listener() { }
/**
 * @brief Factory
*/
std::shared_ptr<uds_listener> uds_listener::create() {
  return std::shared_ptr<uds_listener>(new uds_listener);
}

/**
 * @brief Bind address to this socket, different socket type should use
 * different bind slot
*/
bool uds_listener::bind(const std::string& local_addr) {
  local_path_ = local_addr;
  return inet_adapter::bind(path_bind(local_addr));
}

/**
 * @brief Listen on the binded socket
*/
bool uds_listener::listen(listener_adapter::slot_accept_t accept_slot) {
  if ( -1 == ::listen(fd_, CO_MAX_SO_EVENTS) ) {
    log::error << "failed to listen on socket(" << fd_ << "), error: " << ::strerror(errno) << std::endl;
    return false;
  }

  auto self = this->shared_from_this();
  loop::current.run([=]() {
    std::string task_name = "uds_listen:" + local_path_;
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
          loop::current.run([=]() {
            accept_slot(in_fd, peer_t::nan);
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
bool uds_listener::listen(std::function<void(std::shared_ptr<uds_connector>)> accept_slot) {
  return this->listen([=](SOCKET_T fd, const peer_t& remote_addr) {
    accept_slot(uds_connector::create(fd));
  });
}

/**
 * @brief Not allowed
*/
uds_listener::uds_listener() {
  inet_adapter::create_socket(kSocketTypeUnixDomain);
  net_utils::nonblocking(fd_, true);
  net_utils::nodelay(fd_, true);
  net_utils::reusable(fd_, true);
  net_utils::keepalive(fd_, true);
}


} // namespace peco

// Push Chen
