/*
    udp.cpp
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

#include "peco/net/udp.h"
#include "peco/net/bind.h"
#include "peco/net/connect.h"
#include "peco/utils.h"

namespace peco {

udp_connector::~udp_connector() {}

/**
 * @brief Factory
*/
std::shared_ptr<udp_connector> udp_connector::create() {
  return std::shared_ptr<udp_connector>(new udp_connector);
}

/**
 * @brief Create a tcp connector with a given fd and mark it as connected
*/
std::shared_ptr<udp_connector> udp_connector::create(SOCKET_T fd) {
  if (SOCKET_NOT_VALIDATE(fd)) {
    return udp_connector::create();
  } else {
    return std::shared_ptr<udp_connector>(new udp_connector(fd));
  }
}

/**
 * @brief Bind address to this socket, different socket type should use
 * different bind slot
*/
bool udp_connector::bind(const peer_t& local_addr) {
  if (connector_adapter::is_connected()) return false;
  return inet_adapter::bind(peer_bind(local_addr));
}
bool udp_connector::bind(const std::string& local_addr) {
  if (connector_adapter::is_connected()) return false;
  return inet_adapter::bind(peer_bind(local_addr));
}

/**
 * @brief Connect to peer
*/
bool udp_connector::connect(const peer_t& dest_addr) {
  connector_adapter::reset_connect_();
  return connector_adapter::connect(udp_connect(dest_addr));
}
bool udp_connector::connect(const std::string& dest_addr) {
  connector_adapter::reset_connect_();
  return connector_adapter::connect(udp_connect(dest_addr));
}

/**
 * @brief Get local port
*/
uint16_t udp_connector::localport() const {
  return net_utils::localport(fd_);
}
/**
 * @brief Get remote address
*/
peer_t udp_connector::peer_info() const {
  return net_utils::socket_peerinfo(fd_);
}

/**
 * @brief Not Allowed
*/
udp_connector::udp_connector() {
  inet_adapter::create_socket(kSocketTypeUDP);
  net_utils::nonblocking(fd_, true);
  net_utils::nodelay(fd_, true);
  net_utils::reusable(fd_, true);
  net_utils::keepalive(fd_, true);
}
udp_connector::udp_connector(SOCKET_T fd) : connector_adapter(fd, false) {
  net_utils::nonblocking(fd_, true);
  net_utils::nodelay(fd_, true);
  net_utils::reusable(fd_, true);
  net_utils::keepalive(fd_, true);
}

udp_listener::~udp_listener() { }
/**
 * @brief Factory
*/
std::shared_ptr<udp_listener> udp_listener::create() {
  return std::shared_ptr<udp_listener>(new udp_listener);
}

/**
 * @brief Bind address to this socket, different socket type should use
 * different bind slot
*/
bool udp_listener::bind(const std::string& local_addr) {
  bind_slot_ = peer_bind(local_addr);
  return inet_adapter::bind(bind_slot_);
}

/**
 * @brief Listen on the binded socket
*/
bool udp_listener::listen(std::function<void(std::shared_ptr<udp_connector>)> accept_slot) {
  return this->listen([=](SOCKET_T fd, const peer_t& remote_addr) {
    // transfer current fd to the incoming
    auto incoming = udp_connector::create(fd);
    incoming->connect(remote_addr);
    // Re-create a fd to listen
    this->fd_ = net_utils::create_socket(kSocketTypeUDP);
    net_utils::nonblocking(fd_, true);
    net_utils::nodelay(fd_, true);
    net_utils::reusable(fd_, true);
    net_utils::keepalive(fd_, true);
    this->bind_slot_(fd_);
    // Do the accept slot
    loop::current.run([=]() {
      accept_slot(incoming);
    });
  });
}
/**
 * @brief Listen on the binded socket in packet mode
*/
bool udp_listener::listen(std::function<void(std::shared_ptr<udp_packet>)> accept_slot) {
  if (listened_) return false;
  listened_ = true;
  
  auto self = this->shared_from_this();
  loop::current.run([self, accept_slot]() {
    std::string task_name = "udp_listen_packet:" + std::to_string(net_utils::localport(self->fd_));
    task::this_task().set_name(task_name.c_str());

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    while (true) {
      task::this_task().wait_fd_for_event(self->fd_, kEventTypeRead, PECO_TIME_S(1800));
      auto sig = task::this_task().signal();
      if (sig == kWaitingSignalBroken) {
        // task cancelled
        return;
      }
      if (sig == kWaitingSignalNothing) continue;
      std::string buffer;
      bool ret = net_utils::read(
        buffer, self->fd_, std::bind(::recvfrom, 
          std::placeholders::_1,
          std::placeholders::_2,
          std::placeholders::_3,
          0,
          (struct sockaddr *)&addr,
          &addr_len
        ), 2048);
      if (ret == false) {
        break;
      }
      if (buffer.size() > 0) {
        accept_slot(udp_packet::create(self->fd_, peer_t(addr), std::move(buffer)));
      }
    }
  }, PECO_CODE_LOCATION).set_atexit([self]() {
    self->listened_ = false;
  });
  return true;
}

