/*
  adapter.cpp
  libpeco
  2022-02-20
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

#include "peco/net/adapter.h"
#include "peco/task.h"

namespace peco {

/**
 * @brief C'stors
*/
inet_incoming::inet_incoming(NetOpStatus s, const std::string& d)
  :status(s), data(d) { }
inet_incoming::inet_incoming(NetOpStatus s, std::string&& d)
  :status(s), data(std::move(d)) { }

/**
 * @brief Move
*/
inet_incoming::inet_incoming(inet_incoming&& other)
  :status(other.status), data(std::move(other.data)) { }
inet_incoming& inet_incoming::operator = (inet_incoming&& other) {
  if (this == &other) return *this;
  status = other.status;
  other.status = kNetOpStatusFailed;
  data = std::move(other.data);
  return *this;
}

/**
 * @brief Bool cast
*/
inet_incoming::operator bool() const {
  return (status == kNetOpStatusOK && data.size() > 0);
}

/**
 * @brief Move is allowed
*/
inet_adapter::inet_adapter(inet_adapter&& other)
  : fd_(other.fd_) { other.fd_ = INVALIDATE_SOCKET; }
inet_adapter& inet_adapter::operator = (inet_adapter&& other) {
  if (this == &other) return *this;
  fd_ = other.fd_;
  other.fd_ = INVALIDATE_SOCKET;
  return *this;
}

/**
 * @brief Virtual d'stor, will save close the socket
*/
inet_adapter::~inet_adapter() {
  this->close();
}

/**
 * @brief Bind address to this socket, different socket type should use
 * different bind slot
*/
bool inet_adapter::bind(inet_adapter::slot_bind_t bind_slot) {
  if (fd_ == INVALIDATE_SOCKET) return false;
  return bind_slot(fd_);
}

/**
 * @brief The default c'stor is not allowed to be invoked outside the class
*/
inet_adapter::inet_adapter(SOCKET_T related_fd) : fd_(related_fd) { }

/**
 * @brief Create the adapter's socket
*/
void inet_adapter::create_socket(SocketType sock_type) {
  if (fd_ != INVALIDATE_SOCKET) {
    assert(net_utils::socktype(fd_) == sock_type);
    return;
  }
  fd_ = net_utils::create_socket(sock_type);
}

/**
 * @brief Final close of the socket fd.
*/
void inet_adapter::close() {
  if (fd_ == INVALIDATE_SOCKET) return;
  SO_NETWORK_CLOSESOCK(fd_);
  fd_ = INVALIDATE_SOCKET;
}

connector_adapter::~connector_adapter() {}

/**
 * @brief Connect to peer
*/
bool connector_adapter::connect(connector_adapter::slot_connect_t connect_slot) {
  if (INVALIDATE_SOCKET == fd_) return false;
  if (is_connected_) return false;
  is_connected_ = connect_slot(fd_);
  return is_connected_;
}

/**
 * @brief Read data from peer socket
*/
inet_incoming connector_adapter::read(duration_t timedout, size_t bufsize) {
  if (SOCKET_NOT_VALIDATE(fd_)) return inet_incoming(kNetOpStatusFailed, "");
  if (!is_connected()) return inet_incoming(kNetOpStatusFailed, "");
  if (!net_utils::has_data_pending(fd_)) {
    task::this_task().wait_fd_for_event(fd_, kEventTypeRead, timedout);
    auto sig = task::this_task().signal();
    if (sig == kWaitingSignalNothing) return inet_incoming(kNetOpStatusTimedout, "");
    if (sig == kWaitingSignalBroken) return inet_incoming(kNetOpStatusFailed, "");
  }
  std::string buffer = std::forward<std::string>(
    net_utils::read(fd_, std::bind(
      ::recv, 
      std::placeholders::_1, 
      std::placeholders::_2, 
      std::placeholders::_3, 
      0 | SO_NETWORK_NOSIGNAL), bufsize)
  );

  return inet_incoming(kNetOpStatusOK, std::move(buffer));
}

/**
 * @brief Write data to peer socket
*/
bool connector_adapter::write(const char* data, size_t length, duration_t timedout) {
  if (SOCKET_NOT_VALIDATE(fd_)) return false;
  if (!is_connected()) return false;
  if ( length == 0 ) return true;
  size_t sent = 0;
  do {
    // Single Package max size is 4k
    int single_pkg = std::min((size_t)(length - sent), (size_t)(4 * 1024));
    int _ret = net_utils::write(
      fd_,
      data + sent, single_pkg,
      std::bind(::send,
          std::placeholders::_1,
          std::placeholders::_2,
          std::placeholders::_3,
          0 | SO_NETWORK_NOSIGNAL)
    );
    if ( _ret == -1 ) return false;
    sent += (size_t)_ret;

    if (sent < length) {
      task::this_task().wait_fd_for_event(fd_, kEventTypeWrite, timedout);
      auto sig = task::this_task().signal();
      if (sig == kWaitingSignalBroken) break;
      if (sig == kWaitingSignalNothing) break;
    }
  } while (sent < length);
  return sent == length;
}
bool connector_adapter::write(std::string&& data, duration_t timedout) {
  return this->write(data.c_str(), data.size(), timedout);
}
bool connector_adapter::write(const std::string& data, duration_t timedout) {
  return this->write(data.c_str(), data.size(), timedout);
}
/**
 * @brief Tell if the connector is already connected to peer
*/
bool connector_adapter::is_connected() const {
  return is_connected_;
}

connector_adapter::connector_adapter(SOCKET_T related_fd, bool connected) 
  : inet_adapter(related_fd), is_connected_(connected) {}

/**
 * @brief Reset connect status
*/
void connector_adapter::reset_connect_() {
  is_connected_ = false;
}

listener_adapter::~listener_adapter() { }
} // namespace peco

// Push Chen