/**
 * @brief Listen on the binded socket
*/
bool udp_listener::listen(listener_adapter::slot_accept_t accept_slot) {
  if (listened_) return false;
  listened_ = true;

  auto self = this->shared_from_this();
  loop::current.run([=]() {
    std::string task_name = "udp_listen:" + std::to_string(net_utils::localport(fd_));
    task::this_task().set_name(task_name.c_str());

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    while (!task::this_task().is_cancelled()) {
      task::this_task().wait_fd_for_event(self->fd_, kEventTypeRead, PECO_TIME_S(10));
      auto sig = task::this_task().signal();
      if (sig == kWaitingSignalBroken) {
        if (task::this_task().is_cancelled()) {
          log::info << "quit udp listening loop" << std::endl;
          break;
        } else {
          log::warning << "udp listening loop error, get broken sig, try again" << std::endl;
          continue;
        }
        // task cancelled
        // return;
      }
      if (sig == kWaitingSignalNothing) continue;
      size_t l = ::recvfrom(self->fd_, NULL, 0, MSG_PEEK, (struct sockaddr *)&addr, &addr_len);
      if (l < 0) {
        // error
        return;
      }
      accept_slot(self->fd_, peer_t(addr));
    }
  }, PECO_CODE_LOCATION).set_atexit([self]() {
    self->listened_ = false;
  });
  return true;
}

/**
 * @brief Write data to specified peer
*/
bool udp_listener::writeto(const peer_t& peer, const char* data, size_t length) {
  if (length == 0) return false;
  size_t sent = 0;
  struct sockaddr_in addr = (struct sockaddr_in)peer;
  do {
    int ret = net_utils::write(fd_, data + sent, 
      (length - sent < 1500 ? length - sent : 1500), 
      std::bind(::sendto, 
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3, 
        0 | SO_NETWORK_NOSIGNAL,
        (struct sockaddr *)&addr, 
        sizeof(addr)
      )
    );
    if (ret == -1) return false;
    sent += (size_t)ret;
  } while (sent < length);
  return true;
}
bool udp_listener::writeto(const peer_t& peer, const std::string& data) {
  return this->writeto(peer, data.c_str(), data.size());
}

/**
 * @brief Not allowed
*/
udp_listener::udp_listener() : bind_slot_(peer_t::nan) {
  inet_adapter::create_socket(kSocketTypeUDP);
  net_utils::nonblocking(fd_, true);
  net_utils::nodelay(fd_, true);
  net_utils::reusable(fd_, true);
  net_utils::keepalive(fd_, true);
}
/**
 * @brief Package data recived from a listener
*/

/**
 * @brief Get peer info of this packet
*/
const peer_t& udp_packet::peer_info() const {
  return source_addr_;
}

/**
 * @brief Get the data in packet
*/
const std::string& udp_packet::data() const {
  return pkt_;
}

/**
 * @brief Write data back to origin addr
*/
bool udp_packet::write(const char* data, size_t length) {
  if (length == 0) return false;
  size_t sent = 0;
  struct sockaddr_in addr = source_addr_;
  do {
    int ret = net_utils::write(fd_, data + sent, 
      (length - sent < 1500 ? length - sent : 1500), 
      std::bind(::sendto, 
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3, 
        0 | SO_NETWORK_NOSIGNAL,
        (struct sockaddr *)&addr, 
        sizeof(addr)
      )
    );
    if (ret == -1) return false;
    sent += (size_t)ret;
  } while (sent < length);
  return true;
}
bool udp_packet::write(const std::string& data) {
  return this->write(data.c_str(), data.size());
}

/**
 * @brief Static create for udp_listener
*/
std::shared_ptr<udp_packet> udp_packet::create(SOCKET_T fd, const peer_t& addr, std::string&& pkt) {
  return std::shared_ptr<udp_packet>(new udp_packet(fd, addr, std::move(pkt)));
}
/**
 * @brief Build a udp packet
*/
udp_packet::udp_packet(SOCKET_T fd, const peer_t& addr, std::string&& pkt)
  : fd_(fd), source_addr_(addr), pkt_(std::move(pkt))
{ }

} // namespace peco

// Push Chen
